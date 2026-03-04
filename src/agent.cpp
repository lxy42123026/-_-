#include "agent.h"

#include <google/protobuf/util/json_util.h>

#include <QAbstractSocket>
#include <QDebug>
#include <QHostAddress>
#include <QPointF>
#include <QThread>
#include <QTimer>
#include <QUuid>
#include <QtEndian>
#include <iostream>
#include <cmath>

#include "ActionCmd.pb.h"
#include "ActionEventMsg.pb.h"
#include "AgvInitPosReq.pb.h"
#include "AgvInitPosResp.pb.h"
#include "AgvRegister.pb.h"
#include "AgvRegisterResp.pb.h"
#include "MessageBase.pb.h"
#include "QDateTime"
#include "SessionRegister.pb.h"
#include "spdlog/spdlog.h"

using google::protobuf::util::MessageToJsonString;

Agent::Agent(QObject *parent) : QObject{parent} {
  auto log = spdlog::get("rps_logger");
  //    spdlog::info("Create an agent.");
  log->info("Create an agent.");
  debug_vector.append(3);
  debug_vector.append(4);
  debug_vector.append(6);
  debug_vector.append(7);
  debug_vector.append(8);
  //  debug_vector.append(2);

  //connect(timer,&QTimer::timeout,this,&Agent::sendStatus);

}
bool Agent::protocolDecoding(const QByteArray &data) {
  //  spdlog::get("rps_logger")->info("protocolDecoding");
  //  bool res = false;
  QByteArray ba;
  ba.resize(2);
  ba[0] = 0xfe;
  ba[1] = 0xfe;

  //  unsigned short len = 0;
  //  len = (data.at(2) << 8) + data.at(3);
  Msg *msg = (Msg *)data.left(6).data();
  //  len |= data.at(3);
  //  len = (data.at(3)) & 0x000000FF;
  //  len = (data.at(2)) & 0x0000FF00;
  //  len = data.at(2) << 8 | data.at(3);
  //  len = len & 0x0000FFFF;
  //  spdlog::info("{}:{}", data.mid(2, 2).toHex(' ').toStdString(),
  //               QString::number(qToBigEndian(msg->len), 16).toStdString());
  //
  if (data.startsWith(ba)) {
    //    spdlog::info("protocolDecoding ok1");
    if (data.size() == qToBigEndian(msg->len)) {
      spdlog::info("Decode ok({}):{}", qToBigEndian(msg->type),
                   data.right(data.size() - 6).toStdString());

      //      spdlog::info("protocolDecoding ok2");
      if (qToBigEndian(msg->type) == 7) {
        cainiao::SessionRegisterInfoMessage smsg;
        //    //    smsg.sessionregisterinfo().;
        google::protobuf::util::JsonStringToMessage(
            data.right(data.size() - 6).toStdString(), &smsg);
        id = smsg.messagebase().id();
        sendSessionResponse();
      }
      return true;

    } else {
      spdlog::info("{}:{}", data.size(), (data.mid(2, 2).toShort() + 6));
    }
  } else {
    //    spdlog::info("protocolDecoding ok3");
  }
  spdlog::info("Decode false");
  return false;
}

void Agent::handleActionEvent() {}

void Agent::messageProcess(enMsgHeaderType type, const QByteArray &data) {
  switch (type) {
    case enMsgHeaderType::AGV_TYPE_NULL:
      spdlog::error("Undefined process:AGV_TYPE_NULL");
      break;
    case enMsgHeaderType::AGV_ACTION:
      spdlog::error("Undefined process:AGV_ACTION");
      break;
    case enMsgHeaderType::AGV_ACTION_EVENT: {
      cainiao::AgvActionEventMessage msg;
      google::protobuf::util::JsonStringToMessage(data.toStdString(), &msg);//将传入的 JSON 字符串 data 反序列化为 AgvActionEventMessage 对象，并将结果存储在 msg 中

      QStringList info;
      info << QString::fromStdString(msg.agvactionevent().actionid());
      info << QString::fromStdString(msg.agvactionevent().ActionTypeEnum_Name(
          msg.agvactionevent().actiontype()));
      //      agvStatus.AgvStateEnum_Name(agvStatus.agvstate()));
      info << QString("null");
      //将 msg 中的 actionevent 字段转换为 QString 对象添加到 info 列表中，ActionEventEnum_Name() 是通过枚举值获取对应名称的函数。
      info << QString::fromStdString(msg.agvactionevent().ActionEventEnum_Name(msg.agvactionevent().actionevent()));
      info << QString::number(msg.messagebase().netstarttm());

      emit actionInfo(info);
      spdlog::info("Receive AGV_ACTION_EVENT:{},({},{},{}).",
                   msg.agvactionevent().agvid(), info[0].toStdString(),
                   info[1].toStdString(), info[3].toStdString());
      //      spdlog::error("Undefined process:AGV_ACTION_EVENT");
    } break;
    case enMsgHeaderType::AGV_REGISTER_REQUEST: {
      //      spdlog::error("Undefined process:AGV_REGISTER_REQUEST");
      cainiao::AgvRegisterRequestMessage msg;
      google::protobuf::util::JsonStringToMessage(data.toStdString(), &msg);
      std::string agv_id = msg.agvregisterrequest().agvcode();
      double x = msg.agvregisterrequest().curx();
      double y = msg.agvregisterrequest().cury();
      spdlog::info("Receive AGV_REGISTER_REQUEST:{},({},{}).", agv_id, x, y);
      sendRegisterResponse(agv_id, x, y);
    } break;
    case enMsgHeaderType::AGV_RT_STATUS: {
      //      spdlog::error("Undefined process:AGV_RT_STATUS");
      cainiao::AgvRtStatusMessage rt_status;
      google::protobuf::util::JsonStringToMessage(data.toStdString(),
                                                  &rt_status);
      agvStatus = rt_status.agvrtstatus();
      std::string str;
      google::protobuf::util::JsonPrintOptions options;
      options.add_whitespace = true;    //在生成的JSON字符串中包含空格和缩进，使其更易读。
      options.always_print_primitive_fields = true; //始终打印原始字段
      options.preserve_proto_field_names = true; //保留Proto字段的原始名称，不对其进行任何更改
      MessageToJsonString(rt_status.agvrtstatus(), &str, options);
      emit rtStatus(QString::fromStdString(str));
    } break;
    case enMsgHeaderType::AGV_UNREGISTER_REQUEST:
      spdlog::error("Undefined process:AGV_UNREGISTER_REQUEST");
      break;
    case enMsgHeaderType::HEART_BEAT:
      //      spdlog::error("Undefined process:HEART_BEAT");
      if (!timer->isActive())//如果timer定时器不活动，则启动定时器
      {
        timer->start(1000);
      }
      //      sendHeatbeatResponse();
      emit heartBeat();
      break;
    case enMsgHeaderType::SESSION_REGISTER_INFO: {
      //      spdlog::error("Undefined process:SESSION_REGISTER_INFO");
      spdlog::info("Receive SESSION_REGISTER_INFO.");
      sendSessionResponse();
    }

    break;
    case enMsgHeaderType::INIT_POS_REQ: {
      //      spdlog::error("Undefined process:INIT_POS_REQ");
      cainiao::AgvGetOriginPointRequestMessage msg;
      google::protobuf::util::JsonStringToMessage(data.toStdString(), &msg);
      int code = msg.agvgetoriginpointrequest().code();
      spdlog::info("Receive INIT_POS_REQ:{}.", code);

      sendInitPosResponse(msg.agvgetoriginpointrequest().logicalx(),
                          msg.agvgetoriginpointrequest().logicaly(), code);
    } break;
    case enMsgHeaderType::INIT_POS_RESPONSE:
      spdlog::error("Undefined process:INIT_POS_RESPONSE");
      break;
    case enMsgHeaderType::AGV_REGISTER_RESPONSE:
      spdlog::error("Undefined process:AGV_REGISTER_RESPONSE");
      break;
    default:
      spdlog::error("Undefined process:{}", (int)type);
      break;
  }
}

void Agent::sendSessionResponse() {
  cainiao::MessageBase resp;
  resp.set_id(QUuid::createUuid().toString(QUuid::WithoutBraces).toStdString());
  resp.set_messagetypeenum(
      cainiao::MessageBase::MessageTypeEnum::MessageBase_MessageTypeEnum_INIT);
  //  cainiao::
  cainiao::MessageBase::Model *model = new cainiao::MessageBase::Model();
  model->set_modeenum(cainiao::MessageBase::Model::SYNC);
  model->set_typeenum(cainiao::MessageBase::Model::REQUEST);
  resp.set_allocated_mapping(model);
  //  resp.mutable_mapping()->MergeFrom(model);
  resp.set_resptimeoutmillis(15000);
  //  resp.set_collaborationid(id);
  resp.set_netstarttm(QDateTime::currentDateTimeUtc().toTime_t());
  resp.set_netendtm(0);
  std::string reqTemp;
  google::protobuf::util::JsonPrintOptions options;
  //  options.add_whitespace = true;
  //  options.always_print_primitive_fields = true;
  //  options.preserve_proto_field_names = true;

  MessageToJsonString(resp, &reqTemp, options);

  downSocketWrite(enMsgHeaderType::SESSION_REGISTER_INFO, reqTemp);
  //  QByteArray ba;
  //  ba.resize(6);
  //  ba[0] = 0xfe;
  //  ba[1] = 0xfe;
  //  ba[2] = (reqTemp.size() + 6) >> 8;
  //  ba[3] = (reqTemp.size() + 6);
  //  ba[4] = 0x00;
  //  ba[5] = 0x07;
  //  std::cout << 1;
  //  std::string tx = ba.toStdString() + reqTemp;
  //  spdlog::info("size:{}", tx.size());
  //  spdlog::info("[{}:{}]Socket Tx:{}",
  //               socket_down.peerAddress().toString().toStdString(),
  //               socket_down.peerPort(),
  //               QString::fromUtf8(QByteArray::fromStdString(tx).toHex(' '))
  //                   .toUpper()
  //                   .toStdString());
  //  spdlog::info("[{}:{}]Socket Tx:{}",
  //               socket_down.peerAddress().toString().toStdString(),
  //               socket_down.peerPort(), tx);
  //  socket_down->write(tx.c_str(), tx.size());
  spdlog::info("Return SESSION_REGISTER_INFO.");
}

