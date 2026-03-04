#include "robot_proxy_server.h"

#include <QDebug>
#include <QThread>

#include "agent.h"
#include "spdlog/spdlog.h"

RobotProxyServer::RobotProxyServer(QObject* parent)
    : QTcpServer{parent}, useGUI(false) {
  qRegisterMetaType<QAbstractSocket::SocketError>(
      "QAbstractSocket::SocketError");
}
RobotProxyServer::~RobotProxyServer() {
  //  const auto& list = agentPtrList;
  //  for (auto agt : list) {
  //    agt->deleteLater();
  //  }
  //  thread->terminate();
  //  delete thread;
}

// void RobotProxyServer::init(bool gui) {
//   useGUI = gui;
//   connect(this, &RobotProxyServer::newConnection, this, [] {
//     spdlog::info("RPS get new connection.");
//     //    while (this->hasPendingConnections()) {
//     //      QTcpSocket *downClient = this->nextPendingConnection();
//     //      QTcpSocket *upClient = new QTcpSocket(this);
//     //      clientList.append(qMakePair(downClient, upClient));
//     //      qDebug() << clientList;
//     //    }
//   });
// }

void RobotProxyServer::incomingConnection(qintptr socketDescriptor) {
  spdlog::get("rps_logger")->info("incomingConnection");
  QThread* thread = new QThread(this);  // 把线程的父类设为连接的，防止内存泄漏
  //  thread = new QThread(this);  // 把线程的父类设为连接的，防止内存泄漏
  //  qDebug() << "a";
  Agent* agt = new Agent();
  //  qDebug() << "b";
  threadPool.insert(thread, agt);
  //  qDebug() << "c";
  agt->setSktDescriptor(socketDescriptor);
  // 在线程启动的时候创建定时器
  connect(thread, SIGNAL(started()), agt, SLOT(onCreateTimer()));
  connect(agt, &QObject::destroyed, this, [=] {
    spdlog::info("agt destroyed.");
    if (threadPool.contains(thread, agt)) {
      emit agentUpdate(false, agt);
      //      qDebug() << "contains";
      threadPool.remove(thread, agt);
      if (threadPool.contains(thread, agt)) {
        //        qDebug() << "recheck true";
      } else {
        //        qDebug() << "recheck false";
        if (threadPool.values(thread).empty()) {
          //          qDebug() << "thread empty,delete";
          thread->quit();
          thread->wait();
          //            spdlog::info("agt destroye2.");
        }
      }
    } else {
      //      qDebug() << "not contains";
    }

    //    thread->terminate();
  });
  //  connect(thread,SIGNAL(finished()),thread,SLOT(deleteLater()));

  connect(thread, &QThread::finished, this,
          [] { spdlog::info("thread finished"); });
  connect(agt, &Agent::agentCreated, this, [=](QString text) {
    emit this->agentCreated(text);
    emit agentUpdate(true, agt);
  });
  connect(agt, &Agent::downStreamClosed, this, [=]() {
    //    thread->finished();
    //    agt->deleteLater();
    //    thread->terminate();
    //    delete agt;
    //    agt->deleteLater();
    //              emit this->agentCreated(text);
  });
  //  qDebug() << "d";
  agt->moveToThread(thread);
  agt->setMapServer(map);
  //  qDebug() << "e";
  thread->start();  // 线程开始运行
                    //  qDebug() << "f";
  //  agentPtrList.append(agt);
  //  qDebug() << "agentPtrList:" << agentPtrList;
  //  qDebug() << "threadPool:" << threadPool;
}

// void RobotProxyServer::incomingConnection(qintptr socketDescriptor) {
//   spdlog::get("rps_logger")->info("incomingConnection");
//   agent = new Agent();
//   //  qDebug() << socketDescriptor << "da";
//   agent->setSktDescriptor(socketDescriptor);
//   //    emit newConnection();
//   //    myTcpSocket * tcpTemp = new myTcpSocket(socketDescriptor);
//   thread = new QThread(this);  // 把线程的父类设为连接的，防止内存泄漏
//   //
//   //可以信号连接信号的，我要捕捉线程ID就独立出来函数了，使用中还是直接连接信号效率应该有优势
//   //
//   connect(tcpTemp,&myTcpSocket::readData,this,&MyTcpServer::readDataSlot);//接受到数据
//   //
//   connect(tcpTemp,&myTcpSocket::sockDisConnect,this,&MyTcpServer::sockDisConnectSlot);//断开连接的处理，从列表移除，并释放断开的Tcpsocket
//   //
//   connect(this,&MyTcpServer::sentData,tcpTemp,&myTcpSocket::sentData);//发送数据
//   //
//   connect(tcpTemp,&myTcpSocket::disconnected,thread,&QThread::quit);//断开连接时线程退出
//   //    tcpTemp->moveToThread(thread);//把tcp类移动到新的线程
//   connect(thread, SIGNAL(started()), agent,
//           SLOT(onCreateTimer()));  // 在线程启动的时候创建定时器
//   connect(agent, &QObject::destroyed, thread, &QThread::terminate);
//   //    connect(thread, &QThread::finished, agent, &QObject::deleteLater);
//   connect(thread, &QThread::finished, agent,
//           [] { spdlog::info("thread finished"); });

//  //  connect(window, &MainWindow::lift, agent, &Agent::sendActionLift);
//  //  thread->moveToThread(thread);
//  agent->moveToThread(thread);

//  QPointF f;
//  map->getPointById(10351000, f);
//  spdlog::info("test:{},{}", f.x(), f.y());
//  agent->setMapServer(map);

//  qRegisterMetaType<QAbstractSocket::SocketError>(
//      "QAbstractSocket::SocketError");

//  thread->start();  // 线程开始运行

//  //  agent->socket_down->setSocketDescriptor(socketDescriptor);

//  //    tcpClient->insert(socketDescriptor,tcpTemp);//插入到连接信息中
//  //  qDebug() << "incomingConnection THREAD IS：" <<
//  //  QThread::currentThreadId();
//  //    //发送连接信号
//  //    emit
//  //
//  connectClient(tcpTemp->socketDescriptor(),tcpTemp->peerAddress().toString(),tcpTemp->peerPort());
//}

bool RobotProxyServer::startRPS(QString downAddr, int downPort, QString upAddr,
                                int upPort) {
  this->upIPAddr = QHostAddress(upAddr);
  this->upIPPort = upPort;
  if (listen(QHostAddress(downAddr), downPort)) {
    spdlog::get("rps_logger")
        ->info("Listening downstream({}:{})", downAddr.toStdString(), downPort);
    return true;
  } else {
    spdlog::get("rps_logger")->info("Start rps failed.");
    return false;
  }
}

// void RobotProxyServer::setWindow(MainWindow *w) { window = w; }
