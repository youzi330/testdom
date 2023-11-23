客户端
widget.h
  #ifndef WIDGET_H
#define WIDGET_H
 
#include <QWidget>
#include <QAbstractSocket>
class QTcpSocket;
class QFile;
 
namespace Ui {
class Widget;
}
 
class Widget : public QWidget
{
    Q_OBJECT
 
public:
    explicit Widget(QWidget *parent = 0);
    ~Widget();
 
public slots:
    void openFile();
    void send();
    void startTransfer();
    void updateClientProgress(qint64);
    void displayError(QAbstractSocket::SocketError);
    void openBtnClicked();
    void sendBtnClicked();
 
private:
    Ui::Widget *ui;
    QTcpSocket *m_tcpClient;
    QFile *m_localFile;
    qint64 m_totalBytes;
    qint64 m_bytesWritten;
    qint64 m_bytesToWrite;
    qint64 m_payloadSize;
    QString m_fileName;
    QByteArray m_outBlock;
};
 
#endif // WIDGET_H


widget.cpp
  #include "widget.h"
#include "ui_widget.h"
#include <QtNetwork>
#include <QFileDialog>
 
Widget::Widget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Widget)
{
    ui->setupUi(this);
    this->setWindowTitle("CSDN IT1995");
    m_payloadSize=64*1024;
    m_totalBytes=0;
    m_bytesWritten=0;
    m_bytesToWrite=0;
    m_tcpClient=new QTcpSocket(this);
    connect(m_tcpClient,SIGNAL(connected()),this,SLOT(startTransfer()));
    connect(m_tcpClient,SIGNAL(bytesWritten(qint64)),this,SLOT(updateClientProgress(qint64)));
    connect(m_tcpClient,SIGNAL(error(QAbstractSocket::SocketError)),this,SLOT(displayError(QAbstractSocket::SocketError)));
    connect(ui->sendButton,SIGNAL(clicked(bool)),this,SLOT(sendBtnClicked()));
    connect(ui->openButton,SIGNAL(clicked(bool)),this,SLOT(openBtnClicked()));
}
 
void Widget::openFile(){
    m_fileName=QFileDialog::getOpenFileName(this);
    if(!m_fileName.isEmpty()){
        ui->sendButton->setEnabled(true);
        ui->clientStatusLabel->setText(QString("打开文件 %1 成功!").arg(m_fileName));
    }
}
 
void Widget::send(){
    ui->sendButton->setEnabled(false);
    m_bytesWritten=0;
    ui->clientStatusLabel->setText("连接中...");
    m_tcpClient->connectToHost(ui->hostLineEdit->text(),ui->portLineEdit->text().toInt());
}
 
void Widget::startTransfer(){
    m_localFile=new QFile(m_fileName);
    if(!m_localFile->open(QFile::ReadOnly)){
        qDebug()<<"client：open file error!";
        return;
    }
    m_totalBytes=m_localFile->size();
    QDataStream sendOut(&m_outBlock,QIODevice::WriteOnly);
    sendOut.setVersion(QDataStream::Qt_5_7);
    QString currentFileName=m_fileName.right(m_fileName.size()-m_fileName.lastIndexOf('/')-1);
    //文件总大小、文件名大小、文件名
    sendOut<<qint64(0)<<qint64(0)<<currentFileName;
    m_totalBytes+=m_outBlock.size();
    sendOut.device()->seek(0);
    sendOut<<m_totalBytes<<qint64(m_outBlock.size()-sizeof(qint64)*2);
    m_bytesToWrite=m_totalBytes-m_tcpClient->write(m_outBlock);
    ui->clientStatusLabel->setText("已连接");
    m_outBlock.resize(0);
}
 
void Widget::updateClientProgress(qint64 numBytes){
    m_bytesWritten+=(int)numBytes;
    if(m_bytesToWrite>0){
        m_outBlock=m_localFile->read(qMin(m_bytesToWrite,m_payloadSize));
        m_bytesToWrite-=(int)m_tcpClient->write(m_outBlock);
        m_outBlock.resize(0);
    }
    else{
        m_localFile->close();
    }
    ui->clientProgressBar->setMaximum(m_totalBytes);
    ui->clientProgressBar->setValue(m_bytesWritten);
 
    if(m_bytesWritten==m_totalBytes){
        ui->clientStatusLabel->setText(QString("传送文件 %1 成功").arg(m_fileName));
        m_localFile->close();
        m_tcpClient->close();
    }
}
 
void Widget::displayError(QAbstractSocket::SocketError){
    qDebug()<<m_tcpClient->errorString();
    m_tcpClient->close();
    ui->clientProgressBar->reset();
    ui->clientStatusLabel->setText("客户端就绪");
    ui->sendButton->setEnabled(true);
}
 
void Widget::openBtnClicked(){
    ui->clientProgressBar->reset();
    ui->clientStatusLabel->setText("状态：等待打开文件！");
    openFile();
}
 
void Widget::sendBtnClicked(){
    send();
}
 
Widget::~Widget()
{
    delete ui;
}