void Agent::sendHeatbeatResponse() {
  cainiao::MessageBase resp;
  resp.set_id(QUuid::createUuid().toString(QUuid::WithoutBraces).toStdString());
  resp.set_messagetypeenum(cainiao::MessageBase::MessageTypeEnum::
                               MessageBase_MessageTypeEnum_HEART_BEAT);
  //  cainiao::
  cainiao::MessageBase::Model *model = new cainiao::MessageBase::Model();
  model->set_modeenum(cainiao::MessageBase::Model::SYNC);
  model->set_typeenum(cainiao::MessageBase::Model::REQUEST);
  resp.set_allocated_mapping(model);
  //  resp.mutable_mapping()->MergeFrom(model);
  resp.set_resptimeoutmillis(15000);
  //  resp.set_collaborationid(id);
  resp.set_netstarttm(QDateTime::currentDateTimeUtc().toTime_t());
  resp.set_netendtm(0);
  std::string reqTemp;
  google::protobuf::util::JsonPrintOptions options;
  //  options.add_whitespace = true;
  //  options.always_print_primitive_fields = true;
  //  options.preserve_proto_field_names = true;

  MessageToJsonString(resp, &reqTemp, options);

  downSocketWrite(enMsgHeaderType::HEART_BEAT, reqTemp);
  //  QByteArray ba;
  //  ba.resize(6);
  //  ba[0] = 0xfe;
  //  ba[1] = 0xfe;
  //  ba[2] = (reqTemp.size() + 6) >> 8;
  //  ba[3] = (reqTemp.size() + 6);
  //  ba[4] = 0x00;
  //  ba[5] = 0x06;
  //  std::cout << 1;
  //  std::string tx = ba.toStdString() + reqTemp;
  //  spdlog::info("size:{}", tx.size());
  //  spdlog::info("[{}:{}]Socket Tx:{}",
  //               socket_down.peerAddress().toString().toStdString(),
  //               socket_down.peerPort(),
  //               QString::fromUtf8(QByteArray::fromStdString(tx).toHex(' '))
  //                   .toUpper()
  //                   .toStdString());
  //  spdlog::info("[{}:{}]Socket Tx:{}",
  //               socket_down.peerAddress().toString().toStdString(),
  //               socket_down.peerPort(), tx);
  //  socket_down->write(tx.c_str(), tx.size());
  //  INIT_POS_RESPONSE
}

void Agent::sendInitPosResponse(int logic_x, int logic_y, int code) {
  cainiao::AgvGetOriginPointResponseMessage body;

  cainiao::MessageBase resp;
  resp.set_id(QUuid::createUuid().toString(QUuid::WithoutBraces).toStdString());
  resp.set_messagetypeenum(
      cainiao::MessageBase::MessageTypeEnum::MessageBase_MessageTypeEnum_DATA);
  //  cainiao::
  cainiao::MessageBase::Model *model = new cainiao::MessageBase::Model();
  model->set_path("/URcs/biz/origin/point");
  model->set_modeenum(cainiao::MessageBase::Model::SYNC);
  model->set_typeenum(cainiao::MessageBase::Model::REQUEST);
  resp.set_allocated_mapping(model);
  //  resp.mutable_mapping()->MergeFrom(model);
  resp.set_resptimeoutmillis(15000);
  //  resp.set_collaborationid(id);
  resp.set_netstarttm(QDateTime::currentDateTimeUtc().toTime_t());
  resp.set_netendtm(0);

  body.mutable_messagebase()->MergeFrom(resp);
  QPointF p;
  if (!map->getPointById(code, p)) {
    p.setX(logic_x);
    p.setY(logic_y);
    spdlog::info("Could not getPoint,use logical:({},{}).", p.x(), p.y());
  }

  //  qDebug() << code << p;
  cainiao::AgvGetOriginPointResponseMessage::AgvGetOriginPointResponse resp2;
  resp2.set_logicalx(logic_x);
  resp2.set_logicaly(logic_y);
  resp2.set_originx(p.x());
  resp2.set_originy(p.y());
  body.mutable_agvgetoriginpointresponse()->MergeFrom(resp2);

  std::string reqTemp;
  //  google::protobuf::util::JsonPrintOptions options;
  //  options.add_whitespace = true;
  //  options.always_print_primitive_fields = true;
  //  options.preserve_proto_field_names = true;

  MessageToJsonString(body, &reqTemp);

  downSocketWrite(enMsgHeaderType::INIT_POS_RESPONSE, reqTemp);
  //  QByteArray ba;
  //  ba.resize(6);
  //  ba[0] = 0xfe;
  //  ba[1] = 0xfe;
  //  ba[2] = (reqTemp.size() + 6) >> 8;
  //  ba[3] = (reqTemp.size() + 6);
  //  ba[4] = 0x00;
  //  ba[5] = 0x09;
  //  std::cout << 1;
  //  std::string tx = ba.toStdString() + reqTemp;
  //  spdlog::info("size:{}", tx.size());
  //  spdlog::info("[{}:{}]Socket Tx:{}",
  //               socket_down.peerAddress().toString().toStdString(),
  //               socket_down.peerPort(),
  //               QString::fromUtf8(QByteArray::fromStdString(tx).toHex(' '))
  //                   .toUpper()
  //                   .toStdString());
  //  spdlog::info("[{}:{}]Socket Tx:{}",
  //               socket_down.peerAddress().toString().toStdString(),
  //               socket_down.peerPort(), tx);
  //  socket_down->write(tx.c_str(), tx.size());
  spdlog::info("Return INIT_POS_RESPONSE:({},{}).", p.x(), p.y());
}

void Agent::sendRegisterResponse(std::string agv_id, double x, double y) {
  cainiao::AgvRegisterResponseMessage body;

  cainiao::MessageBase resp;
  resp.set_id(QUuid::createUuid().toString(QUuid::WithoutBraces).toStdString());
  resp.set_messagetypeenum(
      cainiao::MessageBase::MessageTypeEnum::MessageBase_MessageTypeEnum_DATA);
  //  cainiao::
  cainiao::MessageBase::Model *model = new cainiao::MessageBase::Model();
  model->set_path("/URcs/biz/register");
  model->set_modeenum(cainiao::MessageBase::Model::SYNC);
  model->set_typeenum(cainiao::MessageBase::Model::REQUEST);
  resp.set_allocated_mapping(model);
  //  resp.mutable_mapping()->MergeFrom(model);
  resp.set_resptimeoutmillis(15000);
  //  resp.set_collaborationid(id);
  resp.set_netstarttm(QDateTime::currentDateTimeUtc().toTime_t());
  resp.set_netendtm(0);

  body.mutable_messagebase()->MergeFrom(resp);

  cainiao::AgvRegisterResponseMessage::AgvRegisterResponse resp2;
  //  resp2.set_code("AGV-1511639");
  resp2.set_agvcode(agv_id);
  resp2.set_result(true);
  resp2.set_pixelx(x);
  resp2.set_pixely(y);
  body.mutable_agvregisterresponse()->MergeFrom(resp2);

  std::string reqTemp;
  //  google::protobuf::util::JsonPrintOptions options;
  //  options.add_whitespace = true;
  //  options.always_print_primitive_fields = true;
  //  options.preserve_proto_field_names = true;

  MessageToJsonString(body, &reqTemp);

  downSocketWrite(enMsgHeaderType::AGV_REGISTER_RESPONSE, reqTemp);
  //  QByteArray ba;
  //  ba.resize(6);
  //  ba[0] = 0xfe;
  //  ba[1] = 0xfe;
  //  ba[2] = (reqTemp.size() + 6) >> 8;
  //  ba[3] = (reqTemp.size() + 6);
  //  ba[4] = 0x00;
  //  ba[5] = 0x0a;
  //  std::cout << 1;
  //  std::string tx = ba.toStdString() + reqTemp;
  //  spdlog::info("size:{}", tx.size());
  //  spdlog::info("[{}:{}]Socket Tx:{}",
  //               socket_down.peerAddress().toString().toStdString(),
  //               socket_down.peerPort(),
  //               QString::fromUtf8(QByteArray::fromStdString(tx).toHex(' '))
  //                   .toUpper()
  //                   .toStdString());
  //  spdlog::info("[{}:{}]Socket Tx:{}",
  //               socket_down.peerAddress().toString().toStdString(),
  //               socket_down.peerPort(), tx);
  //  socket_down->write(tx.c_str(), tx.size());
  spdlog::info("Return AGV_REGISTER_RESPONSE.");
}

