#include "CVAThread.h"
#include <opencv2/opencv.hpp>
#include <QtEndian>
//#include "VAServer.h"
#include <OCREngine/OCREngine.h>
using namespace cv;
#define DEFAULT_BUFLEN		102400
#define DEFAULT_RESPBUFLEN	100
#define DEFAULT_PORT		"56789"

CVAThread::CVAThread(void *pServer, QTcpSocket *pSocket, QObject *parent) :
    QThread(parent)
{
    m_pVAServer = pServer;
    m_pTcpSocket = pSocket;
}

CVAThread::CVAThread(void *pServer, QTcpSocket *pSocket,char* msg,int sizeofmsg, QObject *parent) :
    QThread(parent)
{
    m_pVAServer = pServer;
    m_pTcpSocket = pSocket;
    m_msg = msg;
    m_sizeofmsg = sizeofmsg;
}
CVAThread::~CVAThread()
{
}

void CVAThread::exit()
{
    m_bExit = true;
}
void CVAThread::run()
{
    if(!processRequest())
        processFailed(m_pTcpSocket);
}
qint32 qhtonl(qint32 src)
{
    qint32 dest = qToBigEndian(src);
    return dest;
}

void freememories(char* message,char* resp)
{
    if(message)
    {
        free(message);
    }
    if(resp)
    {
        free(resp);
    }
}

bool CVAThread::processRequest()
{
    //qDebug("Stepped into processRequest");
    /////////////////////////////////////////////////
    qint64 recvbuflen = DEFAULT_BUFLEN;//m_pTcpSocket->bytesAvailable();
    char *message;//, *resp;
    message = (char*)malloc(recvbuflen);
    memset (message, 0, recvbuflen);
    ////////////////////////////////////
    char *resp = new char[DEFAULT_RESPBUFLEN];
    memset(resp, 0, DEFAULT_RESPBUFLEN);
    //////////////////////////////////////////////////
    int iResult = 1;
    QThread::msleep(1000);
    iResult = m_pTcpSocket->read(message, recvbuflen);
    if (iResult <= 0)
    {
        free(message);
        free(resp);
        return false;
    }
    // Login message
    //qDebug("to be about to enter the main hall!");
    //qDebug(message);
    // OCR request message
    int msg_size = qhtonl(*((int *)message+1));
    if (*(int *)message == qhtonl(1)
        && msg_size > 0)
    {
        std::vector<uchar>	jpg;
        Mat			image;
        OCR_RESULT	result;
        int tmp = *((int *)message+1);
        //qDebug("before imdecode");
#ifndef OCR_DEBUG
        int ddd = qhtonl(tmp);
        char out[244];
        sprintf(out,"size of img = %d",ddd);
        //qDebug(out);
#endif
        jpg.resize(qhtonl(tmp));
        tmp = *((int *)message + 1);
        memcpy(jpg.data(), ((int *)message+2), qhtonl(tmp));
        image = imdecode(jpg, CV_LOAD_IMAGE_COLOR);
        if(image.empty())
        {
            //qDebug("Empty Image!");
            free(resp);
            free (message);
            return false;
        }
        OCR(image, &result);
        if(result.string.size() > 0)
        {
           //qDebug("success-wow!");
        }
        else{
            free(resp);
            free(message);
            return false;
        }

        *(qint32 *)resp = qhtonl(1);
        *((qint32 *)resp+1) = 0;
        tmp = result.string.size() + 1;
        *((qint32 *)resp+2) = qhtonl(tmp);
        memcpy((char *)((qint32 *)resp+3), result.string.data(), result.string.size());
        processSucceeded(m_pTcpSocket, resp, tmp+12);
    }
    else{
        free(resp);
    }
    free (message);
    return true;

}
