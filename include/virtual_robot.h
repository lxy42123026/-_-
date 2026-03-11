#ifndef VIRTUALROBOT_H
#define VIRTUALROBOT_H

#include <QHostAddress>
#include <QObject>
#include <QTcpSocket>
#include <QTimer>
#include <cmath>
#include <iostream>
#include <vector>
#include <QVector>
#include <QDebug>

#include "IMRAction.pb.h"
#include "IMRRegister.pb.h"
#include "IMRState.pb.h"
#include "Map.pb.h"
#include "Mission.pb.h"
//#include "agent.h"

// #include "AgvRtStatus.pb.h"
// #include "map_server.h"

// enum class enMsgHeaderType {
//   AGV_TYPE_NULL = 0,
//   AGV_ACTION = 1,
//   AGV_ACTION_EVENT = 2,
//   AGV_REGISTER_REQUEST = 3,
//   AGV_RT_STATUS = 4,
//   AGV_UNREGISTER_REQUEST = 5,
//   HEART_BEAT = 6,
//   SESSION_REGISTER_INFO = 7,
//   INIT_POS_REQ = 8,
//   INIT_POS_RESPONSE = 9,
//   AGV_REGISTER_RESPONSE = 10,
// };

namespace vir {
enum class unpackingEnum {
  UNPACKING_WAITING_PREFIX_1 = 0,
  UNPACKING_WAITING_PREFIX_2,
  UNPACKING_WAITING_HEADER,
  UNPACKING_WAITING_BODY
};

typedef struct {
  unsigned int len;
  unsigned short check_sum;
} HeaderMsg;

class VirtualRobot : public QObject {

  Q_OBJECT

 public:
  explicit VirtualRobot(QObject* parent = nullptr);
  ~VirtualRobot();

  void initConnection(QString addr, quint16 port);
  void deInitConnection();
  void sendRegister();
  void sendStatus();
  void startQTimer();
  void stopQTimer();
  void updateStatus();
  void logicDecision();
  void reportError();
//  void updatePos(int orderID,int missionKey,quint32 pointId,int newX, int newY );
  void updatePos_1(int orderID,int missionKey,quint32 pointId_1,int newX_1, int newY_1,quint32 pointId_2,int newX_2, int newY_2);
  void updateAction(std::string ID, std::string Param);
  void initValue(quint16 imrID,quint16 segID,quint16 imrlayerID,quint32 pointID);
  void linkP40_pos(int p40_x,int p40_y,int directionId,int power,float speed ,int loadHeight );
  void updateHeight(int orderID,int missionKey,quint32 pointId_1,int x_1,int y_1,int height);
  void posDecision();

  void switchOrder();
  //void switchKey();

  int x=0;//imr的x坐标
  int y=0;//imr的y坐标
  int v=0;//速度
  int globalVar = 0;//心跳

  int hight;

  QString log_Sta;
  int i=0;//点次序
  int j=0;//段次序
  int a=0;//动作数量遍历
  int k = 0;
  int isFirstOrder = 0;
  int action_number = 0;//动作执行计时

  Mission task;       //RCS发布的任务
  std::vector<Mission> task_array;
  Mission task_Imr;   //IMR缓存的任务
  IMRState ImrSta;    //IMR状态

  std::string ID;
  std::string Param;

 public slots:


 signals:
    void updateP40pos(int orderID,int missionKey,quint32 pointId_1,int x_1,int y_1);
    void sendAimToP40(int orderID,int missionKey,quint32 pointId_1,int x_1,int y_1,quint32 pointId_2,int x_2,int y_2);
    void doRcsAction(int orderID,int missionKey,quint32 pointId,int x,int y,int height);

 private:
  void messageProcess(std::string type, std::string msg);
  QTcpSocket* client;
  //unpackingEnum 是一个自定义的枚举类型，用于表示解包的状态。
  //枚举类型定义了一组可能的值，可以用于限制变量的取值范围，并增加代码的可读性和可维护性
  //unpackingStatus 变量的初始值为 UNPACKING_WAITING_PREFIX_1，表示当前的解包状态为等待接收消息头部的第一个字节。
  //根据解包状态的不同，代码会执行相应的处理逻辑来解析和处理接收到的数据。  prefix 字首
  unpackingEnum unpackingStatus{unpackingEnum::UNPACKING_WAITING_PREFIX_1};
  //rx_body_len 变量用于存储接收到的消息体的长度。
  //在解包过程中，根据消息头部中的长度字段，将其赋值给 rx_body_len 变量，以便后续处理接收到的消息体。
  unsigned int rx_body_len{0};

  QTimer *timer;
  //QTimer *timer_action;

};
} // namespace vir
#endif  // VIRTUALROBOT_H