void Agent::sendActionLift(int height) {
  cainiao::AgvActionMessage body;

  cainiao::MessageBase resp;
  resp.set_id(QUuid::createUuid().toString(QUuid::WithoutBraces).toStdString());
  resp.set_messagetypeenum(
      cainiao::MessageBase::MessageTypeEnum::MessageBase_MessageTypeEnum_DATA);
  //  cainiao::
  cainiao::MessageBase::Model *model = new cainiao::MessageBase::Model();
  model->set_path("/URcs/biz/publish/command");
  model->set_modeenum(cainiao::MessageBase::Model::SYNC);
  model->set_typeenum(cainiao::MessageBase::Model::NOTIFY);
  resp.set_allocated_mapping(model);
  //  resp.mutable_mapping()->MergeFrom(model);
  resp.set_resptimeoutmillis(15000);
  //  resp.set_collaborationid(id);
  resp.set_netstarttm(QDateTime::currentDateTimeUtc().toTime_t());
  resp.set_netendtm(0);

  body.mutable_messagebase()->MergeFrom(resp);

  cainiao::AgvActionMessage::AgvAction resp2;
  resp2.set_agvid(agvStatus.agvid());
  resp2.set_traceid(std::to_string(traceCnt++));
  resp2.set_actionid(std::to_string(actionCnt++));
  resp2.set_actiontype(cainiao::AgvActionMessage::AgvAction::ACTION_LIFT);
  cainiao::AgvActionMessage::AgvAction::ActionPoint resp2_1;
  resp2_1.set_id(1);
  resp2_1.set_originx(agvStatus.pixelx());
  resp2_1.set_originy(agvStatus.pixely());
  resp2_1.set_distance(0);
  resp2_1.set_direction(0);
  //  resp2.mutable_currentpoint()->MergeFrom(resp2_1);
  //  resp2.set_allocated_currentpoint(resp2_1);

  std::vector<cainiao::AgvActionMessage::AgvAction::ActionPoint> points;
  points.push_back(resp2_1);

  resp2.mutable_points()->CopyFrom({points.begin(), points.end()});
  //  VehicleNavigationStage navigation_stage;

  //  for (int i = 0; i < navigation_stage.mission_points_size(); i++) {
  //    mission_points.push_back(navigation_stage.mission_points(i));
  //  }

  //  resp2.mutable_currentpoint()->MergeFrom(resp2_1);
  //  google::protobuf::RepeatedPtrField<
  //      cainiao::AgvActionMessage::AgvAction::ActionPoint>
  //      points;
  //  points.AddAllocated(resp2_1);
  //  resp2.mutable_points(&points);
  //  cainiao::AgvActionMessage::AgvAction::ActionPoint *resp2_2 =
  //      resp2.add_points();
  //  resp2_2->set_id(1);
  //  resp2_1.set_originx(agvStatus.pixelx());
  //  resp2_1.set_originy(agvStatus.pixely());
  //  resp2_2->set_distance(0);
  //  resp2_2->set_direction(0);
  //  resp2.set_bucketcode("null");
  //  resp2.set_bucketface("0");
  resp2.set_directionid(0);
  resp2.set_nostrongcheck(false);
  resp2.set_followflag(false);
  resp2.set_workmode(cainiao::AgvActionMessage::AgvAction::REGISTERED);
  resp2.set_retrytimes(1);
  resp2.set_loadheight(height);
  //  resp2.

  body.mutable_agvaction()->MergeFrom(resp2);

  std::string reqTemp;
  //  google::protobuf::util::JsonPrintOptions options;
  //  options.add_whitespace = true;
  //  options.always_print_primitive_fields = true;
  //  options.preserve_proto_field_names = true;

  MessageToJsonString(body, &reqTemp);

  downSocketWrite(enMsgHeaderType::AGV_ACTION, reqTemp);
  //  QByteArray ba;
  //  ba.resize(6);
  //  ba[0] = 0xfe;
  //  ba[1] = 0xfe;
  //  ba[2] = (reqTemp.size() + 6) >> 8;
  //  ba[3] = (reqTemp.size() + 6);
  //  ba[4] = 0x00;
  //  ba[5] = 0x01;
  //  std::cout << 1;
  //  std::string tx = ba.toStdString() + reqTemp;
  //  spdlog::info("size:{}", tx.size());
  //  spdlog::info("[{}:{}]Socket Tx:{}",
  //               socket_down.peerAddress().toString().toStdString(),
  //               socket_down.peerPort(),
  //               QString::fromUtf8(QByteArray::fromStdString(tx).toHex(' '))
  //                   .toUpper()
  //                   .toStdString());
  //  spdlog::info("[{}:{}]Socket Tx:{}",
  //               socket_down->peerAddress().toString().toStdString(),
  //               socket_down->peerPort(), tx);
  //  socket_down->write(tx.c_str(), tx.size());
}

void Agent::sendActionPut(int height) {
  cainiao::AgvActionMessage body;

  cainiao::MessageBase resp;
  resp.set_id(QUuid::createUuid().toString(QUuid::WithoutBraces).toStdString());
  resp.set_messagetypeenum(
      cainiao::MessageBase::MessageTypeEnum::MessageBase_MessageTypeEnum_DATA);
  //  cainiao::
  cainiao::MessageBase::Model *model = new cainiao::MessageBase::Model();
  model->set_path("/URcs/biz/publish/command");
  model->set_modeenum(cainiao::MessageBase::Model::SYNC);
  model->set_typeenum(cainiao::MessageBase::Model::NOTIFY);
  resp.set_allocated_mapping(model);
  //  resp.mutable_mapping()->MergeFrom(model);
  resp.set_resptimeoutmillis(15000);
  //  resp.set_collaborationid(id);
  resp.set_netstarttm(QDateTime::currentDateTimeUtc().toTime_t());
  resp.set_netendtm(0);

  body.mutable_messagebase()->MergeFrom(resp);

  cainiao::AgvActionMessage::AgvAction resp2;
  resp2.set_agvid(agvStatus.agvid());
  resp2.set_traceid(std::to_string(traceCnt++));
  resp2.set_actionid(std::to_string(actionCnt++));
  resp2.set_actiontype(cainiao::AgvActionMessage::AgvAction::ACTION_PUT);
  cainiao::AgvActionMessage::AgvAction::ActionPoint resp2_1;
  resp2_1.set_id(1);
  resp2_1.set_originx(agvStatus.pixelx());
  resp2_1.set_originy(agvStatus.pixely());
  resp2_1.set_distance(0);
  resp2_1.set_direction(0);
  //  resp2.mutable_currentpoint()->MergeFrom(resp2_1);
  //  resp2.set_allocated_currentpoint(resp2_1);

  std::vector<cainiao::AgvActionMessage::AgvAction::ActionPoint> points;
  points.push_back(resp2_1);

  //  resp2.mutable_points()->CopyFrom({points.begin(), points.end()});
  //  VehicleNavigationStage navigation_stage;

  //  for (int i = 0; i < navigation_stage.mission_points_size(); i++) {
  //    mission_points.push_back(navigation_stage.mission_points(i));
  //  }

  //  resp2.mutable_currentpoint()->MergeFrom(resp2_1);
  //  google::protobuf::RepeatedPtrField<
  //      cainiao::AgvActionMessage::AgvAction::ActionPoint>
  //      points;
  //  points.AddAllocated(resp2_1);
  //  resp2.mutable_points(&points);
  //  cainiao::AgvActionMessage::AgvAction::ActionPoint *resp2_2 =
  //      resp2.add_points();
  //  resp2_2->set_id(1);
  //  resp2_1.set_originx(agvStatus.pixelx());
  //  resp2_1.set_originy(agvStatus.pixely());
  //  resp2_2->set_distance(0);
  //  resp2_2->set_direction(0);
  //  resp2.set_bucketcode("null");
  //  resp2.set_bucketface("0");
  resp2.set_directionid(0);
  resp2.set_nostrongcheck(false);
  resp2.set_followflag(false);
  resp2.set_workmode(cainiao::AgvActionMessage::AgvAction::REGISTERED);
  resp2.set_retrytimes(1);
  resp2.set_loadheight(height);
  //  resp2.

  body.mutable_agvaction()->MergeFrom(resp2);

  std::string reqTemp;
  //  google::protobuf::util::JsonPrintOptions options;
  //  options.add_whitespace = true;
  //  options.always_print_primitive_fields = true;
  //  options.preserve_proto_field_names = true;

  MessageToJsonString(body, &reqTemp);

  downSocketWrite(enMsgHeaderType::AGV_ACTION, reqTemp);
  //  QByteArray ba;
  //  ba.resize(6);
  //  ba[0] = 0xfe;
  //  ba[1] = 0xfe;
  //  ba[2] = (reqTemp.size() + 6) >> 8;
  //  ba[3] = (reqTemp.size() + 6);
  //  ba[4] = 0x00;
  //  ba[5] = 0x01;
  //  std::cout << 1;
  //  std::string tx = ba.toStdString() + reqTemp;
  //  spdlog::info("size:{}", tx.size());
  //  spdlog::info("[{}:{}]Socket Tx:{}",
  //               socket_down.peerAddress().toString().toStdString(),
  //               socket_down.peerPort(),
  //               QString::fromUtf8(QByteArray::fromStdString(tx).toHex(' '))
  //                   .toUpper()
  //                   .toStdString());
  //  spdlog::info("[{}:{}]Socket Tx:{}",
  //               socket_down->peerAddress().toString().toStdString(),
  //               socket_down->peerPort(), tx);
  //  socket_down->write(tx.c_str(), tx.size());
}

