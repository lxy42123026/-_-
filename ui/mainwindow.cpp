#include "mainwindow.h"

#include <QDebug>

#include "./ui_mainwindow.h"

// #include "spdlog/spdlog.h"
//  #include "spdlog/sinks/qt_sinks.h"

MainWindow::MainWindow(QWidget *parent, RobotProxyServer *rps)
    : QMainWindow(parent), ui(new Ui::MainWindow), rps(rps) {
  ui->setupUi(this);
  setWindowState(Qt::WindowMaximized);
  setWindowTitle(QString("Robot Proxy Server"));
  ui->splitter->setStretchFactor(0, 1);
  ui->splitter->setStretchFactor(1, 1);
  connect(rps, &RobotProxyServer::agentUpdate, this, [=](bool create, Agent *agt) {
        if (create) {
          if (!agentList.contains(agt)) {
            agentList.append(agt);
            ui->agentComboBox->addItem(agt->getAgentAddrAndPort() + "(" +
                                       agt->getAgentName() + ")");
            if (ui->agentComboBox->count() == 1) {
              //      qDebug() << "123";
              connect(agt, &Agent::downWrite, this,[=](QString timestramp, QByteArray data) {
                    ui->downTextBrowser->append(
                        timestramp + "[Tx]:" + "<font color=\"#FF0000\">" +
                        QString::fromUtf8(data.left(6).toHex(' ')).toUpper() +
                        QString::fromUtf8(data.right(data.size() - 6)) +
                        "</font>");
                    //                ;
                  });
            }
          } else {
            qDebug() << "Unexpected";
          }
        } else {
          int cnt = agentList.count(agt);
          if (cnt == 1) {
            int index = agentList.indexOf(agt);
            ui->agentComboBox->removeItem(index);
            agentList.removeAt(index);

          } else {
            qDebug() << "Unexpected delete:" << cnt;
          }
          // qDebug() << "not create";
        }
      });

  //  connect(rps, &RobotProxyServer::agentCreated, this, [=](QString item) {
  //    ui->agentComboBox->addItem(item);
  //    if (ui->agentComboBox->count() == 1) {
  //      //      qDebug() << "123";
  //      connect(rps->getAgentPtrByIndex(0), &Agent::downWrite, this,
  //              [=](QString timestramp, QByteArray data) {
  //                //            data.left(6).toHex();
  //                //            qDebug() << "456";

  //                //            qDebug() << data;
  //                ui->downTextBrowser->append(
  //                    timestramp + "[Tx]:" +
  //                    QString::fromUtf8(data.left(6).toHex(' ')).toUpper() +
  //                    QString::fromUtf8(data.right(data.size() - 6)));
  //                //                ;
  //              });
  //    }
  //  });
  //  ui->downTextBrowser->textCursor();
  //  QTextCursor textCursor = ui->plainTextEdit->textCursor();
  //  QTextBlockFormat textBlockFormat;
  //  textBlockFormat.setLineHeight(1,
  //                                QTextBlockFormat::FixedHeight);  //
  //                                设置固定行高
  //  textCursor.setBlockFormat(textBlockFormat);
  //  ui->plainTextEdit->setTextCursor(textCursor);
  //  initServer();
  //  initClient();
}

MainWindow::~MainWindow() {
  //  closeServer();
  //  upclient->abort();
  delete ui;
}

// void MainWindow::initServer() {
//   // 创建Server对象
//   server = new QTcpServer(this);
//   server->listen(QHostAddress::Any, 12345);
//   updateState();

//  // 监听到新的客户端连接请求
//  connect(server, &QTcpServer::newConnection, this, [this] {
//    // 如果有新的连接就取出
//    while (server->hasPendingConnections()) {
//      // nextPendingConnection返回下一个挂起的连接作为已连接的QTcpSocket对象
//      //
//      套接字是作为服务器的子级创建的，这意味着销毁QTcpServer对象时会自动删除该套接字。
//      // 最好在完成处理后显式删除该对象，以避免浪费内存。
//      //
//      返回的QTcpSocket对象不能从另一个线程使用，如有需要可重写incomingConnection().
//      QTcpSocket *socket = server->nextPendingConnection();
//      clientList.append(socket);
//      qDebug() << QString("[%1:%2] Soket Connected")
//                      .arg(socket->peerAddress().toString())
//                      .arg(socket->peerPort());

