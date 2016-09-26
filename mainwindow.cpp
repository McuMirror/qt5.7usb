#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>
#include "tdlasdialog.h"


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    createaction();
    createmenus();  //菜单栏
    createToolBars();//工具栏
    createdockwidget();//停靠部件

    datafile=NULL;
    rwthread1=NULL;
    rwthread2=NULL;
    savedirectory="./";

    mode=ECT;
    ECT_action->setCheckable(true);
    ECT_action->setChecked(true);
    tdlas_action->setChecked(false);
    stopdataacquisition_action->setEnabled(false);

    QHBoxLayout *mainlayout=new QHBoxLayout(this);
    paintusb=new myPaintusb(ui->frame);
    mainlayout->addWidget(paintusb);
}

MainWindow::~MainWindow()
{
    delete ui;
}


void MainWindow::on_open_clicked()
{
    QString num;
    num=QString::number( usb.getnumDEv());
    ui->listWidget_2->addItem(num);

    QString label;
    if(!usb.isopened()){
        label+=usb.status.at(usb.Open(0));
        ui->listWidget_2->addItem(label);
    }
    else
        ui->listWidget_2->addItem("device 0 is allrdeay opened");
}

void MainWindow::on_close_clicked()
{
    QString label;
    if(usb.isopened()){

        label+=usb.status.at(usb.Close());
        ui->listWidget_2->addItem(label);
    }
    else
        ui->listWidget_2->addItem("device 0 is allrdeay closed");
}

void MainWindow::openusb()//closeusb action
{
    statusdock->setVisible(true);//使停靠栏可见

    QString num;
    num=QString::number( usb.getnumDEv());
    num+=" devices";
    ui->listWidget_2->addItem(num);

    QString label;
    if(!usb.isopened()){
        label+=usb.status.at(usb.Open(0));
        ui->listWidget_2->addItem(label);
    }
    else
        ui->listWidget_2->addItem("device 0 is allrdeay opened");

    ui->listWidget_2->scrollToBottom();
}

void MainWindow::closeusb()//closeusb action
{
    statusdock->setVisible(true);//使停靠栏可见

    QString label;
    if(usb.isopened()){

        label+=usb.status.at(usb.Close());
        ui->listWidget_2->addItem(label);
    }
    else
        ui->listWidget_2->addItem("device 0 is allrdeay closed");

    ui->listWidget_2->scrollToBottom();
}

void MainWindow::tdlas()
{
    mode=TDlas;

    tdlas_action->setChecked(true);
    ECT_action->setChecked(false);

    createtdlasview();

    tdlasDialog tdlasdl;
    connect(this,&MainWindow::transmitusb,&tdlasdl,&tdlasDialog::acceptusb);
    connect(&tdlasdl,&tdlasDialog::mystatus,this,&MainWindow::childrenWidstatus);
    emit transmitusb(&usb);
    tdlasdl.exec();
}

void MainWindow::Ect()
{
    mode=ECT;

    ECT_action->setChecked(true);

    createectview();

    tdlas_action->setChecked(false);
}

/// \brief MainWindow::dataacquisition
///

void MainWindow::dataacquisition()//数据采集 action
{
    statusdock->setVisible(true);//使停靠栏可见

    if(1){//usb.getnumDEv()
        if(1){//usb.isopened()
            QFileDialog::Options options = QFileDialog::DontResolveSymlinks | QFileDialog::ShowDirsOnly;
            //options |= QFileDialog::DontUseNativeDialog;
            currentdirectory = QFileDialog::getExistingDirectory(this,
                                                                      tr("Save Directory"),
                                                                      "",
                                                                      options);
            if (currentdirectory.isEmpty()){
                qDebug()<<"err";
                return;
            }
            else{
                savedirectory=currentdirectory;

                if(savedirectory=="C:/"){
                    ui->listWidget_2->addItem("do not use C:/ as a saving position");
                    ui->listWidget_2->addItem("change dir and try again");
                    return;
                }
            }
        }
        else{
            ui->listWidget_2->addItem("err: ft_device's found, open it first");
        }

    }
    else{
        ui->listWidget_2->addItem("err: find a ft_device first");
    }

    ui->listWidget_2->scrollToBottom();

}

void MainWindow::stopdataacquisition()//停止数据采集 action
{
    stopdataacquisition_action->setEnabled(false);

    //RunSyn=0;
    emit stopacquisition1(false);
    emit stopacquisition2(false);
    if(datafile!=NULL&&rwthread1!=NULL){

        processthread->quit();
        rwthread1->wait();
        rwthread2->wait();

        datafile->close();
        txtfile->flush();
        txtfile->close();

        QByteArray TxBuffer;//准备写入usb的停止发送命令
        TxBuffer.clear();
        QDataStream in(&TxBuffer, QIODevice::ReadWrite);
        DWORD BytesReceived;//向下写入发送命令
        if(mode==TDlas){
            ;
        }
        if(mode==ECT){
            in<<(quint8)0x77;
            qDebug()<<TxBuffer.data();
            if(usb.Write(TxBuffer.data(),1,&BytesReceived)==FT_OK&&BytesReceived==1)
                ui->listWidget_2->addItem("停止发送命令写入成功");
            qDebug()<<BytesReceived;
        }
    }

    startdataacquisition_action->setEnabled(true);
}

