#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTcpServer>
#include <QTcpSocket>

#include "agent.h"
#include "control_panel.h"
#include "robot_proxy_server.h"
#include "virtual_panel.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
  Q_OBJECT

 public:
  MainWindow(QWidget *parent = nullptr, RobotProxyServer *rps = nullptr);
  ~MainWindow();

 private slots:
  void on_startRpsBtn_clicked(bool checked);
  // private
  //  void on_liftBtn_clicked();

  void on_openCtrlPanel_clicked(bool checked);

  void on_refresh_btn_clicked();

  void on_pushButton_clicked();

  signals:
  void lift();

 private:
  // 初始化server操作
  //  void initServer();
  //  // close server
  //  void closeServer();
  //  // 更新当前状态
  //  void updateState();

  //  // 初始化client操作
  //  void initClient();
  //  // 更新当前状态
  //  void updateState1();

 private:
  Ui::MainWindow *ui;
  //  QTcpServer *server;
  // 存储已连接的socket对象
  //  QList<QTcpSocket *> clientList;
  //  QTcpSocket *upclient;

  RobotProxyServer *rps;
  ControlPanel *panel;
  QList<Agent *> agentList;
  vir::VirtualRobot *virtualBot;
};
#endif  // MAINWINDOW_H
