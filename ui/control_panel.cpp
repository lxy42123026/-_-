#include "control_panel.h"

#include <google/protobuf/util/json_util.h>

#include <QApplication>
#include <QDebug>
#include <QDesktopWidget>
#include <QJsonDocument>
#include <QJsonObject>
#include <QScreen>
#include <QScrollBar>
#include <QDateTime>
#include <QThread>

#include "ui_control_panel.h"
//#include "virtual_robot.h"

ControlPanel::ControlPanel(QWidget *parent, Agent *agent)
    : QMainWindow(parent), agent(agent), ui(new Ui::ControlPanel) {
  ui->setupUi(this);
  setWindowTitle(QString("Control Panel"));
  //  int currentScreen =agent
  //      QApplication::desktop()->screenNumber(this);  // 程序所在的屏幕编号
  //  QGuiApplication::screens(currentScreen);
  QRect rect = QGuiApplication::primaryScreen()->geometry();  //
  this->resize(rect.width() * 0.75, rect.height() * 0.75);
  this->move((rect.width() - this->width()) / 2,
             (rect.height() - this->height()) / 2);
  ui->tabWidget->setCurrentIndex(0);
  //  程序所在屏幕尺寸 w.move((rect.width() - w.width()) / 2,
  //         (rect.height() - w.height()) / 2);  // 移动到所在屏幕中间

  std::string str;
  google::protobuf::util::JsonPrintOptions options;
  options.add_whitespace = true;
  options.always_print_primitive_fields = true;
  options.preserve_proto_field_names = true;
  MessageToJsonString(agv, &str, options); //将agv对象转换为JSON字符串，并将结果赋值给str变量
  ui->textBrowser->setText(QString::fromStdString(str));

  connect(agent, &Agent::rtStatus, this, [=](QString str) {
    ui->textBrowser->setText(str);
    google::protobuf::util::JsonStringToMessage(str.toStdString(), &agv);//解析str,存入agv中
    ui->label_id->setText(QString::fromStdString(agv.agvid()));
    updateRtLabel();    //心跳闪烁
  });

  connect(agent, &Agent::heartBeat, this, [=]() { updateAliveLabel(); });
  im_alive_on.load(":/resources/icons/alive_on.png");
  im_alive_off.load(":/resources/icons/alive_off.png");

  im_rt_on.load(":/resources/icons/rt_on.png");
  im_rt_off.load(":/resources/icons/rt_off.png");
  timer = new QTimer(this);

  connect(timer, &QTimer::timeout, this, [=] {
    updateRtLabel();
    //    updateAliveLabel();
  });
  //  timer->start(200);
  // 图片路径可以通过右击工程的图片获取this
  //  im_alive_on.load(":/resources/icons/alive_on.png");
  //  lb->setPixmap(QPixmap::fromImage(im));

  // actionModel
  actionModel = new QStandardItemModel();

  actionModel->setColumnCount(5);
  actionModel->setHeaderData(0, Qt::Horizontal, "action_id");
  actionModel->setHeaderData(1, Qt::Horizontal, "action_type");
  actionModel->setHeaderData(2, Qt::Horizontal, "param");
  actionModel->setHeaderData(3, Qt::Horizontal, "result");
  actionModel->setHeaderData(4, Qt::Horizontal, "timestamp");

  ui->tableView->setModel(actionModel);//将 actionModel 设置为表格视图的数据模型，用于展示数据
  ui->tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);//设置表格视图的水平表头根据内容自动调整大小
  ui->tableView->horizontalHeader()->setStretchLastSection(true);//将表格视图的最后一列自动拉伸以填充剩余空间
  ui->tableView->setAlternatingRowColors(true);//设置表格视图交替行的背景颜色，以增加可读性
  ui->tableView->setEditTriggers(QAbstractItemView::NoEditTriggers);//禁止编辑表格视图中的内容
  ui->tableView->show();

  //  int row = actionModel->rowCount();
  //  actionModel->setItem(row, 0, new QStandardItem(QString("%1").arg(1)));
  //  actionModel->setItem(row, 1, new QStandardItem(QString("%1").arg(2)));
  //  actionModel->setItem(row, 2, new QStandardItem(QString("%1").arg(3)));x
  //  actionModel->setItem(row, 3, new QStandardItem(QString("%1").arg(4)));
  //  actionModel->setItem(row, 4, new QStandardItem("5"));
  connect(ui->move_btn,         &QPushButton::clicked,    agent, &Agent::sendActionMove);
  //connect(ui->lift_btn,         &QPushButton::clicked,    agent, &Agent::sendActionLift);
  connect(ui->charge_btn,       &QPushButton::clicked,    agent, &Agent::sendActionCharge);
  connect(ui->uncharge_btn,     &QPushButton::clicked,    agent, &Agent::sendActionUncharge);
  connect(ui->btn_clear_error,  &QPushButton::clicked,    agent, &Agent::sendActionClearErrors);
  //  connect(ui->turn_btn_absolute, &QPushButton::clicked, agent,&Agent::sendActionTurn);
  connect(this, &ControlPanel::putHeight,    agent, &Agent::sendActionPut);
  connect(this, &ControlPanel::liftHeight,   agent, &Agent::sendActionLift);
  connect(this, &ControlPanel::turnDirection,agent, &Agent::sendActionTurn);
  connect(this, &ControlPanel::moveForword,  agent, &Agent::sendActionMoveForword);
  connect(this, &ControlPanel::moveBackword, agent, &Agent::sendActionMoveBackword);
  connect(agent, &Agent::actionInfo, this, [=](QStringList strList) {
    int row = actionModel->rowCount();//获取 actionModel 模型已有的行数，用于确定新行的插入位置
    actionModel->setItem(row, 0, new QStandardItem(strList.at(0)));//在第 row 行的第一列中添加一个 QStandardItem 对象，其文本内容为 strList 列表中的第一项。
    actionModel->setItem(row, 1, new QStandardItem(strList.at(1)));
    actionModel->setItem(row, 2, new QStandardItem(strList.at(2)));
    actionModel->setItem(row, 3, new QStandardItem(strList.at(3)));
    QString tmp = QDateTime::fromMSecsSinceEpoch(strList.at(4).toLongLong())
                      .toString("hh:mm:ss.zzz");//将 strList 列表中的第五项转换成 QDateTime 对象，并将其格式化为 "hh:mm:ss.zzz" 的字符串格式。
    actionModel->setItem(row, 4, new QStandardItem(tmp));
    qDebug() << strList;
    ui->tableView->scrollToBottom();//滚动条到底部
  });
}