void MainWindow::startdataacquisition()//开始采集
{
    if(1){//usb.isopened()

        startdataacquisition_action->setEnabled(false);
        ui->listWidget_2->addItem("save at "+savedirectory);

        QDateTime datetime=QDateTime::currentDateTime();//准备文件二进制
        QString dt= datetime.toString("yyyy-MM-dd_HH.mm.ss");
        QDir::setCurrent(savedirectory);
        if(mode==ECT)datafile=new QFile("ECT_"+dt+"_acquisition.bin");//准备文件
        if(mode==TDlas)datafile=new QFile("TDlas_"+dt+"_acquisition.bin");//准备文件
        //datafile->open(QIODevice::ReadWrite|QIODevice::Append|QIODevice::Truncate);
         datetime=QDateTime::currentDateTime();//准备文件txt
         dt= datetime.toString("yyyy-MM-dd_HH.mm.ss");
        QDir::setCurrent(savedirectory);
        if(mode==ECT)txtfile=new QFile("ECT_"+dt+"_acquisition.txt");
        if(mode==TDlas)txtfile=new QFile("TDlas_"+dt+"_acquisition.txt");
        txtfile->open(QIODevice::ReadWrite|QIODevice::Append|QIODevice::Truncate);//准备文件


        lockthread=new QMutex;
        RWThread::ET m;processThreadobj::ET mm;
        if(mode==ECT){m=RWThread::ECT;mm=processThreadobj::ECT;}
        if(mode==TDlas){m=RWThread::TDlas;mm=processThreadobj::TDlas;}
        //THread 1
        rwthread1= new RWThread(&usb,datafile,4096,true,lockthread,m);
        connect(rwthread1,&RWThread::readbuffer,this,&MainWindow::threadstatus);
        connect(this,&MainWindow::stopacquisition1,rwthread1,&RWThread::stoprun,Qt::DirectConnection);
        connect(rwthread1, &RWThread::finished, rwthread1, &QObject::deleteLater);
        connect(rwthread1, &RWThread::finished, this, &MainWindow::setrwthread1null);
        connect(rwthread1, &RWThread::finished, this, &MainWindow::deletemylock);
        //Thread2
        rwthread2= new RWThread(&usb,datafile,4096,true,lockthread,m);
        connect(rwthread2,&RWThread::readbuffer,this,&MainWindow::threadstatus);
        connect(this,&MainWindow::stopacquisition2,rwthread2,&RWThread::stoprun,Qt::DirectConnection);
        connect(rwthread2, &RWThread::finished, rwthread2, &QObject::deleteLater);
        connect(rwthread2, &RWThread::finished, this, &MainWindow::setrwthread2null);
        connect(rwthread2, &RWThread::finished, this, &MainWindow::deletemylock);
        //processThread
        processthread=new processThread();
        processthreadobj=new processThreadobj(datafile,4096,true,txtfile,mm);
        processthreadobj->moveToThread(processthread);
        connect(rwthread1,&RWThread::sigECTtransfer,processthreadobj,&processThreadobj::transferforECTdrawing);
        connect(rwthread1,&RWThread::sigTDlastransfer,processthreadobj,&processThreadobj::transferforTDlasdrawing);

        connect(rwthread2,&RWThread::sigECTtransfer,processthreadobj,&processThreadobj::transferforECTdrawing);
        connect(rwthread2,&RWThread::sigTDlastransfer,processthreadobj,&processThreadobj::transferforTDlasdrawing);
        //connect(rwthread2, &RWThread::finished, processthread, &QThread::quit);

        connect(processthreadobj,&processThreadobj::sigdrawECTusbdata,this,&MainWindow::drawECTusbdata);
        connect(processthreadobj,&processThreadobj::sigdrawTDlasusbdata,this,&MainWindow::drawTDlasusbdata);
        connect(processthread,&QThread::finished, processthread, &QObject::deleteLater);
        connect(processthread,&QThread::finished, processthreadobj, &QObject::deleteLater,Qt::DirectConnection);



        QByteArray TxBuffer;//准备工作：写入usb的发送命令
        TxBuffer.clear();
        QDataStream in(&TxBuffer, QIODevice::ReadWrite);
        DWORD BytesReceived;//end准备

        if(mode==TDlas){
            in<<(quint16)0x8800<<(quint16)4096;
            if(usb.Write(TxBuffer.data(),4,&BytesReceived)==FT_OK&&BytesReceived==4)
                ui->listWidget_2->addItem("发送命令写入成功");
            qDebug()<<BytesReceived;
            UCHAR MASK = 0xff;
            UCHAR MODE = 0x40;
            usb.SetBitMode(MASK, MODE);
            usb.SetTimeouts(5000,1000);// Set read timeout of 5sec, write timeout of 1sec

        }
        if(mode==ECT){
            in<<(quint8)0x88;
            qDebug()<<TxBuffer.data();
            if(usb.Write(TxBuffer.data(),1,&BytesReceived)==FT_OK&&BytesReceived==1)
                ui->listWidget_2->addItem("发送命令写入成功");
            qDebug()<<BytesReceived;
            usb.SetTimeouts(5000,1000);// Set read timeout of 5sec, write timeout of 1sec
        }


        rwthread1->start();//开始线程吧
        rwthread2->start();//开始线程吧
        processthread->start();//开始线程吧

        stopdataacquisition_action->setEnabled(true);
    }
    else
        ui->listWidget_2->addItem("open usb first");
    ui->listWidget_2->scrollToBottom();
}

