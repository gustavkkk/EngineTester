#include <QCoreApplication>
#include <OCREngine/OCREngine_unix.h>
#include <OCREngine/global_unix.h>
#include <opencv2/opencv.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <QString>
#include <QDataStream>
#include <QByteArray>
#include <QFile>
#include <QMessageLogContext>
#include <QtService/qtservice.h>
#include <QtCore/QTextStream>
#include <QtCore/QDateTime>
#include <QtCore/QStringList>
#include <QtCore/QDir>
#include <QtCore/QSettings>
#include <QtEndian>
#include <QTcpServer>
#include <QTcpSocket>
#include <CVAThread.h>
//#include <opencv2/highgui/highgui.hpp>
using namespace cv;

BYTE *pDicData[DIC_COUNT];
BYTE *pLockSymDic;

QString src_dir;
QString dest_dir;

#define ini_path "/home/kojy/ocr.ini"
#define src_path  "/home/kojy/ocr/src"
#define dest_path "/home/kojy/ocr/dest"

class COCRDaemon_NoNetwork : public QTcpServer
{
    Q_OBJECT
public:
    COCRDaemon_NoNetwork(QObject *parent = 0)
        : QTcpServer(parent),disabled(false)
    {
        QObject::connect(this,SIGNAL(sstart()),this,SLOT(start()));
        QObject::connect(this,SIGNAL(spause()),this,SLOT(pause()));
        QObject::connect(this,SIGNAL(sresume()),this,SLOT(resume()));
        QObject::connect(this,SIGNAL(sstop()),this,SLOT(stop()));
        start();
    }
//    ~COCRDaemon_NoNetwork();
public slots:
    void start()
    {
        while(1)
        {
            process();
            QThread::msleep(2000);
        }
    }
    void process()
    {
        if(disabled)
            return;

        QDir dir(src_dir);
        QStringList namefilter;
        namefilter<<"*.jpg";
        dir.setFilter(QDir::Files);
        dir.setNameFilters(namefilter);
        QFileInfoList list = dir.entryInfoList();
        for(int i = 0; i < list.size(); i++)
        {
            QFileInfo fileinfo = list.at(i);
            if(fileinfo.fileName().contains("_r"))
                continue;
            Mat image = imread(fileinfo.absoluteFilePath().toStdString());//QString2BString(fileinfo.absoluteFilePath()));
            if(image.empty())
                continue;
            OCR_RESULT result;
            OCR(image,&result);
//            imwrite("/home/joonyong/Downloads/bin.jpg",result.seg_res.imgSegments);
            //--make an out test file
            QFile file(dest_dir + "/" + fileinfo.fileName().mid(0,fileinfo.fileName().size() - 3) + "txt");
            if(!file.open(QIODevice::WriteOnly))
            {

            }
            QTextStream in(&file);
            for(vector<char>::iterator iter = result.string.begin(); iter != result.string.end(); ++iter)
            {
                char ch = *iter;
                in<<ch;
            }
            file.close();
            //rename
            QString oldname = fileinfo.absoluteFilePath(),newname = oldname;
            newname.insert(oldname.size() - 4,"_r");
            dir.rename(oldname,newname);
        }
    }

    void pause()
    {
        disabled = true;
    }
    void resume()
    {
        disabled = false;
    }
    void stop()
    {
        disabled = true;
    }
signals:
    void sstart();
    void spause();
    void sresume();
    void sstop();
private:
    bool disabled;
};

class COCRDaemon : public QTcpServer
{
    Q_OBJECT
public:
    COCRDaemon(quint16 port, QObject* parent = 0)
        : QTcpServer(parent), disabled(false),m_len_hmsg(31),m_len_bmsg(18)
    {
        initFailedMsg();
        listen(QHostAddress::Any, port);
        connect(this, SIGNAL(newConnection()), this, SLOT(AssociateIncomingClientWithContext()));
    }
    ~COCRDaemon()
    {
        free(m_fwhole_msg);
        free(m_fbody_msg);
    }
    void pause()
    {
        disabled = true;
    }

    void resume()
    {
        disabled = false;
    }

private slots:
    void AssociateIncomingClientWithContext()
    {
        if(disabled)
            return;

        //qDebug("pass");

        QTcpSocket *socket = this->nextPendingConnection();//(QTcpSocket*)sender();
        socket->setReadBufferSize(0);
        socket->setSocketOption(QAbstractSocket::LowDelayOption, 1);
        socket->setSocketOption(QAbstractSocket::KeepAliveOption, 1);

        CVAThread* pVAThread = new CVAThread(this, socket);
        QObject::connect(pVAThread, SIGNAL(processSucceeded(QTcpSocket*,char*,int)), this, SLOT(onProcessSucceeded(QTcpSocket*,char*,int)));
        QObject::connect(pVAThread, SIGNAL(processFailed(QTcpSocket*)), this, SLOT(onProcessFailed(QTcpSocket*)));
        QObject::connect(pVAThread, SIGNAL(finished()), pVAThread, SLOT(deleteLater()));
        pVAThread->start();
        return;
    }

