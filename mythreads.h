#ifndef MYTHREADS_H
#define MYTHREADS_H

#include <QObject>
#include "cs_ftfunction.h"
#include <qtoolbar.h>
#include <QFile>
#include <QTextStream>
#include <qdatetime.h>
#include <qthread.h>
#include <qmutex.h>
#include <qmath.h>
#include "ectclass.h"


/// \brief The RWThread class
///
class RWThread : public QThread
  {
      Q_OBJECT
public:
    enum ET{
        ECT,
        TDlas
    };
    ET mode;
public:
    //usb,二进制文件，buffer长度，标志，锁
    explicit RWThread(CS_ftfunction *u, QFile *df,const int bl,bool rf,QMutex *lk,ET m)
        :usb(u),datafile(df),bufferlong(bl),runflag(rf),lock(lk),mode(m),QThread(){
        RxBuffer=(unsigned char*)malloc(bufferlong+1);//if(RxBuffer==NULL)可能需要保护判断
        //ZeroMemory(RxBuffer,bufferlong-10);
        datafile->open(QIODevice::ReadWrite|QIODevice::Append|QIODevice::Truncate);
        infile= new QDataStream(datafile);
        count=0;

    }
    void run(); /* ... here is the expensive or blocking operation ... */
signals:
    void sigECTtransfer(unsigned char*,int);
    void sigTDlastransfer(unsigned char*,int );
signals:
    void rwcount();
    void readbuffer(double st);
public slots:
    void stoprun(bool flag);
private:
    CS_ftfunction *usb;
    QFile *datafile;
    QMutex *lock;
    bool runflag;

    uint count;//infinite how?
    const int bufferlong;
    quint8 shangweijibuffer[100];
    unsigned char * RxBuffer;
    QDataStream *infile;
    DWORD BytesReceived;
    LARGE_INTEGER litmp;
    LONGLONG Qpart1,Qpart2,Useingtime;
    double dfMinus,dfFreq,dfTime;
};
///
/// \brief The processThread class...........................
///
class processThread:public QThread
{
public:
    processThread(): QThread() {}
    void run();

};
///
/// \brief The argfordraw struct............................
///
struct argfordraw{
QVector<float> tran;
float  maxtransfer,mintransfer;
};
///
/// \brief The processThreadobj class............................
///
class processThreadobj : public QObject
{
    Q_OBJECT
public:
    enum ET{
        ECT,
        TDlas
    };
    ET mode;
    ECTClass *ect;
public:
    //二进制文件，buffer长度，标志，txt文件，mode
    explicit processThreadobj( QFile *df,const int bl,bool rf,QFile *tf,ET m)
        :datafile(df),bufferlong(bl),runflag(rf),txtfile(tf),mode(m),QObject(){

        //ZeroMemory(RxBuffer,bufferlong);
        count=0;times=5;

        txtfile->open(QIODevice::WriteOnly|QIODevice::Append|QIODevice::Text);
        trantextfile.setDevice(txtfile);

        ect=ECTClass::getInstance();
    }


signals:

    void readbuffer(quint16);

    void sigdrawECTusbdata(argfordraw *);

    void sigdrawECTonecircledata(argfordraw *);

    void sigdrawTDlasusbdata(argfordraw *);

private:
    QFile *datafile;
    bool runflag;
    const int bufferlong;
    char * RxBuffer;

    QDataStream outfile;
    DWORD BytesReceived;
    QFile *txtfile;
    QTextStream trantextfile;
    float transfer;

    int count,times;
    argfordraw tranarg,onecirclearg;

public slots:
    void transferforECTdrawing(unsigned char *buffer, int bufferlong);
    void transferforTDlasdrawing(unsigned char*buffer, int bufferlong);

};
#endif // MYTHREADS_H
