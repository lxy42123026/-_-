#ifndef ROBOTPROXYSERVER_H
#define ROBOTPROXYSERVER_H

#include <QMultiMap>
#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QThread>

#include "agent.h"
#include "map_server.h"
// #include "mainwindow.h"

class RobotProxyServer : public QTcpServer {
  Q_OBJECT
 public:
  explicit RobotProxyServer(QObject *parent = nullptr);
  ~RobotProxyServer();
  //  void init(bool gui);
  //  void setWindow(MainWindow *window);
  bool startRPS(QString downAddr, int downPort, QString upAddr, int upPort);
  //  QList<Agent *> getAllAgentPtr();
  //  Agent *getAgentPtrByIndex(int index) { return agentPtrList.at(index); }
  //  Agent *getAgent() { return agent; }
  void setMapServer(MapServer *m) { map = m; }

 protected:
  void incomingConnection(qintptr socketDescriptor);  // 覆盖已获取多线程

 signals:
  void agentCreated(QString descriptor);
  void agentUpdate(bool create, Agent *agt);

 private:
  bool useGUI;
  //  QList<Agent *> agentPtrList;
  //  QList<QPair<QTcpSocket *, QTcpSocket *>> clientList;
  QHostAddress upIPAddr;
  quint16 upIPPort;
  //  QThread *thread;
  Agent *agent;
  MapServer *map;
  QMultiMap<QThread *, Agent *> threadPool;
  //  MainWindow *window;
};

#endif  // ROBOTPROXYSERVER_H
