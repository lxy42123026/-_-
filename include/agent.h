#ifndef AGENT_H
#define AGENT_H

#include <QObject>
#include <QTcpSocket>
#include <QTimer>

#include "AgvRtStatus.pb.h"
#include "map_server.h"

enum class enMsgHeaderType {
  AGV_TYPE_NULL = 0,
  AGV_ACTION = 1,
  AGV_ACTION_EVENT = 2,
  AGV_REGISTER_REQUEST = 3,
  AGV_RT_STATUS = 4,
  AGV_UNREGISTER_REQUEST = 5,
  HEART_BEAT = 6,
  SESSION_REGISTER_INFO = 7,
  INIT_POS_REQ = 8,
  INIT_POS_RESPONSE = 9,
  AGV_REGISTER_RESPONSE = 10,
};

enum class unpackingEnum {
  UNPACKING_WAITING_PREFIX_1 = 0,
  UNPACKING_WAITING_PREFIX_2,
  UNPACKING_WAITING_HEADER,
  UNPACKING_WAITING_BODY
};

typedef struct {
  unsigned char ch1;
  unsigned char ch2;
  unsigned short len;
  unsigned short type;
} Msg;

typedef struct {
  unsigned short len;
  unsigned short type;
} HeaderMsg;

class Agent : public QObject {
  Q_OBJECT
 public:
  explicit Agent(QObject* parent = nullptr);
  ~Agent();
  QString getAgentAddrAndPort() { return ipAddr + ":" + ipPort; }
  QString getAgentName() {
    if (agvStatus.agvid().empty())
      return "Unknow";
    else
      return QString::fromStdString(agvStatus.agvid());
  }
  void messageProcess(enMsgHeaderType type, const QByteArray& data);
  bool protocolDecoding(const QByteArray& data);
  void sendSessionResponse();
  void sendHeatbeatResponse();
  void sendInitPosResponse(int logic_x, int logic_y, int code);
  void sendRegisterResponse(std::string agv_id, double x, double y);
  void handleActionEvent();
  void setMapServer(MapServer* m) { map = m; };

  void setSktDescriptor(qintptr skt) { sktDescriptor = skt; }
  void downSocketWrite(enMsgHeaderType type, std::string body);





  QTcpSocket* socket_down;

 public slots:

  void timer_slot();
  void onCreateTimer();
  void sendActionLift(int height);
  void sendActionPut(int height);
  void sendActionMove();
  void sendActionMoveForword(int grids);
  void sendActionMoveBackword(int grids);
  void sendActionTurn(int direction);
  void sendActionCharge();
  void sendActionUncharge();
  void sendActionClearErrors();
  void genMsgBase(cainiao::MessageBase& base,
                  cainiao::MessageBase::MessageTypeEnum msgType,
                  std::string path,
                  cainiao::MessageBase::Model::TypeEnum modelType);

  void linkRCStoP40(int orderId,int missionKey,quint32 pointId,int errorx,int errory);
  void linkRCStoP40_1(int orderId,int missionKey,quint32 pointId_1,int errorx_1,int errory_1,
                                                 quint32 pointId_2,int errorx_2,int errory_2);
  void sendRCSActionLiftDown(int orderId,int missionKey,quint32 pointId_1,int newX_1,int newY_1,int height);

 signals:
  void downStreamClosed();

  void agentCreated(QString descriptor);

  void downWrite(QString timestramp, QByteArray data);
  void downRead(QString timestramp, QByteArray data);

  void rtStatus(QString str);
  void heartBeat();
  void actionInfo(QStringList strList);

 private:
  QString ipAddr;
  QString ipPort;
  //  QString name;
  QTimer* timer = nullptr;
  //  QTcpSocket socket;
  std::string id;
  unpackingEnum unpackingStatus{unpackingEnum::UNPACKING_WAITING_PREFIX_1};
  HeaderMsg header_msg{0, 0};
  int cnt{0};
  QVector<int> debug_vector;
  MapServer* map;
  cainiao::AgvRtStatusMessage::AgvRtStatus agvStatus;

  qintptr sktDescriptor;

  int actionCnt{100};//初始化为100
  int traceCnt{100};
  //  QByteArray header_perfix;
};

#endif  // AGENT_H