void Agent::sendActionTurn(int direction) {
  cainiao::AgvActionMessage body;

  cainiao::MessageBase msgBase;
  genMsgBase(
      msgBase,
      cainiao::MessageBase::MessageTypeEnum::MessageBase_MessageTypeEnum_DATA,
      "/URcs/biz/publish/command", cainiao::MessageBase::Model::NOTIFY);
  body.mutable_messagebase()->MergeFrom(msgBase);

  cainiao::AgvActionMessage::AgvAction msgAction;
  msgAction.set_agvid(agvStatus.agvid());
  msgAction.set_traceid(std::to_string(traceCnt++));
  msgAction.set_actionid(std::to_string(actionCnt++));
  msgAction.set_actiontype(cainiao::AgvActionMessage::AgvAction::ACTION_ROTATE);

  cainiao::AgvActionMessage::AgvAction::ActionPoint currentP;
  currentP.set_id(1);
  currentP.set_originx(agvStatus.pixelx());
  currentP.set_originy(agvStatus.pixely());
  currentP.set_distance(0);
  currentP.set_direction(0);
  msgAction.mutable_currentpoint()->MergeFrom(currentP);

  std::vector<cainiao::AgvActionMessage::AgvAction::ActionPoint> points;
  points.push_back(currentP);
  msgAction.mutable_points()->CopyFrom({points.begin(), points.end()});

  //  resp2.set_bucketcode();
  //  resp2.set_bucketface("1");
  msgAction.set_directionid(direction);
  msgAction.set_followflag(false);
  msgAction.set_workmode(cainiao::AgvActionMessage::AgvAction::REGISTERED);
  msgAction.set_retrytimes(1);
  body.mutable_agvaction()->MergeFrom(msgAction);

  std::string reqTemp;
  MessageToJsonString(body, &reqTemp);

  downSocketWrite(enMsgHeaderType::AGV_ACTION, reqTemp);
  QStringList info;
  info << QString::fromStdString(msgAction.actionid());
  info << QString::fromStdString(
      msgAction.ActionTypeEnum_Name(msgAction.actiontype()));
  //      agvStatus.AgvStateEnum_Name(agvStatus.agvstate()));
  info << QString("null");
  info << QString("ACTION_SENT");
  info << QString::number(msgBase.netstarttm());
  emit actionInfo(info);
  //  QByteArray ba;
  //  ba.resize(6);
  //  ba[0] = 0xfe;
  //  ba[1] = 0xfe;
  //  ba[2] = (reqTemp.size() + 6) >> 8;
  //  ba[3] = (reqTemp.size() + 6);
  //  ba[4] = 0x00;
  //  ba[5] = 0x01;
  //  std::cout << 1;
  //  std::string tx = ba.toStdString() + reqTemp;
  //  spdlog::info("size:{}", tx.size());
  //  spdlog::info("[{}:{}]Socket Tx:{}",
  //               socket_down.peerAddress().toString().toStdString(),
  //               socket_down.peerPort(),
  //               QString::fromUtf8(QByteArray::fromStdString(tx).toHex(' '))
  //                   .toUpper()
  //                   .toStdString());
  //  spdlog::info("[{}:{}]Socket Tx:{}",
  //               socket_down->peerAddress().toString().toStdString(),
  //               socket_down->peerPort(), tx);
  //  socket_down->write(tx.c_str(), tx.size());
}

void Agent::sendActionCharge() {
  cainiao::AgvActionMessage body;

  cainiao::MessageBase msgBase;
  genMsgBase(
      msgBase,
      cainiao::MessageBase::MessageTypeEnum::MessageBase_MessageTypeEnum_DATA,
      "/URcs/biz/publish/command", cainiao::MessageBase::Model::NOTIFY);
  body.mutable_messagebase()->MergeFrom(msgBase);

  cainiao::AgvActionMessage::AgvAction msgAction;
  msgAction.set_agvid(agvStatus.agvid());
  msgAction.set_traceid(std::to_string(traceCnt++));
  msgAction.set_actionid(std::to_string(actionCnt++));
  msgAction.set_actiontype(cainiao::AgvActionMessage::AgvAction::ACTION_CHARGE);

  cainiao::AgvActionMessage::AgvAction::ActionPoint currentP;
  currentP.set_id(1);
  currentP.set_originx(agvStatus.pixelx());
  currentP.set_originy(agvStatus.pixely());
  currentP.set_distance(0);
  currentP.set_direction(3);
  msgAction.mutable_currentpoint()->MergeFrom(currentP);

  std::vector<cainiao::AgvActionMessage::AgvAction::ActionPoint> points;
  points.push_back(currentP);

  msgAction.mutable_points()->CopyFrom({points.begin(), points.end()});

  //  resp2.set_bucketcode();
  //  resp2.set_bucketface("1");
  msgAction.set_directionid(3);
  msgAction.set_followflag(false);
  msgAction.set_workmode(cainiao::AgvActionMessage::AgvAction::REGISTERED);
  msgAction.set_retrytimes(1);
  body.mutable_agvaction()->MergeFrom(msgAction);

  std::string reqTemp;
  MessageToJsonString(body, &reqTemp);

  downSocketWrite(enMsgHeaderType::AGV_ACTION, reqTemp);
  QStringList info;
  info << QString::fromStdString(msgAction.actionid());
  info << QString::fromStdString(
      msgAction.ActionTypeEnum_Name(msgAction.actiontype()));
  //      agvStatus.AgvStateEnum_Name(agvStatus.agvstate()));
  info << QString("null");
  info << QString("ACTION_SENT");
  info << QString::number(msgBase.netstarttm());
  emit actionInfo(info);
  //  QByteArray ba;
  //  ba.resize(6);
  //  ba[0] = 0xfe;
  //  ba[1] = 0xfe;
  //  ba[2] = (reqTemp.size() + 6) >> 8;
  //  ba[3] = (reqTemp.size() + 6);
  //  ba[4] = 0x00;
  //  ba[5] = 0x01;
  //  std::cout << 1;
  //  std::string tx = ba.toStdString() + reqTemp;
  //  spdlog::info("size:{}", tx.size());
  //  spdlog::info("[{}:{}]Socket Tx:{}",
  //               socket_down.peerAddress().toString().toStdString(),
  //               socket_down.peerPort(),
  //               QString::fromUtf8(QByteArray::fromStdString(tx).toHex(' '))
  //                   .toUpper()
  //                   .toStdString());
  //  spdlog::info("[{}:{}]Socket Tx:{}",
  //               socket_down->peerAddress().toString().toStdString(),
  //               socket_down->peerPort(), tx);
  //  socket_down->write(tx.c_str(), tx.size());
}

void Agent::sendActionUncharge() {
  cainiao::AgvActionMessage body;

  cainiao::MessageBase msgBase;
  genMsgBase(
      msgBase,
      cainiao::MessageBase::MessageTypeEnum::MessageBase_MessageTypeEnum_DATA,
      "/URcs/biz/publish/command", cainiao::MessageBase::Model::NOTIFY);
  body.mutable_messagebase()->MergeFrom(msgBase);

  cainiao::AgvActionMessage::AgvAction msgAction;
  msgAction.set_agvid(agvStatus.agvid());
  msgAction.set_traceid(std::to_string(traceCnt++));
  msgAction.set_actionid(std::to_string(actionCnt++));
  msgAction.set_actiontype(
      cainiao::AgvActionMessage::AgvAction::ACTION_UNCHARGE);

  cainiao::AgvActionMessage::AgvAction::ActionPoint currentP;
  currentP.set_id(1);
  currentP.set_originx(agvStatus.pixelx());
  currentP.set_originy(agvStatus.pixely());
  currentP.set_distance(0);
  currentP.set_direction(0);
  msgAction.mutable_currentpoint()->MergeFrom(currentP);

  std::vector<cainiao::AgvActionMessage::AgvAction::ActionPoint> points;
  points.push_back(currentP);
  msgAction.mutable_points()->CopyFrom({points.begin(), points.end()});

  //  resp2.set_bucketcode();
  //  resp2.set_bucketface("1");
  msgAction.set_directionid(agvStatus.directionid());
  msgAction.set_followflag(false);
  msgAction.set_workmode(cainiao::AgvActionMessage::AgvAction::REGISTERED);
  msgAction.set_retrytimes(1);
  body.mutable_agvaction()->MergeFrom(msgAction);

  std::string reqTemp;
  MessageToJsonString(body, &reqTemp);

  downSocketWrite(enMsgHeaderType::AGV_ACTION, reqTemp);
  QStringList info;
  info << QString::fromStdString(msgAction.actionid());
  info << QString::fromStdString(
      msgAction.ActionTypeEnum_Name(msgAction.actiontype()));
  //      agvStatus.AgvStateEnum_Name(agvStatus.agvstate()));
  info << QString("null");
  info << QString("ACTION_SENT");
  info << QString::number(msgBase.netstarttm());
  emit actionInfo(info);
  //  QByteArray ba;
  //  ba.resize(6);
  //  ba[0] = 0xfe;
  //  ba[1] = 0xfe;
  //  ba[2] = (reqTemp.size() + 6) >> 8;
  //  ba[3] = (reqTemp.size() + 6);
  //  ba[4] = 0x00;
  //  ba[5] = 0x01;
  //  std::cout << 1;
  //  std::string tx = ba.toStdString() + reqTemp;
  //  spdlog::info("size:{}", tx.size());
  //  spdlog::info("[{}:{}]Socket Tx:{}",
  //               socket_down.peerAddress().toString().toStdString(),
  //               socket_down.peerPort(),
  //               QString::fromUtf8(QByteArray::fromStdString(tx).toHex(' '))
  //                   .toUpper()
  //                   .toStdString());
  //  spdlog::info("[{}:{}]Socket Tx:{}",
  //               socket_down->peerAddress().toString().toStdString(),
  //               socket_down->peerPort(), tx);
  //  socket_down->write(tx.c_str(), tx.size());
}

