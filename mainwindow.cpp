#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFileDialog>
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setFixedSize(300,200);
    start = false;
    migration = 0;
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::StartToEnd()
{
    start = false;
    ui->startPushButton->
            setStyleSheet("border-image: url(:/start.ico)" );
    ui->intervalComboBox->setEnabled(true);
    ui->chooseButton->setEnabled(true);
}

void MainWindow::EndToStart()
{
    start = true;
    ui->startPushButton->
            setStyleSheet("border-image: url(:/pause.ico)" );
    ui->intervalComboBox->setEnabled(false);
    ui->chooseButton->setEnabled(false);
}

void MainWindow::on_startPushButton_clicked()
{
    if(!start)
    {
        QString directory = ui->dircetoryLabel->text();
        int interval = ui->intervalComboBox->currentText().toInt();
        if(directory.isNull())
        {
            QMessageBox::information(this,"Tip"
                                     ,"Please select a directory!"
                                     ,QMessageBox::Ok);
            return;
        }
        EndToStart();
        migration = new Migration(directory,interval);
        migration->Start();
    }
    else
    {
        StartToEnd();
        if(migration!=0)
        {
            migration->Stop();
            //delete migration;//不能删除，还有收尾工作
        }
    }
}

void MainWindow::on_chooseButton_clicked()
{
    QString directory = QFileDialog::getExistingDirectory(this,"Choose Location","c:/");
    if(!directory.isNull())
    {
        ui->dircetoryLabel->setText(directory);
    }
}