main.cpp
  #include "widget.h"
#include <QApplication>
 
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    Widget w;
    w.show();
 
    return a.exec();
}

服务端
widget.h
  #ifndef WIDGET_H
#define WIDGET_H
 
#include <QWidget>
#include <QAbstractSocket>
#include <QTcpServer>
class QTcpSocket;
class QFile;
 
 
namespace Ui {
class Widget;
}
 
class Widget : public QWidget
{
    Q_OBJECT
 
public:
    explicit Widget(QWidget *parent = 0);
    ~Widget();
 
public slots:
    void start();
    void acceptConnection();
    void updateServerProgress();
    void displayError(QAbstractSocket::SocketError socketError);
    void startBtnClicked();
 
private:
    Ui::Widget *ui;
 
    QTcpServer m_tcpServer;
    QTcpSocket *m_tcpServerConnection;
    qint64 m_totalBytes;
    qint64 m_bytesReceived;
    qint64 m_fileNameSize;
    QString m_fileName;
    QFile *m_localFile;
    QByteArray m_inBlock;
};
 
#endif // WIDGET_H

widget.cpp
  #include "widget.h"
#include "ui_widget.h"
#include <QtNetwork>
#include <QDebug>
 
Widget::Widget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Widget)
{
    ui->setupUi(this);
    this->setWindowTitle("CSDN IT1995");
    connect(&m_tcpServer,SIGNAL(newConnection()),this,SLOT(acceptConnection()));
    connect(ui->startButton,SIGNAL(clicked(bool)),this,SLOT(startBtnClicked()));
}
 
Widget::~Widget()
{
    delete ui;
}
 
void Widget::start(){
    if(!m_tcpServer.listen(QHostAddress::LocalHost,10086)){
        qDebug()<<m_tcpServer.errorString();
        close();
        return;
    }
    ui->startButton->setEnabled(false);
    m_totalBytes=0;
    m_bytesReceived=0;
    m_fileNameSize=0;
    ui->serverStatusLabel->setText("监听");
    ui->serverProgressBar->reset();
}
 
void Widget::acceptConnection(){
    m_tcpServerConnection=m_tcpServer.nextPendingConnection();
    connect(m_tcpServerConnection,SIGNAL(readyRead()),this,SLOT(updateServerProgress()));
    connect(m_tcpServerConnection,SIGNAL(error(QAbstractSocket::SocketError)),this,SLOT(displayError(QAbstractSocket::SocketError)));
    ui->serverStatusLabel->setText("接受连接");
    //关闭服务器不再监听，直接进入数据收发
    m_tcpServer.close();
}
 
void Widget::updateServerProgress(){
    QDataStream in(m_tcpServerConnection);
    in.setVersion(QDataStream::Qt_5_7);
    // 如果接收到的数据小于16个字节，保存到来的文件头结构
    if (m_bytesReceived<=sizeof(qint64)*2){
        if((m_tcpServerConnection->bytesAvailable()>=sizeof(qint64)*2)&&(m_fileNameSize==0)){
            // 接收数据总大小信息和文件名大小信息
            in>>m_totalBytes>>m_fileNameSize;
            m_bytesReceived +=sizeof(qint64)*2;
        }
        if((m_tcpServerConnection->bytesAvailable()>=m_fileNameSize)&&(m_fileNameSize!=0)){
            // 接收文件名，并建立文件
            in>>m_fileName;
            ui->serverStatusLabel->setText(tr("接收文件 %1 …").arg(m_fileName));
            m_bytesReceived+=m_fileNameSize;
            m_localFile = new QFile(m_fileName);
            if (!m_localFile->open(QFile::WriteOnly)){
                qDebug() << "server: open file error!";
                return;
            }
        }
        else{
            return;
        }
    }
    // 如果接收的数据小于总数据，那么写入文件
    if(m_bytesReceived<m_totalBytes) {
        m_bytesReceived+=m_tcpServerConnection->bytesAvailable();
        m_inBlock = m_tcpServerConnection->readAll();
        m_localFile->write(m_inBlock);
        m_inBlock.resize(0);
    }
    ui->serverProgressBar->setMaximum(m_totalBytes);
    ui->serverProgressBar->setValue(m_bytesReceived);
 
    // 接收数据完成时
    if (m_bytesReceived==m_totalBytes){
        m_tcpServerConnection->close();
        m_localFile->close();
        ui->startButton->setEnabled(true);
        ui->serverStatusLabel->setText(tr("接收文件 %1 成功！").arg(m_fileName));
    }
}
 
void Widget::displayError(QAbstractSocket::SocketError socketError){
    Q_UNUSED(socketError)
    qDebug()<<m_tcpServerConnection->errorString();
    m_tcpServerConnection->close();
    ui->serverProgressBar->reset();
    ui->serverStatusLabel->setText("服务端就绪");
    ui->startButton->setEnabled(true);
}
 
void Widget::startBtnClicked(){
    start();
}

main.cpp
  #include "widget.h"
#include <QApplication>
 
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    Widget w;
    w.show();
 
    return a.exec();
}