// void Agent::actionInfo()

void Agent::sendActionMove() {
  cainiao::AgvActionMessage body;

  cainiao::MessageBase resp;
  resp.set_id(QUuid::createUuid().toString(QUuid::WithoutBraces).toStdString());
  resp.set_messagetypeenum(
      cainiao::MessageBase::MessageTypeEnum::MessageBase_MessageTypeEnum_DATA);
  //  cainiao::
  cainiao::MessageBase::Model *model = new cainiao::MessageBase::Model();
  model->set_path("/URcs/biz/publish/command");
  model->set_modeenum(cainiao::MessageBase::Model::SYNC);
  model->set_typeenum(cainiao::MessageBase::Model::NOTIFY);
  resp.set_allocated_mapping(model);
  //  resp.mutable_mapping()->MergeFrom(model);
  resp.set_resptimeoutmillis(15000);
  //  resp.set_collaborationid(id);
  resp.set_netstarttm(QDateTime::currentDateTimeUtc().toTime_t());
  resp.set_netendtm(0);

  body.mutable_messagebase()->MergeFrom(resp);

  cainiao::AgvActionMessage::AgvAction resp2;
  resp2.set_agvid(agvStatus.agvid());
  resp2.set_traceid(std::to_string(traceCnt++));
  resp2.set_actionid(std::to_string(actionCnt++));//RCS发送给 AGV 的动作指令 ID
  resp2.set_actiontype(cainiao::AgvActionMessage::AgvAction::ACTION_MOVE);//移动指令
  cainiao::AgvActionMessage::AgvAction::ActionPoint resp2_1;
  resp2_1.set_id(2);
  resp2_1.set_originx(agvStatus.pixelx());
  resp2_1.set_originy(agvStatus.pixely());
  resp2_1.set_distance(0);
  resp2_1.set_direction(0);
  resp2.mutable_currentpoint()->MergeFrom(resp2_1);
  google::protobuf::RepeatedPtrField<cainiao::AgvActionMessage::AgvAction::ActionPoint>points;
  cainiao::AgvActionMessage::AgvAction::ActionPoint *resp2_2 =
      resp2.add_points();
  resp2_2->set_id(2);
  resp2_2->set_originx(agvStatus.pixelx());
  resp2_2->set_originy(agvStatus.pixely());
  resp2_2->set_distance(0);//充电桩距离 (mm)
  resp2_2->set_direction(0);//充电桩方向 (EAST:0 、 SOUTH:1 、 WEST:2 、 NORTH:3)

  cainiao::AgvActionMessage::AgvAction::ActionPoint *resp2_3 =
      resp2.add_points();
  resp2_3->set_id(3);
  resp2_3->set_originx(agvStatus.pixelx()+800);
  resp2_3->set_originy(agvStatus.pixely());
  resp2_3->set_distance(0);
  resp2_3->set_direction(0);

//    cainiao::AgvActionMessage::AgvAction::ActionPoint *resp2_4 =
//        resp2.add_points();
//    resp2_4->set_id(3);
//    resp2_4->set_originx(4000.0);
//    resp2_4->set_originy(1600.0);
//    resp2_4->set_distance(0);
//    resp2_4->set_direction(0);
  //  resp2.set_bucketcode("null");
  //  resp2.set_bucketface("1");
  resp2.set_directionid(0);//AGV到达 points 列表最后一个点后的⻋车头朝向 (EAST:0 、 SOUTH:1 、 WEST:2 、 NORTH:3)
  resp2.set_followflag(false);//跟车标志 ,true 表示需要跟车 ,false 表示这不需要跟⻋,预留,不支持;
  resp2.set_workmode(cainiao::AgvActionMessage::AgvAction::REGISTERED);//AGV工作模式，上线
  resp2.set_retrytimes(1);//该指令重试次数
  //  resp2.set_loadheight(300);
  //  resp2.

  body.mutable_agvaction()->MergeFrom(resp2);

  std::string reqTemp;
  //  google::protobuf::util::JsonPrintOptions options;
  //  options.add_whitespace = true;
  //  options.always_print_primitive_fields = true;
  //  options.preserve_proto_field_names = true;

  MessageToJsonString(body, &reqTemp);

  downSocketWrite(enMsgHeaderType::AGV_ACTION, reqTemp);
}

void Agent::linkRCStoP40(int orderId,int missionKey,quint32 pointId,int newX,int newY){

  qDebug()<<"agent: "<<newX<<"---"<<newY<<'\n';

   int errorx;
   int errory;
   errorx = newX - agvStatus.pixelx();
   errory = newY - agvStatus.pixely();

   qDebug()<<"agent___errorx: "<<errorx<<'\n';
   qDebug()<<"agent___errory: "<<errory<<'\n';

  cainiao::AgvActionMessage body;

  cainiao::MessageBase resp;
  resp.set_id(QUuid::createUuid().toString(QUuid::WithoutBraces).toStdString());
  resp.set_messagetypeenum(cainiao::MessageBase::MessageTypeEnum::MessageBase_MessageTypeEnum_DATA);
  //  cainiao::
  cainiao::MessageBase::Model *model = new cainiao::MessageBase::Model();
  model->set_path("/URcs/biz/publish/command");
  model->set_modeenum(cainiao::MessageBase::Model::SYNC);//同步发送
  model->set_typeenum(cainiao::MessageBase::Model::NOTIFY);//通知类消息
  resp.set_allocated_mapping(model);
  //  resp.mutable_mapping()->MergeFrom(model);
  resp.set_resptimeoutmillis(15000);
  //  resp.set_collaborationid(id);
  resp.set_netstarttm(QDateTime::currentDateTimeUtc().toTime_t());
  resp.set_netendtm(0);

  body.mutable_messagebase()->MergeFrom(resp);

  cainiao::AgvActionMessage::AgvAction resp2;
  resp2.set_agvid(agvStatus.agvid());
  resp2.set_traceid(std::to_string(orderId));
  resp2.set_actionid(std::to_string(missionKey));
  resp2.set_actiontype(cainiao::AgvActionMessage::AgvAction::ACTION_MOVE);
  cainiao::AgvActionMessage::AgvAction::ActionPoint resp2_1;
  //int pointID = 1;
  resp2_1.set_id(1);
  resp2_1.set_originx(agvStatus.pixelx());
  resp2_1.set_originy(agvStatus.pixely());
  resp2_1.set_distance(0);
  resp2_1.set_direction(0);
  resp2.mutable_currentpoint()->MergeFrom(resp2_1);

  std::vector<cainiao::AgvActionMessage::AgvAction::ActionPoint> points;
  points.push_back(resp2_1);
  cainiao::AgvActionMessage::AgvAction::ActionPoint resp2_2;
  resp2_2.set_id(pointId);

  if(std::abs(errorx) > std::abs(errory) && std::abs(errorx) > 200){
    resp2_2.set_originx(newX);
    resp2_2.set_originy(agvStatus.pixely());
  }
  else if(std::abs(errory) > std::abs(errorx) && std::abs(errory) > 200){
    resp2_2.set_originx(agvStatus.pixelx());
    resp2_2.set_originy(newY);
  }
  else{
    resp2_2.set_originx(agvStatus.pixelx());
    resp2_2.set_originy(agvStatus.pixely());
  }

  points.push_back(resp2_2);
  resp2.mutable_points()->CopyFrom({points.begin(), points.end()});

  int i;
  if(errory>200) i=3;
  else if(errorx>200) i=0;
  else if(errory<-200) i=1;
  else if(errorx<-200) i=2;
  else  i=4;

  if(i==0 || i==1 || i==2 || i==3) resp2.set_directionid(i);
  if(i==4) resp2.set_directionid(agvStatus.directionid());

  qDebug()<<"directionid: "<<i<<'\n';

  resp2.set_followflag(false);//不需要跟⻋
  resp2.set_workmode(cainiao::AgvActionMessage::AgvAction::REGISTERED);//工作模式：上线
  resp2.set_retrytimes(1);//该指令重试次数
  resp2.set_speedlimit(1);//速度限制为1m/s
  //  resp2.set_loadheight(300);
  //  resp2.

  body.mutable_agvaction()->MergeFrom(resp2);

  std::string reqTemp;

  MessageToJsonString(body, &reqTemp);

  downSocketWrite(enMsgHeaderType::AGV_ACTION, reqTemp);
}