//      // 关联相关操作的信号槽
//      // 收到数据，触发readyRead
//      connect(socket, &QTcpSocket::readyRead, [this, socket] {
//        // 没有可读的数据就返回
//        if (socket->bytesAvailable() <= 0) return;
//        // 注意收发两端文本要使用对应的编解码
//        const QString recv_text = QString::fromUtf8(socket->readAll());
//        ui->downTextBrowser->append(QString("[%1:%2]")
//                                        .arg(socket->peerAddress().toString())
//                                        .arg(socket->peerPort()));
//        ui->downTextBrowser->append(recv_text);

//        //
//        if (upclient->isValid()) {
//          // 将发送区文本发送给客户端
//          const QByteArray send_data = recv_text.toUtf8();
//          // 数据为空就返回
//          if (send_data.isEmpty()) return;
//          upclient->write(send_data);
//        }
//      });

//      // 错误信息
//      //      connect(
//      //          socket, &QAbstractSocket::errorOccurred,
//      //          [this, socket](QAbstractSocket::SocketError) {
//      //            ui->textBrowser->append(QString("[%1:%2] Soket Error:%3")
//      // .arg(socket->peerAddress().toString())
//      //                                        .arg(socket->peerPort())
//      //                                        .arg(socket->errorString()));
//      //          });

//      // 连接断开，销毁socket对象，这是为了开关server时socket正确释放
//      connect(socket, &QTcpSocket::disconnected, [this, socket] {
//        socket->deleteLater();
//        clientList.removeOne(socket);
//        ui->downTextBrowser->append(QString("[%1:%2] Soket Disonnected")
//                                        .arg(socket->peerAddress().toString())
//                                        .arg(socket->peerPort()));
//        updateState();
//      });
//    }
//    updateState();
//  });

//  // server向client发送内容
//  connect(ui->sendDownBtn, &QPushButton::clicked, [this] {
//    // 判断是否开启了server
//    if (!server->isListening()) return;
//    // 将发送区文本发送给客户端
//    const QByteArray send_data = ui->lineEdit_3->text().toUtf8();
//    // 数据为空就返回
//    if (send_data.isEmpty()) return;
//    for (QTcpSocket *socket : clientList) {
//      socket->write(send_data);
//      // socket->waitForBytesWritten();
//    }
//  });

//  // server的错误信息
//  // 如果发生错误，则serverError()返回错误的类型，
//  // 并且可以调用errorString()以获取对所发生事件的易于理解的描述
//  connect(
//      server, &QTcpServer::acceptError, [this](QAbstractSocket::SocketError) {
//        ui->downTextBrowser->append("Server Error:" + server->errorString());
//      });
//}

// void MainWindow::closeServer() {
//   // 停止服务
//   server->close();
//   for (QTcpSocket *socket : clientList) {
//     // 断开与客户端的连接
//     socket->disconnectFromHost();
//     if (socket->state() != QAbstractSocket::UnconnectedState) {
//       socket->abort();
//     }
//   }
// }

// void MainWindow::updateState() {
//   // 将当前server地址和端口、客户端连接数写在标题栏
//   if (server->isListening()) {
//     qDebug() << (QString("Server[%1:%2] connections:%3")
//                      .arg(server->serverAddress().toString())
//                      .arg(server->serverPort())
//                      .arg(clientList.count()));
//   }
// }

// void MainWindow::updateState1() {
//   // 将当前client地址和端口写在标题栏
//   if (upclient->state() == QAbstractSocket::ConnectedState) {
//     qDebug() << (QString("Client[%1:%2]")
//                      .arg(upclient->localAddress().toString())
//                      .arg(upclient->localPort()));
//   }
// }

// void MainWindow::initClient() {
//   // 创建client对象
//   upclient = new QTcpSocket(this);

//  // 点击连接，根据ui设置的服务器地址进行连接
//  if (upclient->state() == QAbstractSocket::UnconnectedState) {
//    // 从界面上读取ip和端口
//    const QHostAddress address = QHostAddress(QString("10.122.197.87"));
//    // 连接服务器
//    upclient->connectToHost(address, 23456);
//  } else {
//    ui->upTextBrowser->append("It is not ConnectedState or UnconnectedState");
//  }

