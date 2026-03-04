#include "virtual_robot.h"

#include <google/protobuf/util/json_util.h>

#include <QtEndian>
#include <cstring>
#include <QCoreApplication>

#include "QDebug"
#include "spdlog/spdlog.h"
#include "util/crc16_modbus.h"

//#include"control_panel.h"

namespace vir {


VirtualRobot::VirtualRobot(QObject *parent)
    : QObject{parent} {

    auto state = IMRState();
    state.SerializeAsString();//将整个消息(Protocol Buffers 消息对象)对象的所有字段都序列化成一个字符串
    state.SerializePartialAsString();//对部分字段进行序列化

    timer = new QTimer;

    connect(timer,&QTimer::timeout,this,&VirtualRobot::sendStatus);

}


VirtualRobot::~VirtualRobot() {
    //  timer->stop();
    //  timer->deleteLater();
}

void VirtualRobot::initConnection(QString addr, quint16 port) {
    client = new QTcpSocket(this);
    const QHostAddress address = QHostAddress(addr);
    //连接服务器并判断客户端是否打开
    client->connectToHost(address, port);

    if (client->isOpen()){
        spdlog::info("Virtual Bot connected({}:{}).", addr.toStdString().c_str(),port);
    }
    else{
        spdlog::info("Virtual Bot connected failed.");
    }


    //clint收到数据，触发下面操作
    connect(client, &QTcpSocket::readyRead, this, [=] {
        bool break_flag = true;
        //获取套接字中当前可读取的字节数，它返回一个 qint64 类型的整数
        while (client->bytesAvailable() > 0 && break_flag) {
            switch (unpackingStatus) {
            case unpackingEnum::UNPACKING_WAITING_PREFIX_1: {
                auto rx = client->read(1); //读取client的第一个字节放在rx中
                if (rx.at(0) == (char)0x02) {
                    unpackingStatus = unpackingEnum::UNPACKING_WAITING_PREFIX_2;
                }
                else {
                    spdlog::warn("UNPACKING_WAITING_PREFIX_1:{}",
                                 rx.toHex(' ').toStdString());
                    //将 QByteArray 对象 rx 中的数据以空格分隔的十六进制字符串的形式存储在 std::string 类型的变量中
                }
            } break;

            case unpackingEnum::UNPACKING_WAITING_PREFIX_2: {
                auto rx = client->read(1);
                //rx.at(0) 表示获取 rx 变量中索引为 0 的字节的值（rx 的第一个字母是否为‘T’）
                //【团标】每帧数据都必须包含的固定标识，从 RCS 到 IMR 方向为‘T’，从 IMR 到 RCS 方向为‘V’
                if (rx.at(0) == 'T') {
                    unpackingStatus = unpackingEnum::UNPACKING_WAITING_HEADER;
                }
                else {
                    spdlog::warn("UNPACKING_WAITING_PREFIX_2:{}",
                                 rx.toHex(' ').toStdString());
                }
            } break;

            case unpackingEnum::UNPACKING_WAITING_HEADER:
                if (client->bytesAvailable() >= 6) {
                    auto rx = client->read(6);
                    spdlog::info("Header Rx:{}", rx.toHex(' ').toStdString());

                    QByteArray head_pre;//帧头
                    head_pre.resize(2);
                    head_pre[0] = 0x02;
                    head_pre[1] = 'T';

                    auto crc = CRC16_MODBUS::CalcCRC16Modbus(
                        (unsigned char *)((head_pre + rx).left(6).data()), 6);
                    HeaderMsg *msg_ = (HeaderMsg *)rx.data();
                    spdlog::info("Header:({},{}).", msg_->len, msg_->check_sum);

                    if (crc == msg_->check_sum) {
                        rx_body_len = msg_->len;
                        spdlog::info("Header CRC Check Passed({}).", rx_body_len);
                    }
                    else {
                        spdlog::warn("Header CRC Check Error:{},{}", crc,
                                     msg_->check_sum);
                        rx_body_len = 0;
                    }
                    unpackingStatus = unpackingEnum::UNPACKING_WAITING_BODY;
                }
                else {
                    spdlog::warn("UNPACKING_WAITING_HEADER");
                    break_flag = false;
                }
                break;
            case unpackingEnum::UNPACKING_WAITING_BODY:
                if (client->bytesAvailable() >= rx_body_len + 3) {
                    auto rx = client->read(rx_body_len + 3);
                    qDebug() << rx.toHex(' ');

                    if (rx.back() == (char)0x03) {
                        auto crc = CRC16_MODBUS::CalcCRC16Modbus(
                            (unsigned char *)rx.left(rx.size() - 3).data(),
                            rx.size() - 3);
                        unsigned short *msg_crc =
                            (unsigned short *)rx.mid(rx.size() - 3, 2).data();

                        //              (rx.at(rx.size() - 2) << 8) + rx.at(rx.size() -
                        //              3);
                        if (crc == *msg_crc) {
                            messageProcess(rx.left(2).toStdString(),
                                           rx.mid(2, rx.size() - 5).toStdString());
                        } else {
                            spdlog::warn("UNPACKING_BODY:crc wrong({},{})", crc, *msg_crc);
                        }
                        //
                        //                  rx);
                    } else {
                        spdlog::warn("UNPACKING_BODY:tail wrong({})",
                                     rx.right(1).toHex(' ').toStdString());
                    }
                    unpackingStatus = unpackingEnum::UNPACKING_WAITING_PREFIX_1;
                } else {
                    spdlog::warn("UNPACKING_WAITING_BODY");
                    break_flag = false;
                }
                break;
            default:
                spdlog::error("Unexpected unpackingStatus:{}", (int)unpackingStatus);
                break;
            }
        }
        return;
    });

    connect(client, &QTcpSocket::disconnected, this, [=] {
        spdlog::error("[{}:{}]Socket Disconnected.",
                      client->peerAddress().toString().toStdString(),
                      client->peerPort());
    });

}

void VirtualRobot::deInitConnection() { client->deleteLater(); }

void VirtualRobot::sendRegister() {
    IMRRegister msg_rg;
    msg_rg.set_imrid(100);
    msg_rg.set_protocolversion("rt");

    IMRType msg_type;
    msg_type.set_imrtype("geekplusP40");
    msg_type.set_drivingstructure(DrivingStructureEnum::DIFFERENTIAL_DRIVE);
    msg_type.set_drivingmethod(DrivingMethodEnum::TWO_WHEEL);
    msg_type.set_loadcapacity(40000);
    msg_type.set_locatemode("2");
    msg_rg.mutable_imrtype()->MergeFrom(msg_type);

    IMRPhysicalParam msg_phy;
    msg_phy.set_imrlength(1000);
    msg_phy.set_imrwidth(500);
    msg_phy.set_imrheight(200);
    msg_phy.set_maxspeed(1000);
    msg_rg.mutable_imrphysicalparam()->MergeFrom(msg_phy);

    IMRGeometricFeature msg_geo;
    msg_rg.mutable_imrgeometricfeature()->MergeFrom(msg_geo);

    IMRLoadSpecification msg_load;
    msg_rg.mutable_imrloadspecification()->MergeFrom(msg_load);

    std::string reqTemp;
    google::protobuf::util::JsonPrintOptions options;
    options.add_whitespace = false;
    options.always_print_primitive_fields = true;
    options.preserve_proto_field_names = true;
    MessageToJsonString(msg_rg, &reqTemp, options);

    std::string body_type = "RG";
    auto body_msg = msg_rg.SerializeAsString();
    auto body = body_type + body_msg;

    QByteArray head;
    head.resize(8);
    head[0] = 0x02;
    head[1] = 'V';
    head[5] = (msg_rg.ByteSize() + 2) >> 24;
    head[4] = (msg_rg.ByteSize() + 2) >> 16;
    head[3] = (msg_rg.ByteSize() + 2) >> 8;
    head[2] = (msg_rg.ByteSize() + 2);
    auto crc =
        CRC16_MODBUS::CalcCRC16Modbus((unsigned char *)head.left(6).data(), 6);
    head[7] = crc >> 8;
    head[6] = crc;

    QByteArray tail;
    tail.resize(3);
    crc =
        CRC16_MODBUS::CalcCRC16Modbus((unsigned char *)body.c_str(), body.size());
    tail[1] = crc >> 8;
    tail[0] = crc;
    tail[2] = 0x03;

    spdlog::info("Head:{},Type:{},Rt({}):{},Tail:{}",
                 head.toHex(' ').toStdString(),
                 body_type,
                 msg_rg.ByteSize(),
                 reqTemp.c_str(),
                 tail.toHex(' ').toStdString());

    client->write((head.toStdString() + body + tail.toStdString()).c_str(),
                  body.size() + 11);
}

void VirtualRobot::startQTimer()
{
    timer->start(1000);
}

void VirtualRobot::stopQTimer()
{
    timer->stop();
    globalVar = 0;
    task.Clear();
    task_Imr.Clear();
    ImrSta.Clear();
}

void VirtualRobot::updatePos_1(int orderID,int missionKey,quint32 pointId_1,int newX_1, int newY_1,
                               quint32 pointId_2,int newX_2, int newY_2){
    emit sendAimToP40(orderID,missionKey,pointId_1,newX_1,newY_1,pointId_2,newX_2,newY_2);
}

void VirtualRobot::updateHeight(int orderID,int missionKey,quint32 pointId_1,int newX_1, int newY_1,int height){
    emit doRcsAction(orderID,missionKey,pointId_1,newX_1,newY_1,height);
}


void VirtualRobot::initValue(quint16 imrID,quint16 segID,quint16 imrlayerID,quint32 pointID){
    ImrSta.set_imrid(imrID);
    ImrSta.mutable_positionstate()->set_segid(segID);
    ImrSta.mutable_positionstate()->set_imrlayerid(imrlayerID);//车所在图层ID
    ImrSta.mutable_positionstate()->set_lastpasspointid(pointID);
}

//按照车发送状态的频率更新
void VirtualRobot::linkP40_pos(int p40_x,int p40_y,int directionId,int power,float speed,int loadHeight){

//    if(task_Imr.orderid() != 0){
//        if((p40_x >= task_Imr.missionpoints(0).pointposition().x()) && (p40_x <= task_Imr.missionpoints(1).pointposition().x()) &&
//            (p40_y >= task_Imr.missionpoints(0).pointposition().y()) && (p40_y <= task_Imr.missionpoints(1).pointposition().y())){
//            ImrSta.mutable_missionstate()->set_orderid(task_Imr.orderid());
//            ImrSta.mutable_missionstate()->set_missionkey(task_Imr.missionkey());
//            qDebug()<<"----------orderID:"<<task_array.at(i).orderid()<<endl;
//            qDebug()<<"----------missionKey:"<<task_array.at(i).missionkey()<<endl;
//        }
//    }


    ImrSta.mutable_positionstate()->set_x(p40_x);
    ImrSta.mutable_positionstate()->set_y(p40_y);
    ImrSta.mutable_positionstate()->set_imrangle(directionId);

    ImrSta.mutable_runningstate()->set_linearvelocity(speed*1000);
    ImrSta.mutable_runningstate()->set_angularvelocity(0.2);

    ImrSta.mutable_batterystate()->set_energyper(power);

    hight = loadHeight;

    if(task_Imr.orderid() != 0){
        ImrSta.mutable_positionstate()->set_segid(task_Imr.missionsegs(0).segid());
        ImrSta.mutable_positionstate()->set_segstartid(task_Imr.missionsegs(0).segstart());
        ImrSta.mutable_positionstate()->set_segendid(task_Imr.missionsegs(0).segend());
        ImrSta.mutable_positionstate()->set_lastpasspointid(task_Imr.missionsegs(0).segend());

        int seginput;//段进量
        seginput = (100 * sqrt(pow((p40_x - task_Imr.missionpoints(0).pointposition().x()),2) + pow((p40_y - task_Imr.missionpoints(0).pointposition().y()),2)))
                   /sqrt(pow((task_Imr.missionpoints(1).pointposition().x() - task_Imr.missionpoints(0).pointposition().x()),2) + pow((task_Imr.missionpoints(1).pointposition().y() - task_Imr.missionpoints(0).pointposition().y()),2));
        ImrSta.mutable_positionstate()->set_seginput(seginput);
    }
}

void VirtualRobot::updateStatus(){

    ImrSta.mutable_missionstate()->set_orderid(task_Imr.orderid());
    ImrSta.mutable_missionstate()->set_missionkey(task_Imr.missionkey());

    log_Sta = log_Sta + "MissionKey "+ QString::number(task_Imr.missionkey()) +" update successful!!!" + '\n';

    emit sendAimToP40(task_Imr.orderid(),
                        task_Imr.missionkey(),
                        task_Imr.missionpoints(0).pointid(),
                        task_Imr.missionpoints(0).pointposition().x(),
                        task_Imr.missionpoints(0).pointposition().y(),
                        task_Imr.missionpoints(1).pointid(),
                        task_Imr.missionpoints(1).pointposition().x(),
                        task_Imr.missionpoints(1).pointposition().y());
    qDebug()<<task_Imr.missionpoints(0).pointposition().x()<<"---"<<task_Imr.missionpoints(0).pointposition().y()<<"::"
            <<task_Imr.missionpoints(1).pointposition().x()<<"---"<<task_Imr.missionpoints(1).pointposition().y()<<'\n';

//    task_1 = task_Imr;

}

void VirtualRobot::posDecision(){

}

void VirtualRobot::logicDecision(){


    if((task.missionpoints(0).actions_size() != 0)||(task.missionpoints(1).actions_size() != 0)){
        action_number=1;
    }else{
        action_number=0;
    }

//    if(action_number.empty()){
        //第一个订单
        if(task_Imr.orderid() == 0 && task.orderid()!=task_Imr.orderid()){
            //任务点可达，开始执行任务
            if(ImrSta.positionstate().lastpasspointid()==task.missionpoints(0).pointid())
            {
                if(action_number == 0){
                    task_Imr = task;
                    log_Sta = log_Sta + "Order update successful!!!" + '\n';
                    VirtualRobot::updateStatus();
                }else{
                    task_Imr = task;
                    log_Sta = log_Sta + "Order update successful!!!" + '\n';
                    for(i=0;i<task_Imr.missionpoints_size();i++){
                        if(task_Imr.missionpoints(i).actions_size() != 0){
                            ID = task_Imr.missionpoints(1).actions(0).action().actionid();
                            Param = task_Imr.missionpoints(1).actions(0).action().actionparam();
                            VirtualRobot::updateStatus();
                            while(!((std::abs(ImrSta.positionstate().x()-task_Imr.missionpoints(i).pointposition().x()) <= 20) &&
                                     (std::abs(ImrSta.positionstate().y()-task_Imr.missionpoints(i).pointposition().y()) <= 20))){
                                QCoreApplication::processEvents(QEventLoop::AllEvents, 200); //非阻塞式
                                qDebug()<<"errorX: "<<std::abs(ImrSta.positionstate().x()-task_Imr.missionpoints(i).pointposition().x())<<
                                    "  errorY:"<<std::abs(ImrSta.positionstate().y()-task_Imr.missionpoints(i).pointposition().y())<<endl;
                            }
                            emit doRcsAction(task_Imr.orderid(),
                                             task_Imr.missionkey(),
                                             task_Imr.missionpoints(i).pointid(),
                                             task_Imr.missionpoints(i).pointposition().x(),
                                             task_Imr.missionpoints(i).pointposition().y(),
                                             std::stoi(task_Imr.missionpoints(i).actions(0).action().actionparam()));
                            while(!(std::abs( hight - std::stoi(task_Imr.missionpoints(i).actions(0).action().actionparam())) <= 20)){
                                QCoreApplication::processEvents(QEventLoop::AllEvents, 200); //非阻塞式
                                qDebug()<<"货架高度："<<hight<<"  目标高度："<<std::stoi(task_Imr.missionpoints(i).actions(0).action().actionparam())
                                    <<"errorH: "<<std::abs( hight - std::stoi(task_Imr.missionpoints(i).actions(0).action().actionparam()) )<<endl;
                            }
                            RunningState msg_run_st;
                            RunningState_ActionState msg_run_action_st;
                            msg_run_st.add_actionstates();
                            msg_run_action_st.set_actionid(ID);
                            msg_run_action_st.set_actionparam(Param);
                            msg_run_action_st.set_actionstate(ActionStateEnum::ACTION_COMPLETE);
                            msg_run_st.mutable_actionstates(0)->MergeFrom(msg_run_action_st);
                            ImrSta.mutable_runningstate()->MergeFrom(msg_run_st);

                        }
                    }
                }
            }
            //任务点不可达，拒绝任务，报告错误
            else
            {
                VirtualRobot::reportError();
            }
        }
        //同一个订单中更新 missionKey
        else if(task.orderid()==task_Imr.orderid()){
            if(task.missionkey() < task_Imr.missionkey() || task.missionkey() == task_Imr.missionkey())
            {
                VirtualRobot::reportError();
            }
            else{
                if(task.missionpoints(0).pointid()==task_Imr.missionpoints(task_Imr.missionpoints_size()-1).pointid()){
                    if(action_number == 0){
                        task_Imr = task;
                        qDebug()<<'\n'<<'\n'<<"MissionKey update successful!!! "<<'\n'<<'\n';
                        log_Sta=log_Sta +  "***MissionKey"+ QString::number(task_Imr.missionkey()) +" update successful!!!" + '\n';
                        VirtualRobot::updateStatus();
                        j=0;
                    }else{
                        task_Imr = task;
                        log_Sta = log_Sta + "******MissionKey "+ QString::number(task_Imr.missionkey()) +" update successful!!!" + '\n';
                        for(i=0;i<task_Imr.missionpoints_size();i++){
                            if(task_Imr.missionpoints(i).actions_size() != 0){
                                VirtualRobot::updateStatus();
                                if(i==0 && (std::abs( hight - std::stoi(task_Imr.missionpoints(i).actions(0).action().actionparam())) <= 20)){
                                    log_Sta = log_Sta + "Action done! ! !" + '\n';
                                }else{
                                    ID = task_Imr.missionpoints(1).actions(0).action().actionid();
                                    Param = task_Imr.missionpoints(1).actions(0).action().actionparam();

                                    while(!((std::abs(ImrSta.positionstate().x()-task_Imr.missionpoints(i).pointposition().x()) <= 20) &&
                                             (std::abs(ImrSta.positionstate().y()-task_Imr.missionpoints(i).pointposition().y()) <= 20))){
                                        QCoreApplication::processEvents(QEventLoop::AllEvents, 200); //非阻塞式
                                        //qDebug()<<"errorX: "<<std::abs(ImrSta.positionstate().x()-task_Imr.missionpoints(i).pointposition().x())<<
                                        //    "  errorY:"<<std::abs(ImrSta.positionstate().y()-task_Imr.missionpoints(i).pointposition().y())<<endl;
                                    }
                                    emit doRcsAction(task_Imr.orderid(),
                                                     task_Imr.missionkey(),
                                                     task_Imr.missionpoints(i).pointid(),
                                                     task_Imr.missionpoints(i).pointposition().x(),
                                                     task_Imr.missionpoints(i).pointposition().y(),
                                                     std::stoi(task_Imr.missionpoints(i).actions(0).action().actionparam()));
                                    while(!(std::abs( hight - std::stoi(task_Imr.missionpoints(i).actions(0).action().actionparam())) <= 20)){
                                        QCoreApplication::processEvents(QEventLoop::AllEvents, 200); //非阻塞式
                                        //if(std::abs( hight - std::stoi(task_Imr.missionpoints(i).actions(0).action().actionparam())) <= 30){
                                        //    qDebug()<<"货架高度："<<hight<<"  目标高度："<<std::stoi(task_Imr.missionpoints(i).actions(0).action().actionparam())
                                        //        <<"errorH: "<<std::abs( hight - (std::stoi(task_Imr.missionpoints(i).actions(0).action().actionparam())) )<<endl;
                                        //}
                                    }
                                    RunningState msg_run_st;
                                    RunningState_ActionState msg_run_action_st;
                                    msg_run_st.add_actionstates();
                                    msg_run_action_st.set_actionid(ID);
                                    msg_run_action_st.set_actionparam(Param);
                                    msg_run_action_st.set_actionstate(ActionStateEnum::ACTION_COMPLETE);
                                    msg_run_st.mutable_actionstates(0)->MergeFrom(msg_run_action_st);
                                    ImrSta.mutable_runningstate()->MergeFrom(msg_run_st);

                                }
                            }
                        }
                    }
                }
                else{
                    //拒绝任务
                    VirtualRobot::reportError();
                    qDebug()<<"Order connection points do not match!!! "<<'\n';
                }
            }
        }
        //切换订单
        else if(task_Imr.orderid() != 0 && task.orderid()!=task_Imr.orderid()){
            if(task.missionpoints(0).pointid()==task_Imr.missionpoints(task_Imr.missionpoints_size()-1).pointid()){
                task_Imr = task;
                log_Sta = log_Sta + "Order update successful!!!" + '\n';
                VirtualRobot::updateStatus();
                j=0;
            }
            else{
                //拒绝任务
                VirtualRobot::reportError();
                qDebug()<<"Order connection points do not match!!! "<<'\n';
            }
        }
//    }

}


void VirtualRobot::reportError(){

    qDebug()<<'\n'<<"Task update wrong!!! "<<'\n';
    qDebug()<<"Task update wrong!!! "<<"Current OrderID: "<<task_Imr.orderid()<<"  Current Key: "<<task_Imr.missionkey()<<"  lastPointID: "<<task_Imr.missionpoints(task_Imr.missionpoints_size()-1).pointid()<<'\n';
    qDebug()<<"Task update wrong!!! "<<"New Tsak OrderID: "<<task.orderid()<<"  New Task Key: "<<task.missionkey()<<"  firstPointID: "<<task.missionpoints(0).pointid()<<'\n'<<'\n'<<'\n';
    log_Sta = log_Sta + "Task update wrong!!!" + '\n';
}


void VirtualRobot::sendStatus() {

    log_Sta = "imrID：" + QString::number(ImrSta.imrid()) +
              "      heart：" +QString::number(globalVar) +
              "      orderID：" + QString::number(ImrSta.missionstate().orderid()) +
              "      Key：" + QString::number(ImrSta.missionstate().missionkey()) +
              "      lastPointID：" + QString::number(ImrSta.positionstate().lastpasspointid()) +
              "      X：" + QString::number(ImrSta.positionstate().x()) +
              "      Y：" + QString::number(ImrSta.positionstate().y()) +
              "      segID：" + QString::number(ImrSta.positionstate().segid()) +
              "      segInput：" + QString::number(ImrSta.positionstate().seginput()) + '\n';

    //VirtualRobot::logicDecision();
    globalVar+=1;
    ImrSta.set_heartcount(globalVar);

    AbnormalState msg_abnor_st;//异常事件状态信息
    std::vector<AbnormalState> abnor_sts;
    abnor_sts.push_back(msg_abnor_st);
    ImrSta.mutable_abnormalstates()->CopyFrom(
        {abnor_sts.begin(), abnor_sts.end()});

    FacilityState msg_faclility_st;//设备状态信息
    std::vector<FacilityState> faclility_sts;
    faclility_sts.push_back(msg_faclility_st);
    ImrSta.mutable_facilitystates()->CopyFrom(
        {faclility_sts.begin(), faclility_sts.end()});


    std::string reqTemp;
    //定义了一个名为 options 的 google::protobuf::util::JsonPrintOptions 类型的变量
    google::protobuf::util::JsonPrintOptions options;
    options.add_whitespace = false;
    options.always_print_primitive_fields = true;
    options.preserve_proto_field_names = true;
    //将 Protocol Buffer 对象 msg_st 序列化为 JSON 格式的字符串 repTemp
    MessageToJsonString(ImrSta, &reqTemp, options);

    std::string body_type = "ST";
    //将 msg_st 对象转换为字符串表示形式,存储在body_msg变量中
    auto body_msg = ImrSta.SerializeAsString();
    auto body = body_type + body_msg;

    QByteArray head;
    head.resize(8);
    head[0] = 0x02;
    head[1] = 'V';
    head[5] = (ImrSta.ByteSize() + 2) >> 24;
    head[4] = (ImrSta.ByteSize() + 2) >> 16;
    head[3] = (ImrSta.ByteSize() + 2) >> 8;
    head[2] = (ImrSta.ByteSize() + 2);
    auto crc =
        CRC16_MODBUS::CalcCRC16Modbus((unsigned char *)head.left(6).data(), 6);
    head[7] = crc >> 8;
    head[6] = crc;

    QByteArray tail;
    tail.resize(3);
    crc =
        CRC16_MODBUS::CalcCRC16Modbus((unsigned char *)body.c_str(), body.size());
    tail[1] = crc >> 8;
    tail[0] = crc;
    tail[2] = 0x03;

    spdlog::info("Head:{},Type:{},Rt({}):{},Tail:{}",
                 head.toHex(' ').toStdString(),
                 body_type,
                 ImrSta.ByteSize(),
                 reqTemp.c_str(),
                 tail.toHex(' ').toStdString());

    client->write((head.toStdString() + body + tail.toStdString()).c_str(),
                  body.size() + 11);

    if(ImrSta.runningstate().actionstates_size() != 0){
        ImrSta.mutable_runningstate()->clear_actionstates();
    }
}



void VirtualRobot::messageProcess(std::string type, std::string msg) {
    spdlog::info("messageProcess:{},{}", type, msg);
    if (type == "GM") //全局地图同步命令字
    {
        spdlog::warn("Unhandle msg GM");
    }
    else if (type == "AT")    //任务消息结构命令字
    {
        Mission task_AT;
        if (task_AT.ParseFromString(msg)) {
            std::string reqTemp;
            google::protobuf::util::JsonPrintOptions options;
            options.add_whitespace = false;
            options.always_print_primitive_fields = true;
            options.preserve_proto_field_names = true;
            MessageToJsonString(task_AT, &reqTemp, options);
            spdlog::info("Rx AT:{}\n\n\n", reqTemp.c_str());

            task = task_AT;
            log_Sta=log_Sta +  "MissionKey"+ QString::number(task.missionkey()) +" recisived!!!" + '\n';
            //task_array.push_back(task_AT);
            VirtualRobot::logicDecision();

        } else {
            spdlog::warn("ParseFromString failed.");
        }
        //spdlog::warn("Unhandle msg AT");
    }
    else if (type == "ST")    //状态消息结构命令字
    {
        spdlog::warn("Handling msg SA,but this msg is unexpected.");
        IMRState state;
        //解析字符串msg并将结果存储在state对象中
        if (state.ParseFromString(msg)) {
            std::string reqTemp;
            google::protobuf::util::JsonPrintOptions options;
            options.add_whitespace = false;
            options.always_print_primitive_fields = true;
            options.preserve_proto_field_names = true;
            //将转换后的JSON字符串存储在reqTemp中
            MessageToJsonString(state, &reqTemp, options);
            spdlog::info("Rx ST:{}", reqTemp.c_str());


        } else {
            spdlog::warn("ParseFromString failed.");
        }
    }
    else if (type == "SA")    //状态确认消息结构命令字
    {
        spdlog::warn("Unhandle msg SA");
    }
    else if (type == "OP")    //即时动作结构命令字
    {
        spdlog::warn("Unhandle msg OP");
    }
    else if (type == "RG")    //IMR注册接口模型命令字
    {
        spdlog::warn("Unhandle msg RG");
    } else {
        spdlog::error("Illegal type:{}", type);
    }
}
}  // namespace vir