void Agent::linkRCStoP40_1(int orderId,int missionKey,quint32 pointId_1,int newX_1,int newY_1,
                                                      quint32 pointId_2,int newX_2,int newY_2){

  int errorx;
  int errory;
  errorx = newX_2 - newX_1;
  errory = newY_2 - newY_1;
//  qDebug()<<newX_1<<"---"<<newY_1<<"::"<<newX_2<<"---"<<newY_2<<'\n';
//  qDebug()<<"agent___errorx: "<<errorx<<'\n';
//  qDebug()<<"agent___errory: "<<errory<<'\n';

  cainiao::AgvActionMessage body;

  cainiao::MessageBase resp;
  resp.set_id(QUuid::createUuid().toString(QUuid::WithoutBraces).toStdString());
  resp.set_messagetypeenum(cainiao::MessageBase::MessageTypeEnum::MessageBase_MessageTypeEnum_DATA);
  //  cainiao::
  cainiao::MessageBase::Model *model = new cainiao::MessageBase::Model();
  model->set_path("/URcs/biz/publish/command");
  model->set_modeenum(cainiao::MessageBase::Model::SYNC);//同步发送
  model->set_typeenum(cainiao::MessageBase::Model::NOTIFY);//通知类消息
  resp.set_allocated_mapping(model);
  //  resp.mutable_mapping()->MergeFrom(model);
  resp.set_resptimeoutmillis(15000);
  //  resp.set_collaborationid(id);
  resp.set_netstarttm(QDateTime::currentDateTimeUtc().toTime_t());
  resp.set_netendtm(0);

  body.mutable_messagebase()->MergeFrom(resp);

  cainiao::AgvActionMessage::AgvAction resp2;
  resp2.set_agvid(agvStatus.agvid());
  resp2.set_traceid(std::to_string(orderId));
  resp2.set_actionid(std::to_string(missionKey));
  resp2.set_actiontype(cainiao::AgvActionMessage::AgvAction::ACTION_MOVE);
  cainiao::AgvActionMessage::AgvAction::ActionPoint resp2_1;
  //int pointID = 1;
  resp2_1.set_id(pointId_1);
  resp2_1.set_originx(newX_1);
  resp2_1.set_originy(newY_1);
  resp2_1.set_distance(0);
  resp2_1.set_direction(0);
  resp2.mutable_currentpoint()->MergeFrom(resp2_1);

  std::vector<cainiao::AgvActionMessage::AgvAction::ActionPoint> points;
  points.push_back(resp2_1);
  cainiao::AgvActionMessage::AgvAction::ActionPoint resp2_2;
  resp2_2.set_id(pointId_2);

//  if(std::abs(errorx) > std::abs(errory) && std::abs(errorx) > 200){
    resp2_2.set_originx(newX_2);
    resp2_2.set_originy(newY_2);
//  }
//  else if(std::abs(errory) > std::abs(errorx) && std::abs(errory) > 200){
//    resp2_2.set_originx(newX_2);
//    resp2_2.set_originy(newY_2);
//  }
//  else{
//    resp2_2.set_originx(agvStatus.pixelx());
//    resp2_2.set_originy(agvStatus.pixely());
//  }

  points.push_back(resp2_2);
  resp2.mutable_points()->CopyFrom({points.begin(), points.end()});

  int i;
  if(errory>200) i=3;
  else if(errorx>200) i=0;
  else if(errory<-200) i=1;
  else if(errorx<-200) i=2;
  else  i=4;

  if(i==0 || i==1 || i==2 || i==3){
    resp2.set_directionid(i);
  }
  else if(i==4) {
    resp2.set_directionid(agvStatus.directionid());
  }
//  qDebug()<<"directionid: "<<i<<'\n';

  resp2.set_followflag(false);//不需要跟⻋
  resp2.set_workmode(cainiao::AgvActionMessage::AgvAction::REGISTERED);//工作模式：上线
  resp2.set_retrytimes(1);//该指令重试次数
  resp2.set_speedlimit(50);//速度限制为1m/s
  //  resp2.set_loadheight(300);
  //  resp2.

  body.mutable_agvaction()->MergeFrom(resp2);

  std::string reqTemp;

  MessageToJsonString(body, &reqTemp);

  downSocketWrite(enMsgHeaderType::AGV_ACTION, reqTemp);
}

void Agent::sendRCSActionLiftDown(int orderId,int missionKey,quint32 pointId,int newX,int newY,int height) {
  cainiao::AgvActionMessage body;

  cainiao::MessageBase resp;
  resp.set_id(QUuid::createUuid().toString(QUuid::WithoutBraces).toStdString());
  resp.set_messagetypeenum(
      cainiao::MessageBase::MessageTypeEnum::MessageBase_MessageTypeEnum_DATA);
  //  cainiao::
  cainiao::MessageBase::Model *model = new cainiao::MessageBase::Model();
  model->set_path("/URcs/biz/publish/command");
  model->set_modeenum(cainiao::MessageBase::Model::SYNC);
  model->set_typeenum(cainiao::MessageBase::Model::NOTIFY);
  resp.set_allocated_mapping(model);
  //  resp.mutable_mapping()->MergeFrom(model);
  resp.set_resptimeoutmillis(15000);
  //  resp.set_collaborationid(id);
  resp.set_netstarttm(QDateTime::currentDateTimeUtc().toTime_t());
  resp.set_netendtm(0);

  body.mutable_messagebase()->MergeFrom(resp);

  cainiao::AgvActionMessage::AgvAction resp2;
  resp2.set_agvid(agvStatus.agvid());
  resp2.set_traceid(std::to_string(orderId));
  resp2.set_actionid(std::to_string(missionKey));
  resp2.set_actiontype(cainiao::AgvActionMessage::AgvAction::ACTION_LIFT);
  cainiao::AgvActionMessage::AgvAction::ActionPoint resp2_1;
  resp2_1.set_id(pointId);
  resp2_1.set_originx(newX);
  resp2_1.set_originy(newY);
  resp2_1.set_distance(0);
  resp2_1.set_direction(0);
  resp2.mutable_currentpoint()->MergeFrom(resp2_1);

  std::vector<cainiao::AgvActionMessage::AgvAction::ActionPoint> points;
  points.push_back(resp2_1);

  resp2.mutable_points()->CopyFrom({points.begin(), points.end()});

  resp2.set_directionid(0);
  resp2.set_nostrongcheck(false);
  resp2.set_followflag(false);
  resp2.set_workmode(cainiao::AgvActionMessage::AgvAction::REGISTERED);
  resp2.set_retrytimes(1);
  resp2.set_loadheight(height);
  //  resp2.

  body.mutable_agvaction()->MergeFrom(resp2);

  std::string reqTemp;

  MessageToJsonString(body, &reqTemp);

  downSocketWrite(enMsgHeaderType::AGV_ACTION, reqTemp);
}


