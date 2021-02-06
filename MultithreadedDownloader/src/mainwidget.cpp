/**
 * @brief MainWidget
 * @anchor Ho229<2189684957@qq.com>
 * @date 2021/2/1
 */

#include "mainwidget.h"
#include "ui_mainwidget.h"

#include <QFileDialog>
#include <QMessageBox>

MainWidget::MainWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::MainWidget),
      m_downloader(new MultithreadedDownloader(this)),
      m_toast(new Toast(this))
{
    ui->setupUi(this);

    this->initUI();
    this->initSignalSlots();
}

MainWidget::~MainWidget()
{
    delete ui;
}

void MainWidget::initDownloadUrl(const QString &url)
{
    if(url.isEmpty())
        return;

    ui->urlEdit->setPlainText(url);
    this->on_downloadBtn_clicked();
}

void MainWidget::initUI()
{
    ui->failedLabel->hide();
    ui->retranslateUi(this);
}

void MainWidget::initSignalSlots()
{
    QObject::connect(m_downloader, &MultithreadedDownloader::finished,
                     [this]{
        QMessageBox::information(this, tr("infomation"), tr("download finished."));
        ui->stackedWidget->moveToIndex(0);
    });

    QObject::connect(m_downloader, &MultithreadedDownloader::error,
                     [this](MultithreadedDownloader::Error err){
        if(err == MultithreadedDownloader::OpenFileFailed)
            QMessageBox::critical(this, tr("error"), tr("File can not open."));
        else
            QMessageBox::critical(this, tr("error"),
                                  tr("Download Failed.\nNetwork Error Code:")
                                  .append(m_downloader->networkErrorString()));

        this->on_stopBtn_clicked();
    });

    QObject::connect(m_downloader, &MultithreadedDownloader::stateChanged,
                     [this](MultithreadedDownloader::State state){
        switch (state)
        {
        case MultithreadedDownloader::Running:
            ui->stateLabel->setText(tr("Running"));
            break;
        case MultithreadedDownloader::Paused:
            ui->stateLabel->setText(tr("Paused"));
            break;
        case MultithreadedDownloader::Stopped:
            ui->stateLabel->setText(tr("Stopped"));
            break;
        }
    });

    QObject::connect(m_downloader, &MultithreadedDownloader::downloadProgress,
                     [this](qint64 bytesReceived, qint64 bytesTotal){
        ui->progressBar->setValue(static_cast<int>
                                 (static_cast<qreal>(bytesReceived) / bytesTotal * 100));
        ui->byteLabel->setText(tr("Received: %1 / Total: %2 (Byte)")
                               .arg(bytesReceived).arg(bytesTotal));
    });
}

void MainWidget::on_downloadBtn_clicked()
{
    ui->retranslateUi(this);
    QString url;
    if((url = ui->urlEdit->toPlainText()).isEmpty())
    {
        m_toast->toast(tr("URL is empty."));
        return;
    }

    m_downloader->setUrl(url);
    if(m_downloader->getFileInfo())
    {
        ui->failedLabel->hide();

        QString dir = QFileDialog::getExistingDirectory(this,
                tr("Please select the download directory"), QDir::homePath());

        if(dir.isEmpty())
            return;

        ui->fileNameLabel->setText(m_downloader->fileName());
        ui->startBtn->setEnabled(false);
        ui->pauseBtn->setEnabled(true);
        ui->stopBtn->setEnabled(true);
        m_toast->toast(tr("Download started."));

        ui->stackedWidget->moveToIndex(1);

        m_downloader->setDownloadDir(dir);
        m_downloader->start();
    }
    else
        ui->failedLabel->show();
}

void MainWidget::on_startBtn_clicked()
{
    ui->startBtn->setEnabled(false);
    ui->pauseBtn->setEnabled(true);
    ui->stopBtn->setEnabled(true);

    m_downloader->start();
    m_toast->toast(tr("Download started."));
}

void MainWidget::on_pauseBtn_clicked()
{
    ui->startBtn->setEnabled(true);
    ui->pauseBtn->setEnabled(false);
    ui->stopBtn->setEnabled(true);

    m_downloader->pause();
    m_toast->toast(tr("Download has been pause."));
}

void MainWidget::on_stopBtn_clicked()
{
    m_downloader->stop();
    m_toast->toast(tr("Download terminated."));
    ui->stackedWidget->moveToIndex(0);
}