void MainWindow::reconstruct()
{
    statusdock->hide();
}

void MainWindow::drawECTusbdata(argfordraw *arg)
{
//    QString s;
//    s.setNum(arg->tran.at(1000));
//    ui->listWidget_2->addItem(s);
//    ui->listWidget_2->scrollToBottom();
    paintusb->update();

}

void MainWindow::drawTDlasusbdata(argfordraw *arg)
{

}

void MainWindow::setrwthread1null()
{
    rwthread1=NULL;
}

void MainWindow::setrwthread2null()
{
    rwthread2=NULL;
}

void MainWindow::deletemylock()
{
    delete lockthread;
}

void MainWindow::threadstatus(double st)//slot
{
    QString label;
    label.setNum(st);
    ui->listWidget_2->addItem(label);
    ui->listWidget_2->scrollToBottom();

}

void MainWindow::childrenWidstatus(QString &str)//查看 子窗口 状态
{
    ui->listWidget_2->addItem(str);
    ui->listWidget_2->scrollToBottom();
}


///
/// \brief MainWindow::creating...............
///
void MainWindow::createToolBars()
{
    toolmode=addToolBar("mode");
    toolmode->addAction(ECT_action);
    toolmode->addAction(tdlas_action);

    toolusb=addToolBar("usb");
    toolusb->addAction(openusb_action);
    toolusb->addAction(closeusb_action);


    tooldataacquisition=addToolBar("data_acquisition");
    tooldataacquisition->addAction(dataacquisition_action);
    tooldataacquisition->addAction(startdataacquisition_action);
    tooldataacquisition->addAction(stopdataacquisition_action);

    toolreconstruction=addToolBar("reconstruction");
    toolreconstruction->addAction(reconstruct_action);
    //insert here

}

void MainWindow::createaction()
{
    openusb_action= new QAction("openusb",this);
    connect(openusb_action,SIGNAL(triggered()),this,SLOT(openusb()));

    closeusb_action=new QAction("closeusb",this);
    connect(closeusb_action,SIGNAL(triggered()),this,SLOT(closeusb()));

    tdlas_action=new QAction("tdlas",this);
    connect(tdlas_action,SIGNAL(triggered()),this,SLOT(tdlas()));
    tdlas_action->setCheckable(true);

    ECT_action= new QAction("ECT",this);
    connect(ECT_action,SIGNAL(triggered()),this,SLOT(Ect()));
    ECT_action->setCheckable(true);

    dataacquisition_action=new QAction("Dir of data acquisition",this);
    connect(dataacquisition_action,SIGNAL(triggered()),this,SLOT(dataacquisition()));

    stopdataacquisition_action= new QAction("stop",this);
    connect(stopdataacquisition_action,SIGNAL(triggered()),this,SLOT(stopdataacquisition()));

    startdataacquisition_action= new QAction("start",this);
    connect(startdataacquisition_action,SIGNAL(triggered()),this,SLOT(startdataacquisition()));

    reconstruct_action=new QAction("reconstruct",this);
    connect(reconstruct_action,SIGNAL(triggered()),SLOT(reconstruct()));

    LBP=new QAction("LBP",this);

    caldelong=new QAction("caldelong",this);
}

void MainWindow::createmenus()//insert codes
{
    ui->menu->addAction(tdlas_action);
    ui->menu->addAction(ECT_action);
    ui->menu->addAction(openusb_action);

    menualgorithm=new QMenu("algorithm");
    ui->menureconstruct->addMenu(menualgorithm);
    ui->menureconstruct->addAction(reconstruct_action);
    menualgorithm->addAction(LBP);
    menualgorithm->addAction(caldelong);
}

void MainWindow::createdockwidget()
{
    statusdock=new QDockWidget(tr("status"),this);
    statusdock->setWidget(ui->listWidget_2);
    addDockWidget(Qt::RightDockWidgetArea,statusdock);
}

void MainWindow::createtdlasview()
{

}

void MainWindow::createectview()
{

}