void Agent::sendActionMoveBackword(int grids) {
  cainiao::AgvActionMessage body;

  cainiao::MessageBase resp;
  resp.set_id(QUuid::createUuid().toString(QUuid::WithoutBraces).toStdString());
  resp.set_messagetypeenum(
      cainiao::MessageBase::MessageTypeEnum::MessageBase_MessageTypeEnum_DATA);
  //  cainiao::
  cainiao::MessageBase::Model *model = new cainiao::MessageBase::Model();
  model->set_path("/URcs/biz/publish/command");
  model->set_modeenum(cainiao::MessageBase::Model::SYNC);
  model->set_typeenum(cainiao::MessageBase::Model::NOTIFY);
  resp.set_allocated_mapping(model);
  //  resp.mutable_mapping()->MergeFrom(model);
  resp.set_resptimeoutmillis(15000);
  //  resp.set_collaborationid(id);
  resp.set_netstarttm(QDateTime::currentDateTimeUtc().toTime_t());
  resp.set_netendtm(0);

  body.mutable_messagebase()->MergeFrom(resp);

  cainiao::AgvActionMessage::AgvAction resp2;
  resp2.set_agvid(agvStatus.agvid());
  resp2.set_traceid(std::to_string(traceCnt++));
  resp2.set_actionid(std::to_string(actionCnt++));
  resp2.set_actiontype(cainiao::AgvActionMessage::AgvAction::ACTION_BACKWARD);//AGV倒车
  cainiao::AgvActionMessage::AgvAction::ActionPoint resp2_1;
  int pointID = 1;
  resp2_1.set_id(pointID++);
  resp2_1.set_originx(agvStatus.pixelx());
  resp2_1.set_originy(agvStatus.pixely());
  resp2_1.set_distance(0);
  resp2_1.set_direction(0);
  resp2.mutable_currentpoint()->MergeFrom(resp2_1);

  std::vector<cainiao::AgvActionMessage::AgvAction::ActionPoint> points;
  points.push_back(resp2_1);
  for (int i = 0; i < grids; ++i) {
    cainiao::AgvActionMessage::AgvAction::ActionPoint resp2_2;
    resp2_2.set_id(pointID++);
    switch (agvStatus.directionid())  //(EAST:0 、 SOUTH:1 、 WEST:2 、 NORTH:3)
    {
      case 0: {
        resp2_2.set_originx(agvStatus.pixelx() - 800 * (i + 1));
        resp2_2.set_originy(agvStatus.pixely());
      } break;
      case 1: {
        resp2_2.set_originx(agvStatus.pixelx());
        resp2_2.set_originy(agvStatus.pixely() + 800 * (i + 1));
      } break;
      case 2: {
        resp2_2.set_originx(agvStatus.pixelx() + 800 * (i + 1));
        resp2_2.set_originy(agvStatus.pixely());
      } break;
      case 3: {
        resp2_2.set_originx(agvStatus.pixelx());
        resp2_2.set_originy(agvStatus.pixely() - 800 * (i + 1));
      } break;
      default:
        spdlog::error("Unexpected directionID:{}", agvStatus.directionid());
        continue;
        break;
    }
    points.push_back(resp2_2);
  }
  resp2.mutable_points()->CopyFrom({points.begin(), points.end()});

  //  google::protobuf::RepeatedPtrField<
  //      cainiao::AgvActionMessage::AgvAction::ActionPoint>
  //      points;
  //  cainiao::AgvActionMessage::AgvAction::ActionPoint *resp2_2 =
  //      resp2.add_points();
  //  resp2_2->set_id(1);
  //  resp2_2->set_originx(agvStatus.pixelx());
  //  resp2_2->set_originy(agvStatus.pixely());
  //  resp2_2->set_distance(0);
  //  resp2_2->set_direction(0);

  //  cainiao::AgvActionMessage::AgvAction::ActionPoint *resp2_3 =
  //      resp2.add_points();
  //  resp2_3->set_id(2);
  //  resp2_3->set_originx(agvStatus.pixelx() + 800);
  //  resp2_3->set_originy(agvStatus.pixely());
  //  resp2_3->set_distance(0);
  //  resp2_3->set_direction(0);

  //  cainiao::AgvActionMessage::AgvAction::ActionPoint *resp2_4 =
  //      resp2.add_points();
  //  resp2_4->set_id(3);
  //  resp2_4->set_originx(5800.0);
  //  resp2_4->set_originy(6400.0);
  //  resp2_4->set_distance(0);
  //  resp2_4->set_direction(0);
  //  resp2.set_bucketcode("null");
  //  resp2.set_bucketface("1");
  resp2.set_directionid(agvStatus.directionid());
  resp2.set_followflag(false);
  resp2.set_workmode(cainiao::AgvActionMessage::AgvAction::REGISTERED);
  resp2.set_retrytimes(1);
  //  resp2.set_loadheight(300);
  //  resp2.

  body.mutable_agvaction()->MergeFrom(resp2);

  std::string reqTemp;
  //  google::protobuf::util::JsonPrintOptions options;
  //  options.add_whitespace = true;
  //  options.always_print_primitive_fields = true;
  //  options.preserve_proto_field_names = true;

  MessageToJsonString(body, &reqTemp);

  downSocketWrite(enMsgHeaderType::AGV_ACTION, reqTemp);
}

void Agent::sendActionMoveForword(int grids) {
  cainiao::AgvActionMessage body;

  cainiao::MessageBase resp;
  resp.set_id(QUuid::createUuid().toString(QUuid::WithoutBraces).toStdString());
  resp.set_messagetypeenum(cainiao::MessageBase::MessageTypeEnum::MessageBase_MessageTypeEnum_DATA);
  //  cainiao::
  cainiao::MessageBase::Model *model = new cainiao::MessageBase::Model();
  model->set_path("/URcs/biz/publish/command");
  model->set_modeenum(cainiao::MessageBase::Model::SYNC);
  model->set_typeenum(cainiao::MessageBase::Model::NOTIFY);
  resp.set_allocated_mapping(model);
  //  resp.mutable_mapping()->MergeFrom(model);
  resp.set_resptimeoutmillis(15000);
  //  resp.set_collaborationid(id);
  resp.set_netstarttm(QDateTime::currentDateTimeUtc().toTime_t());
  resp.set_netendtm(0);

  body.mutable_messagebase()->MergeFrom(resp);

  cainiao::AgvActionMessage::AgvAction resp2;
  resp2.set_agvid(agvStatus.agvid());
  resp2.set_traceid(std::to_string(traceCnt++));
  resp2.set_actionid(std::to_string(actionCnt++));
  resp2.set_actiontype(cainiao::AgvActionMessage::AgvAction::ACTION_MOVE);
  cainiao::AgvActionMessage::AgvAction::ActionPoint resp2_1;
  int pointID = 1;
  resp2_1.set_id(pointID++);
  resp2_1.set_originx(agvStatus.pixelx());
  resp2_1.set_originy(agvStatus.pixely());
  resp2_1.set_distance(0);
  resp2_1.set_direction(0);
  resp2.mutable_currentpoint()->MergeFrom(resp2_1);

  std::vector<cainiao::AgvActionMessage::AgvAction::ActionPoint> points;
  points.push_back(resp2_1);
  for (int i = 0; i < grids; ++i) {
    cainiao::AgvActionMessage::AgvAction::ActionPoint resp2_2;
    resp2_2.set_id(pointID++);
    switch (agvStatus.directionid()) {
      case 0: {
        resp2_2.set_originx(agvStatus.pixelx() + 800 * (i + 1));
        resp2_2.set_originy(agvStatus.pixely());
      } break;
      case 1: {
        resp2_2.set_originx(agvStatus.pixelx());
        resp2_2.set_originy(agvStatus.pixely() - 800 * (i + 1));
      } break;
      case 2: {
        resp2_2.set_originx(agvStatus.pixelx() - 800 * (i + 1));
        resp2_2.set_originy(agvStatus.pixely());
      } break;
      case 3: {
        resp2_2.set_originx(agvStatus.pixelx());
        resp2_2.set_originy(agvStatus.pixely() + 800 * (i + 1));
      } break;
      default:
        spdlog::error("Unexpected directionID:{}", agvStatus.directionid());
        continue;
        break;
    }
    points.push_back(resp2_2);
  }
  resp2.mutable_points()->CopyFrom({points.begin(), points.end()});

  //  google::protobuf::RepeatedPtrField<
  //      cainiao::AgvActionMessage::AgvAction::ActionPoint>
  //      points;
  //  cainiao::AgvActionMessage::AgvAction::ActionPoint *resp2_2 =
  //      resp2.add_points();
  //  resp2_2->set_id(1);
  //  resp2_2->set_originx(agvStatus.pixelx());
  //  resp2_2->set_originy(agvStatus.pixely());
  //  resp2_2->set_distance(0);
  //  resp2_2->set_direction(0);

  //  cainiao::AgvActionMessage::AgvAction::ActionPoint *resp2_3 =
  //      resp2.add_points();
  //  resp2_3->set_id(2);
  //  resp2_3->set_originx(agvStatus.pixelx() + 800);
  //  resp2_3->set_originy(agvStatus.pixely());
  //  resp2_3->set_distance(0);
  //  resp2_3->set_direction(0);

  //  cainiao::AgvActionMessage::AgvAction::ActionPoint *resp2_4 =
  //      resp2.add_points();
  //  resp2_4->set_id(3);
  //  resp2_4->set_originx(5800.0);
  //  resp2_4->set_originy(6400.0);
  //  resp2_4->set_distance(0);
  //  resp2_4->set_direction(0);
  //  resp2.set_bucketcode("null");
  //  resp2.set_bucketface("1");
  resp2.set_directionid(agvStatus.directionid());
  resp2.set_followflag(false);
  resp2.set_workmode(cainiao::AgvActionMessage::AgvAction::REGISTERED);
  resp2.set_retrytimes(1);
  resp2.set_speedlimit(20000);
  //  resp2.set_loadheight(300);
  //  resp2.

  body.mutable_agvaction()->MergeFrom(resp2);

  std::string reqTemp;
  //  google::protobuf::util::JsonPrintOptions options;
  //  options.add_whitespace = true;
  //  options.always_print_primitive_fields = true;
  //  options.preserve_proto_field_names = true;

  MessageToJsonString(body, &reqTemp);

  downSocketWrite(enMsgHeaderType::AGV_ACTION, reqTemp);
}

void Agent::genMsgBase(cainiao::MessageBase &base,
                       cainiao::MessageBase::MessageTypeEnum msgType,
                       std::string path,
                       cainiao::MessageBase::Model::TypeEnum modelType) {
  //  cainiao::MessageBase resp;
  base.set_id(QUuid::createUuid().toString(QUuid::WithoutBraces).toStdString());
  base.set_messagetypeenum(msgType);
  //  cainiao::
  cainiao::MessageBase::Model *mapping = new cainiao::MessageBase::Model();
  mapping->set_path(path);
  mapping->set_modeenum(cainiao::MessageBase::Model::SYNC);
  mapping->set_typeenum(modelType);
  base.set_allocated_mapping(mapping);
  //  resp.mutable_mapping()->MergeFrom(model);
  base.set_resptimeoutmillis(15000);
  //  resp.set_collaborationid(id);
  base.set_netstarttm(QDateTime::currentDateTimeUtc().toMSecsSinceEpoch());
  //  QDateTime::currentMSecsSinceEpoch();
  base.set_netendtm(0);
}

