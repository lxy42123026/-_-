#include "virtual_panel.h"
#include "virtual_robot.h"

#include <google/protobuf/util/json_util.h>

#include <QApplication>
#include <QDebug>
#include <QDesktopWidget>
#include <QScreen>
#include <QThread>

#include "ui_virtual_panel.h"

VirtualPanel::VirtualPanel(QWidget *parent, vir::VirtualRobot *virtualBot,Agent *agent)
    : QMainWindow(parent), virtualBot(virtualBot),agent(agent), ui(new Ui::VirtualPanel) {
  ui->setupUi(this);
  QRect rect = QGuiApplication::primaryScreen()->geometry();  //
  this->resize(rect.width() * 0.75, rect.height() * 0.75);
  this->move((rect.width() - this->width()) / 2,
             (rect.height() - this->height()) / 2);

  timer1 = new QTimer;

  connect(timer1,&QTimer::timeout,this,&VirtualPanel::set_textBrowser);

  connect(agent, &Agent::rtStatus,this, [=](QString str) {
      ui->textBrowser_3->setText(str);
      ui->textBrowser_3->setFont(QFont("Arial",15));
      google::protobuf::util::JsonStringToMessage(str.toStdString(), &agv_1);
      ui->label_id->setText(QString::fromStdString(agv_1.agvid()));
      ui->progressBar_1->setValue(agv_1.power());
      updateRtLabel();
      virtualBot->linkP40_pos(agv_1.pixelx(),agv_1.pixely(),agv_1.directionid(),agv_1.power(),agv_1.speed(),agv_1.loadheight());
  });

  connect(agent, &Agent::heartBeat, this, [=]() { updateAliveLabel(); });
  im_alive_on.load(":/resources/icons/alive_on.png");
  im_alive_off.load(":/resources/icons/alive_off.png");

  im_rt_on.load(":/resources/icons/rt_on.png");
  im_rt_off.load(":/resources/icons/rt_off.png");

  connect(virtualBot,&vir::VirtualRobot::updateP40pos,agent,&Agent::linkRCStoP40);
  connect(virtualBot,&vir::VirtualRobot::sendAimToP40,agent,&Agent::linkRCStoP40_1);
  connect(virtualBot,&vir::VirtualRobot::doRcsAction,agent,&Agent::sendRCSActionLiftDown);
}

VirtualPanel::~VirtualPanel() { delete ui; }

void VirtualPanel::on_connectBtn_clicked(bool checked) {
  if (virtualBot == nullptr) return;
  if (checked) {
    virtualBot->initConnection(ui->lineEdit->text(), ui->spinBox->value());
  } else {
    virtualBot->deInitConnection();
  }
}

void VirtualPanel::updateAliveLabel() {
  ui->label->setPixmap(QPixmap::fromImage(im_alive_on));
  QTimer::singleShot(200, this, [=] {
      ui->label->setPixmap(QPixmap::fromImage(im_alive_off));
  });
}

void VirtualPanel::updateRtLabel() {
  rt = !rt;
  if (rt) {
    ui->label_2->setPixmap(QPixmap::fromImage(im_rt_on));
  } else {
    ui->label_2->setPixmap(QPixmap::fromImage(im_rt_off));
  }
}

void VirtualPanel::on_sendRgBtn_clicked() {
  if (virtualBot != nullptr) {
    virtualBot->sendRegister();
  }
}

void VirtualPanel::on_sendStBtn_clicked()
{
    virtualBot->sendStatus();
    timer1->start(1000);
    virtualBot->startQTimer();
}

void VirtualPanel::set_textBrowser()
{
    std::string str_state;
    std::string str_task;
    google::protobuf::util::JsonPrintOptions options;
    options.add_whitespace = true;
    options.always_print_primitive_fields = true;
    options.preserve_proto_field_names = true;
    MessageToJsonString(virtualBot->ImrSta, &str_state, options);
    MessageToJsonString(virtualBot->task, &str_task, options);
    ui->textBrowser->setFont(QFont("Arial",13));
    ui->textBrowser->insertPlainText("— — — — — AT：" + QString::fromStdString(str_task)+"**********************************************************"+ '\n');
    ui->textBrowser->insertPlainText("^ ^ ^ ^ ^ ST：" + QString::fromStdString(str_state)+"**********************************************************"+ '\n');
    ui->textBrowser_2->insertPlainText(virtualBot->log_Sta);

    ui->textBrowser->moveCursor(ui->textBrowser->textCursor().End); //进度条显示在最下方
    ui->textBrowser_2->moveCursor(QTextCursor::End);                //进度条最底端
}

void VirtualPanel::on_stopSdBtn_clicked()
{
    virtualBot->stopQTimer();
    timer1->stop();
}


void VirtualPanel::on_clearButton_clicked()
{
    ui->textBrowser->clear();
    ui->textBrowser_2->clear();
}


void VirtualPanel::on_settingBtn_clicked()
{
    //virtualBot->updatePos(3200, 800);
    virtualBot->initValue( ui->spinBox_2->value(), ui->spinBox_3->value(), ui->spinBox_4->value(), ui->spinBox_5->value());
}

//void VirtualPanel::on_pushButton_clicked()
//{
//    virtualBot->updatePos_1(1,1,2,agv_1.pixelx(), agv_1.pixely(),3,agv_1.pixelx()+800,agv_1.pixely());
////    QThread::msleep(1000);
//    virtualBot->updatePos_1(1,2,3,agv_1.pixelx()+800, agv_1.pixely(),4,agv_1.pixelx()+1600,agv_1.pixely());
////    QThread::msleep(1000);
//    virtualBot->updatePos_1(1,3,4,agv_1.pixelx()+1600, agv_1.pixely(),5,agv_1.pixelx()+2400,agv_1.pixely());
////    while(!((std::abs(agv_1.pixelx()-5600) <= 20) && (std::abs(agv_1.pixely()-800) <= 20))){
////    qDebug()<<std::abs(agv_1.pixelx()-5600)<<"-------"<<std::abs(agv_1.pixely()-800)<<endl;
////        std::this_thread::sleep_for(std::chrono::seconds(1));
////    }
//    virtualBot->updateHeight(1,4,5,agv_1.pixelx()+2400, agv_1.pixely(),500);
//}

