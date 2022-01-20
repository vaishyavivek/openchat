#ifndef UDPSOCKETRECEIVER_H
#define UDPSOCKETRECEIVER_H

#include <QObject>
#include <QUdpSocket>
#include <QJsonObject>

class UdpSocketReceiver : public QObject
{
    Q_OBJECT
public:
    explicit UdpSocketReceiver(QString ActorId, QString ActorName = "User", QString BrokerIp = "localhost", qint16 BrokerPort = 3001, QObject *parent = nullptr);

signals:

    void setDebugMessages(const QString &message);

    void connectionFinished();

private slots:

    void MessageRecieved();

private:

    // MEMBERS
    QString brokerIp;
    qint16 brokerPort;
    QUdpSocket *socket;

    QJsonObject actorCredentials;

    qint8 tries;
    bool isConnected = false;
    bool isActive = false;

    //METHODS
    void SendReHello();

    void SendActorConnectAccept(QJsonObject ActorData);

    void SendActorConnectAcceptAgain();

    void SendActorConnectFinish();
};

#endif // UDPSOCKETRECEIVER_H
