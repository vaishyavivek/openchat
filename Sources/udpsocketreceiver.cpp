#include "udpsocketreceiver.h"

#include <QJsonDocument>
#include <QTimer>

UdpSocketReceiver::UdpSocketReceiver(QString ActorId, QString ActorName, QString BrokerIp, qint16 BrokerPort, QObject *parent)
    : QObject{parent}, brokerIp(BrokerIp), brokerPort(BrokerPort)
{
    actorCredentials["ActorIdB"] = ActorId;
    actorCredentials["ActorNameB"] = ActorName;

    socket = new QUdpSocket();
    socket->bind(QHostAddress("0.0.0.0"), 0, QAbstractSocket::ReuseAddressHint);
    connect(socket, &QUdpSocket::readyRead, this, &UdpSocketReceiver::MessageRecieved);

    SendReHello();
}

void UdpSocketReceiver::SendReHello() {

    QJsonObject obj;
    obj["MessageType"] = "ReHello";
    obj["ActorId"] = actorCredentials["ActorIdB"];

    emit setDebugMessages("Creating New Socket connection with Broker");

    QJsonDocument doc(obj);
    socket->writeDatagram(doc.toJson(QJsonDocument::Compact), QHostAddress(brokerIp), brokerPort);
}

void UdpSocketReceiver::SendActorConnectAccept(QJsonObject actorData) {

    if( actorData["ActorSecret"].toString().compare(actorCredentials["ActorSecretB"].toString()) == 0) {
        actorCredentials["ActorIdA"] = actorData["ActorId"];
        actorCredentials["ActorNameA"] = actorData["ActorName"];
        actorCredentials["ActorPublicIpA"] = actorData["ActorPublicIp"];
        actorCredentials["ActorPortA"] = actorData["ActorPort"];

        tries = 1;

        QJsonObject obj;
        obj["MessageType"] = "ActorConnectAccept";
        obj["ActorId"] = actorCredentials["ActorIdB"];
        obj["ActorName"] = actorCredentials["ActorNameB"];

        emit setDebugMessages("Sending ActorConnectAccept to " + actorCredentials["ActorIdA"].toString() +
                " > " + actorCredentials["ActorPublicIpA"].toString() +
                ":" + actorCredentials["ActorPortA"].toString());

        QJsonDocument doc(obj);
        socket->writeDatagram(doc.toJson(QJsonDocument::Compact),
                              QHostAddress(actorCredentials["ActorPublicIpA"].toString()),
                              actorCredentials["ActorPortA"].toInt());

        QTimer::singleShot(3000, this, &UdpSocketReceiver::SendActorConnectAcceptAgain);
    }
}

void UdpSocketReceiver::SendActorConnectAcceptAgain() {

    if(tries >= 1 && tries < 3) {

        tries++;

        QJsonObject obj;
        obj["MessageType"] = "ActorConnectAccept";
        obj["ActorId"] = actorCredentials["ActorIdB"];
        obj["ActorName"] = actorCredentials["ActorNameB"];

        emit setDebugMessages("Sending ActorConnectAccept to " + actorCredentials["ActorIdA"].toString() +
                " > " + actorCredentials["ActorPublicIpA"].toString() +
                ":" + actorCredentials["ActorPortA"].toString());

        QJsonDocument doc(obj);
        socket->writeDatagram(doc.toJson(QJsonDocument::Compact),
                              QHostAddress(actorCredentials["ActorPublicIpA"].toString()),
                              actorCredentials["ActorPortA"].toInt());

        QTimer::singleShot(3000, this, &UdpSocketReceiver::SendActorConnectAcceptAgain);
    }
}


void UdpSocketReceiver::SendActorConnectFinish() {

    QJsonObject obj;
    obj["MessageType"] = "ActorConnectFinish";
    obj["ActorId"] = actorCredentials["ActorIdB"];

    emit setDebugMessages("Sending ActorConnectFinish to " + actorCredentials["ActorIdA"].toString());

    QJsonDocument doc(obj);
    tries++;
    socket->writeDatagram(doc.toJson(QJsonDocument::Compact),
                          QHostAddress(actorCredentials["ActorPublicIpA"].toString()),
                          actorCredentials["ActorPortA"].toInt());
}

void UdpSocketReceiver::MessageRecieved() {

    while ( socket->hasPendingDatagrams()) {

        int size = socket->pendingDatagramSize();
        char *data = new char[ size];
        socket->readDatagram( data, size);

        QString datastr( data);
        datastr.truncate( datastr.lastIndexOf( '}') + 1);

        auto json = ( QJsonDocument::fromJson( datastr.toUtf8() ) ).object();

        delete[] data;

        QString typeOfPacket = json["MessageType"].toString();

        if( typeOfPacket  == "ReHelloAcknowledge") {

            emit setDebugMessages("Received ReHello Acknowledge");
        }
        else if( typeOfPacket == "ActorConnectInitiate") {

            emit setDebugMessages("Received ActorConnectInitiate from "
                                  + json["ActorPublicIp"].toString() + ":" + json["ActorPort"].toString());
            SendActorConnectAccept(json);
        }

        else if( typeOfPacket == "ActorConnectAcknowledge") {

            tries = 3;
            SendActorConnectFinish();

            emit setDebugMessages("Received ActorConnectAcknowledge from "
                                  + json["ActorId"].toString());

            isConnected = true;
            emit connectionFinished();
        }
        else if( typeOfPacket == "Data") {

            emit setDebugMessages(json["dataType"].toString());
        }
    }
}