void Agent::sendActionClearErrors() {
  cainiao::AgvActionMessage body;

  cainiao::MessageBase msgBase;
  genMsgBase(
      msgBase,
      cainiao::MessageBase::MessageTypeEnum::MessageBase_MessageTypeEnum_DATA,
      "/URcs/biz/publish/command", cainiao::MessageBase::Model::NOTIFY);
  body.mutable_messagebase()->MergeFrom(msgBase);

  cainiao::AgvActionMessage::AgvAction resp2;
  resp2.set_agvid(agvStatus.agvid());
  resp2.set_actiontype(
      cainiao::AgvActionMessage::AgvAction::ACTION_CLEAR_ERROR);
  resp2.set_directionid(0);
  resp2.set_followflag(false);
  resp2.set_workmode(cainiao::AgvActionMessage::AgvAction::OFFLINE);
  resp2.set_retrytimes(0);
  //  resp2.set_loadheight(300);
  //  resp2.

  body.mutable_agvaction()->MergeFrom(resp2);

  std::string reqTemp;
  //  google::protobuf::util::JsonPrintOptions options;
  //  options.add_whitespace = true;
  //  options.always_print_primitive_fields = true;
  //  options.preserve_proto_field_names = true;

  MessageToJsonString(body, &reqTemp);

  downSocketWrite(enMsgHeaderType::AGV_ACTION, reqTemp);
}

Agent::~Agent() {
  //  timer->stop();
  //  timer->deleteLater();
}
void Agent::onCreateTimer() {
//  QTimer::singleShot(10000, this, [=] {
//    spdlog::info("later");
//    this->deleteLater();
//  });
#if 1
  //    qDebug() <<"child THREAD IS：" <<QThread::currentThreadId();
  spdlog::get("rps_logger")->info("Create timer...");
  timer = new QTimer(this);
  connect(timer, &QTimer::timeout, this, &Agent::timer_slot);
  //  timer->start(2000);

  socket_down = new QTcpSocket(this);
  //  qDebug() << sktDescriptor;
  socket_down->setSocketDescriptor(sktDescriptor);

  ipAddr = socket_down->peerAddress().toString();
  ipPort = QString::number(socket_down->peerPort());

  connect(socket_down, &QTcpSocket::readyRead, this, [=] {
    //    if () return;
    //    qDebug() << "dsa";
    bool break_flag = true;
    while (socket_down->bytesAvailable() > 0 && break_flag) {
      switch (unpackingStatus) {
        case unpackingEnum::UNPACKING_WAITING_PREFIX_1:
          if (socket_down->read(1).at(0) == (char)0xFE) {
            unpackingStatus = unpackingEnum::UNPACKING_WAITING_PREFIX_2;
          } else {
            spdlog::warn("UNPACKING_WAITING_PREFIX_1");
          }
          break;
        case unpackingEnum::UNPACKING_WAITING_PREFIX_2:
          if (socket_down->read(1).at(0) == (char)0xFE) {
            unpackingStatus = unpackingEnum::UNPACKING_WAITING_HEADER;
          } else {
            spdlog::warn("UNPACKING_WAITING_PREFIX_2");
          }
          break;
        case unpackingEnum::UNPACKING_WAITING_HEADER:
          if (socket_down->bytesAvailable() >= 4) {
            auto rx = socket_down->read(4);
            HeaderMsg *msg__ = (HeaderMsg *)rx.data();
            header_msg.len = qToBigEndian(msg__->len);
            header_msg.type = qToBigEndian(msg__->type);
            if (!debug_vector.contains(header_msg.type)) {
              spdlog::info("[{}:{}]Socket Rx Header:({},{})",
                           socket_down->peerAddress().toString().toStdString(),
                           socket_down->peerPort(), header_msg.len,
                           header_msg.type);
            }
            unpackingStatus = unpackingEnum::UNPACKING_WAITING_BODY;
          } else {
            spdlog::warn("UNPACKING_WAITING_HEADER");
            break_flag = false;
          }
          break;
        case unpackingEnum::UNPACKING_WAITING_BODY:
          if (socket_down->bytesAvailable() >= header_msg.len - 6) {
            auto rx = socket_down->read(header_msg.len - 6);
            if (!debug_vector.contains(header_msg.type)) {
              spdlog::info("[{}:{}]Socket Rx:{}",
                           socket_down->peerAddress().toString().toStdString(),
                           socket_down->peerPort(), rx.toStdString());
            }
            messageProcess(enMsgHeaderType(header_msg.type), rx);
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
    // 注意收发两端文本要使用对应的编解码
    //    socket_down.read()
    auto rx = socket_down->readAll();
    //    rx.toHex();

    if (this->protocolDecoding(rx)) {
      return;
      ;
    }
    const QString recv_text = QString::fromUtf8(rx.toHex(' ')).toUpper();
    spdlog::info("[{}:{}]Socket Rx:{}",
                 socket_down->peerAddress().toString().toStdString(),
                 socket_down->peerPort(), recv_text.toStdString());
    //    qDebug() << "om";
    //    spdlog::info("protocolDecoding");

    //    qDebug() << "om2";
    //    spdlog::info("[{}:{}]Socket Rx size:{}",
    //                 socket_down.peerAddress().toString().toStdString(),
    //                 socket.peerPort(), rx.size());
    //    spdlog::info("[{}:{}]Socket Rxt:{}",
    //                 socket_down.peerAddress().toString().toStdString(),
    //                 socket.peerPort(), rx.toStdString());
    //    cainiao::SessionRegisterInfoMessage smsg;
    //    //    smsg.sessionregisterinfo().;
    //    google::protobuf::util::JsonStringToMessage(rx.right(398).toStdString(),
    //                                                &smsg);
    //    //    sms
    //    //    if (smsg.ParseFromString(rx.right(398).toStdString())) {
    //    //      spdlog::info("parse ok");
    //    //    } else {
    //    //      spdlog::info("parse error");
    //    //    }
    //    std::string reqTemp;
    //    MessageToJsonString(smsg, &reqTemp);
    //    spdlog::info("Josndecode Rx:{}", reqTemp);
  });
//  connect(socket_down, &QAbstractSocket::errorOccurred, this, [=] {
//    spdlog::error("[{}:{}]Socket Error:{}",
//                  socket_down->peerAddress().toString().toStdString(),
//                  socket_down->peerPort(),
//                  socket_down->errorString().toStdString());
//  });
  connect(socket_down, &QTcpSocket::disconnected, this, [=] {
    spdlog::error("[{}:{}]Socket Disconnected.",
                  socket_down->peerAddress().toString().toStdString(),
                  socket_down->peerPort());
    //    emit downStreamClosed();
    this->deleteLater();
  });
  emit agentCreated(socket_down->peerAddress().toString() + ":" +
                    QString::number(socket_down->peerPort()));
#endif
}

void Agent::timer_slot() {
  sendHeatbeatResponse();
  //  QStringList info;
  //  info << QString::number(actionCnt++);
  //  info << QString::fromStdString(
  //      agvStatus.AgvStateEnum_Name(agvStatus.agvstate()));
  //  info << QString("point");
  //  info << QString("ACTION_RECEIVED");
  //  info << QString::number(agvStatus.timestamp());
  //  emit actionInfo(info);
  //  qDebug() << "child THREAD IS：" << QThread::currentThreadId();
  //  cnt++;
  //  spdlog::get("rps_logger")
  //  ->info("THREAD({}) timer trigger{}", QThread::currentThreadId(), cnt);
  //  if (cnt == 10) {
  //    sendActionLift();
  //  }
}

void Agent::downSocketWrite(enMsgHeaderType type, std::string body) {
  QByteArray ba;
  ba.resize(6);
  ba[0] = 0xfe;
  ba[1] = 0xfe;
  ba[2] = (body.size() + 6) >> 8;
  ba[3] = (body.size() + 6);
  ba[4] = 0x00;
  ba[5] = (int)type;
  //  std::cout << 1;
  std::string tx = ba.toStdString() + body;
  //  spdlog::info("size:{}", tx.size());
  //  spdlog::info("[{}:{}]Socket Tx:{}",
  //               socket_down.peerAddress().toString().toStdString(),
  //               socket_down.peerPort(),
  //               QString::fromUtf8(QByteArray::fromStdString(tx).toHex(' '))
  //                   .toUpper()
  //                   .toStdString());
  //  spdlog::info("[{}:{}]Socket Tx:{}",
  //               socket_down->peerAddress().toString().toStdString(),
  //               socket_down->peerPort(), tx);
  socket_down->write(tx.c_str(), tx.size());
  emit downWrite(QDateTime::currentDateTime().toString("[hh:mm:ss.zzz]"),
                 QByteArray::fromStdString(tx));
}