ControlPanel::~ControlPanel() { delete ui; }

void ControlPanel::updateAliveLabel() {
  ui->label_alive->setPixmap(QPixmap::fromImage(im_alive_on));
  QTimer::singleShot(200, this, [=] {
    ui->label_alive->setPixmap(QPixmap::fromImage(im_alive_off));
  });
}

void ControlPanel::updateRtLabel() {
  rt = !rt;
  if (rt) {
    ui->label_rt->setPixmap(QPixmap::fromImage(im_rt_on));
  } else {
    ui->label_rt->setPixmap(QPixmap::fromImage(im_rt_off));
  }
}

// void ControlPanel::on_pushButton_4_clicked(bool checked) {}

void ControlPanel::on_lift_btn_clicked(){
  emit liftHeight(ui->lift_height->value());
}

void ControlPanel::on_put_btn_clicked() {
  emit putHeight(ui->put_height->value());
}

void ControlPanel::on_turn_btn_absolute_clicked() {
  emit turnDirection(ui->turn_absolute->value());
}

void ControlPanel::on_move_btn_forword_clicked() {
  //发射moveForword()的信号
  emit moveForword(ui->move_forward->value());
  //QTimer::singleShot(2000, this, SLOT(zhuanquanquan()));
}

void ControlPanel::on_backward_btn_clicked() {
  //emit moveBackword(ui->backward_grid->value());
}

void ControlPanel::zhuanquanquan(){
  emit turnDirection(3);
}
void ControlPanel::zhuanquanquan1(){
  emit moveForword(1);
}
void ControlPanel::zhuanquanquan2(){
  emit turnDirection(1);
}

void ControlPanel::on_task_clicked()
{
  emit moveForword(1);
//  QTimer::singleShot(3000, this, SLOT(zhuanquanquan2()));
}

