#include <QtWidgets>
#include <QUdpSocket>

struct MessageHeader {
    quint16 messageIdentification;  // 消息标识
    quint32 messageLength;          // 消息长度
    quint16 messageType;            // 消息类型
    quint16 messageNumber;          // 消息序号
    quint32 messageVersion;         // 消息版本号
    quint16 messageEncryptionType;  // 消息加密类型
    quint16 aggrementContentType;   // 协议内容类型
    quint16 reservedByte;           // 保留字节
};

struct MessageBody {
    // 消息体内容
};

class Server : public QObject
{
    Q_OBJECT
public:
    Server(QObject *parent = nullptr) : QObject(parent)
    {
        server = new QUdpSocket(this);

        // 绑定端口
        server->bind(QHostAddress::Any, 8080);

        connect(server, &QUdpSocket::readyRead, this, &Server::onReadyRead);
    }

private slots:
    void onReadyRead()
    {
        while (server->hasPendingDatagrams()) {
            // 读取客户端发送的数据
            QByteArray requestData;
            requestData.resize(server->pendingDatagramSize());
            QHostAddress clientAddress;
            quint16 clientPort;
            server->readDatagram(requestData.data(), requestData.size(), &clientAddress, &clientPort);

            // 解析消息头
            MessageHeader header;
            memcpy(&header, requestData.constData(), sizeof(header));
            quint32 messageLength = qFromBigEndian(header.messageLength);
            quint16 messageType = qFromBigEndian(header.messageType);

            // 解析消息体
            QByteArray messageData = requestData.mid(sizeof(MessageHeader));
            MessageBody *messageBody = reinterpret_cast<MessageBody*>(messageData.data());

            // 处理消息
            // ...

            // 发送响应消息头
            MessageHeader responseHeader;
            responseHeader.messageLength = qToBigEndian(responseBodyLength + sizeof(MessageHeader));
            responseHeader.messageType = qToBigEndian(responseMessageType);
            responseHeader.reserve = 0;

            // 构建响应消息体
            QByteArray responseBody;
            // ...

            // 构建响应消息
            QByteArray responseMessage;
            responseMessage.append(reinterpret_cast<const char*>(&responseHeader), sizeof(responseHeader));
            responseMessage.append(responseBody);

            // 发送响应消息
            server->writeDatagram(responseMessage, clientAddress, clientPort);
        }
    }

private:
    QUdpSocket *server;
};

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    Server server;
    return a.exec();
}













