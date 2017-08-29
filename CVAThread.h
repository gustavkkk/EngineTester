#pragma once
#ifndef CVATHREAD_H
#define CVATHREAD_H

#include <QtCore/QThread>
#include <QtNetwork/QTcpSocket>

class CVAThread : public QThread
{
    Q_OBJECT
public:
    CVAThread(void *pServer, QTcpSocket *pSocket, QObject *parent = 0);
    CVAThread(void *pServer, QTcpSocket *pSocket,char *msg,int sizeofmsg, QObject *parent = 0);
    virtual ~CVAThread();

    void run();

signals:
    void error(QTcpSocket::SocketError socketError);
    void processSucceeded(QTcpSocket *socket, char* result, int len);
    void processFailed(QTcpSocket *socket);
public slots:
    bool processRequest();
    void exit();
private:
    void *m_pVAServer;
    QTcpSocket *m_pTcpSocket;
    int m_nSeek;
    bool m_bExit;

    char* m_msg;
    int m_sizeofmsg;

};

#endif // CVATHREAD_H