//  // 发送数据
//  connect(ui->sendUpBtn, &QPushButton::clicked, this, [this] {
//    // 判断是可操作，isValid表示准备好读写
//    if (!upclient->isValid()) return;
//    // 将发送区文本发送给客户端
//    const QByteArray send_data = ui->upTextBrowser->toPlainText().toUtf8();
//    // 数据为空就返回
//    if (send_data.isEmpty()) return;
//    upclient->write(send_data);
//    // client->waitForBytesWritten();
//  });

//  // 收到数据，触发readyRead
//  connect(upclient, &QTcpSocket::readyRead, [this] {
//    // 没有可读的数据就返回
//    if (upclient->bytesAvailable() <= 0) return;
//    // 注意收发两端文本要使用对应的编解码
//    const QString recv_text = QString::fromUtf8(upclient->readAll());
//    ui->upTextBrowser->append(QString("[%1:%2]")
//                                  .arg(upclient->peerAddress().toString())
//                                  .arg(upclient->peerPort()));
//    ui->upTextBrowser->append(recv_text);

//    // 将发送区文本发送给客户端
//    const QByteArray send_data = recv_text.toUtf8();
//    // 数据为空就返回
//    if (send_data.isEmpty()) return;
//    for (QTcpSocket *socket : clientList) {
//      socket->write(send_data);
//      // socket->waitForBytesWritten();
//    }
//  });

//  // 错误信息
//  //  connect(
//  //      upclient, &QAbstractSocket::errorOccurred,
//  //      [this](QAbstractSocket::SocketError) {
//  //        ui->textBrowser_2->append("Socket Error:" +
//  //        upclient->errorString());
//  //      });
//  updateState1();
//}

void MainWindow::on_startRpsBtn_clicked(bool checked) {
  if (checked) {
    QString upAddr = ui->upIPAddr->text();
    QString downAddr = ui->downIPAddr->text();
    int upPort = ui->upIPPort->value();
    int downPort = ui->downIPPort->value();
    //    rps->setWindow(this);
    if (rps->startRPS(downAddr, downPort, upAddr, upPort)) {
      ui->startRpsBtn->setChecked(true);
      ui->startRpsBtn->setText("Stop");
    }
    else {
      ui->startRpsBtn->setChecked(false);
    }
  }
  else {
    rps->close();
    ui->startRpsBtn->setText("Start");
  }
}

// void MainWindow::on_liftBtn_clicked() {
//   connect(this, &MainWindow::lift, rps->getAgent(), &Agent::sendActionMove);
//   emit lift();
// }

//连接P40
void MainWindow::on_openCtrlPanel_clicked(bool checked) {
  if (checked) {
    if (ui->agentComboBox->count() > 0) {
      panel = new ControlPanel(this,
                               agentList.at(ui->agentComboBox->currentIndex()));
      connect(panel, &ControlPanel::windowsClosed, this, [=] {
        ui->openCtrlPanel->setChecked(false);
        ui->openCtrlPanel->setText("ControlPanel");
        panel->deleteLater();
      });
      panel->show();
      ui->openCtrlPanel->setText("CloseControl");
    } else {
      ui->openCtrlPanel->setChecked(false);
    }
  } else {
    ui->openCtrlPanel->setChecked(false);
    ui->openCtrlPanel->setText("ControlPanel");
    panel->deleteLater();
  }
}

void MainWindow::on_refresh_btn_clicked() {
  for (int i = 0; i < agentList.size(); ++i) {
    QString data = agentList.at(i)->getAgentAddrAndPort() + "(" +
                   agentList.at(i)->getAgentName() + ")";
    ui->agentComboBox->setItemText(i, data);
  }
}

//虚拟车按钮
void MainWindow::on_pushButton_clicked()
{
  vir::VirtualRobot *virtualBot = new vir::VirtualRobot(this);
  VirtualPanel *panel = new VirtualPanel(this,virtualBot,
                                         agentList.at(ui->agentComboBox->currentIndex()));

  panel->show();
}