    static qint32 qhtonl(qint32 src)
    {
        qint32 dest = qToBigEndian(src);
        return dest;
    }
    void onProcessSucceeded(QTcpSocket *socket, char *result, int len)
    {
        //qDebug("Last Step");
        //qDebug(result);
        socket->write(result, len);
        socket->disconnectFromHost();
        socket->close();
        free(result);
    }
    void onProcessFailed(QTcpSocket *socket)
    {
        //qDebug("Failed");
        socket->write(m_fwhole_msg,12 + m_len_bmsg + 1);
        socket->disconnectFromHost();
        socket->close();
    }
    void initFailedMsg()
    {
        m_fwhole_msg = new char[m_len_hmsg];
        memset(m_fwhole_msg,0,m_len_hmsg);
        *(qint32 *)m_fwhole_msg = qhtonl(1);
        *((qint32 *)m_fwhole_msg+1) = 0;
        *((qint32 *)m_fwhole_msg+2) = qhtonl(m_len_bmsg + 1);
        m_fbody_msg = new char[m_len_bmsg];
        memset(m_fbody_msg,0,m_len_bmsg);
        char msg[] = {'F','a','i','l','e','d','!',' ','T','r','y',' ','A','g','a','i','n','!'};
        memcpy((char *)((qint32 *)m_fwhole_msg+3), msg, m_len_bmsg);

    }

private:
    bool disabled;
    char* m_fwhole_msg;
    char* m_fbody_msg;
    int m_len_hmsg;
    int m_len_bmsg;

};
#include "main.moc"
class COCRService : public QtService
{
public:
    COCRService(int argc,char** argv)
        : QtService(argc,argv,"Qt OCR Daemon")
    {
        setStartupType(QtServiceController::AutoStartup);
        setServiceDescription("A dummy OCR service implemented with Qt");
        setServiceFlags(QtServiceBase::CanBeSuspended);
    }
    ~COCRService(){}
protected:
    void start()
    {
        QCoreApplication *app = application();

#ifdef USING_NETWORK
        quint16 port = 56789;
        daemon = new COCRDaemon(port, app);

        if (!daemon->isListening()) {
            logMessage(QString("Failed to bind to port %1").arg(daemon->serverPort()), QtServiceBase::Error);
            app->quit();
        }
#else
    daemon = new COCRDaemon_NoNetwork(app);
#endif
    }
    void pause()
    {
    daemon->pause();
    }

    void resume()
    {
    daemon->resume();
    }

private:

#ifdef USING_NETWORK
    COCRDaemon *daemon;
#else
    COCRDaemon_NoNetwork *daemon;
#endif
};
#define DIC_PATH "/usr/share/ocr"
// "var/tmp/DIC""/home/kojy/Downloads/QtOCR_DEBUG/bin/DIC"
//#define LIB_PATH "/usr/lib/ocr"
bool BringDictionary()
{
    QString DIC_dir;
    DIC_dir = DIC_PATH;
    //////////////////////////////////////////////////
    QString	DicPath;
    QString	temp;
    int	i, nSize;
    int dic_count = 0;
    for( i = 0; i < DIC_COUNT; i++)
    {
        pDicData[i] = NULL;
        temp = DIC_dir;
        temp += "/sz_ocr_";
        char count[1];
        sprintf(count,"%d",i);
        temp += count;
        DicPath = temp + ".dic";

        QFile file(DicPath);
        if(file.open(QIODevice::ReadOnly))
        {
            nSize = file.size();
            pDicData[i] = new BYTE[nSize];
            QDataStream stream(&file);
            char* tmp = new char[nSize];
            stream.readRawData(tmp,nSize);
            memcpy(pDicData[i],tmp,nSize);
            dic_count++;
        }
        file.close();
    }
    pLockSymDic = NULL;
    DicPath = DIC_dir + "/sz_sym.dic";
    QFile file(DicPath);
    if(file.open(QIODevice::ReadOnly))
    {
        nSize = file.size();
        pLockSymDic = new BYTE[nSize];
        QDataStream stream(&file);
        char* tmp = new char[nSize];
        stream.readRawData(tmp,nSize);
        memcpy(pLockSymDic,tmp,nSize);
        dic_count++;
    }

    file.close();
    if(dic_count < DIC_COUNT)
        return false;
    return true;
}

bool existInifile()
{
    QFile file(ini_path);
    if(file.open(QIODevice::ReadOnly))
    {
        file.close();
        return true;
    }
    file.close();
    return false;
}

int main(int argc, char **argv)
{
#if !defined(Q_WS_WIN)
    // QtService stores service settings in SystemScope, which normally require root privileges.
    // To allow testing this example as non-root, we change the directory of the SystemScope settings file.
    //QSettings::setPath(QSettings::NativeFormat, QSettings::SystemScope, QDir::tempPath());
    //qWarning("(Example uses dummy settings file: %s/QtSoftware.conf)", QDir::tempPath().toLatin1().constData());
#endif
    //check if dictionary exists or not
    if(!BringDictionary())
    {
        qWarning("No Dictionary! Check the Dic Directory!");
        return 0;
    }
    //Initialize environment variables;
    QString srcdir = "srcdir",destdir = "destdir";
    QVariant srcdir_qv("/usr/share/ocr/src"),destdir_qv("/usr/share/ocr/dest");
    QSettings settings("/usr/bin/ocr.ini",QSettings::IniFormat);
    if(settings.value(srcdir).toString() == "" ||
       settings.value(destdir).toString() == "")
    {
        settings.setValue(srcdir,srcdir_qv);
        settings.setValue(destdir,destdir_qv);
    }
    src_dir = settings.value(srcdir).toString();
    dest_dir = settings.value(destdir).toString();
    ////////////////////////////////////////////////////
    COCRService service(argc, argv);
    return service.exec();
}
