#include "widget.h"
#include "ui_widget.h"

#include <QMessageBox>
#include <QApplication>
#include <QVarLengthArray>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QPushButton>
#include <QHBoxLayout>
#include <QButtonGroup>
#include <QTextStream>

#include "literal_strings/literal_strings.h"
#include "common_tools/common_tool_func.h"
#include "logger/logger.h"

const quint8 g_sptrum_threshold_num = 3;
const int g_channel_per_card = 64;

static const int g_min_eng_sptrm_val = 0;
static const int g_max_eng_sptrm_val = 1000;

static const char* g_data_save_folder = "./scan_result";
static const char* g_eng_sptrm_scan_data_bfn = "eng_sptrum_data";
static const char* g_eng_sptrm_scan_data_ext = ".csv";

static const char* g_eng_sptrm_scan_data_raw_bfn = "eng_sptrum_raw";
static const char* g_eng_sptrm_scan_data_raw_ext = ".raw";

Widget::Widget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Widget)
{
    ui->setupUi(this);


    QButtonGroup *work_mode_btn_grp = new QButtonGroup(this);
    work_mode_btn_grp->addButton(ui->manTimerScanRBtn);
    work_mode_btn_grp->addButton(ui->engSptrmScanRBtn);

    ui->engSptrmScanRBtn->setChecked(true);
    ui->startEngSpinBox->setMinimum(g_min_eng_sptrm_val);
    ui->startEngSpinBox->setMaximum(g_max_eng_sptrm_val);
    ui->endEngSpinBox->setMinimum(g_min_eng_sptrm_val);
    ui->endEngSpinBox->setMaximum(g_max_eng_sptrm_val);
    ui->stepEngSpinBox->setMinimum(-1 * g_max_eng_sptrm_val);
    ui->stepEngSpinBox->setMaximum(g_max_eng_sptrm_val);

    m_ui_cfg_rec.load_configs_to_ui(this, m_ui_cfg_rec_filter_in, m_ui_cfg_rec_filter_out);

    this->InitMember();
    this->InitUI();
    this->InitConnect();

    m_init_ok = true;
}

Widget::~Widget()
{
    m_ui_cfg_rec.record_ui_configs(this);

    delete ui;
    delete l103Controller;
}

void Widget::InitMember()
{
    // 初始化帧大小
    frameSize = 64;
    
    // 初始化灰度拉伸上限
    greyScaleMax = 4000;
    
    // 创建探测器库对象
    l103Controller = new L103Controller(frameSize, 5000);

    // 初始化探测器库
    if (!l103Controller->Init())
    {
        QMessageBox::critical(this, "初始化失败", "探测器初始化发生错误, 错误代码为" + QString::number(static_cast<quint8>(l103Controller->GetLastError())));
        std::exit(-1);
    }

    // 初始化卡数
    if (!l103Controller->GetCardNum(cardNum))
    {
        QMessageBox::critical(this, "初始化失败", "读取板卡数发生错误, 错误代码为" + QString::number(static_cast<quint8>(l103Controller->GetLastError())));
        std::exit(-1);
    }

    // 初始化单次展示帧数
    double width = cardNum * 64.0 * 611 / 131;
    displayNum = static_cast<quint16>(width / frameSize);
    if (displayNum == 0) displayNum = 1;
    
    // 初始化自动停止定时器
    autoStopTimer = new QTimer(this);
    autoStopTimer->setSingleShot(true);
}

void Widget::InitUI()
{
    // ************ 初始化阈值表 ************

    // 设置列表头
    ui->tableWidget->setColumnCount(cardNum * 64);
    for (quint16 card = 0; card < cardNum; ++card) {
        for (int channel = 0; channel < 64; ++channel) {
            int col = card * 64 + channel;
            ui->tableWidget->setHorizontalHeaderItem(col, new QTableWidgetItem(QString("%1-%2").arg(card + 1).arg(channel + 1)));
        }
    }

    // 初始化值
    for (int threshold = 0; threshold < 3; ++threshold)
    {
        QList<std::array<quint8,64>> thresholdList;
        l103Controller->GetThresholdList(threshold+1, thresholdList);
        for (int card = 0; card < thresholdList.size(); ++card) {
            for (int channel = 0; channel < 64; ++channel) {
                int col = card * 64 + channel;
                quint8 value = thresholdList[card][channel];
                ui->tableWidget->setItem(threshold, col, new QTableWidgetItem(QString::number(value)));
            }
        }
    }

    // ************ 初始化其他控件 ************

    // 设置初始帧大小
    ui->lineEdit->setText(QString::number(frameSize));
}

void Widget::InitConnect()
{
    // 连接 button -> this
    connect(ui->pushButton, &QPushButton::clicked, this, &Widget::OnStartScan);
    connect(ui->pushButton_2, &QPushButton::clicked, this, &Widget::OnStopScan);
    connect(ui->pushButton_4, &QPushButton::clicked, this, &Widget::OnSetFrameLineNum);
    connect(ui->pushButton_3, &QPushButton::clicked, this, &Widget::OnSetThreshold1);
    connect(ui->pushButton_6, &QPushButton::clicked, this, &Widget::OnSetThreshold2);
    connect(ui->pushButton_7, &QPushButton::clicked, this, &Widget::OnSetThreshold3);
    connect(ui->pushButton_5, &QPushButton::clicked, this, &Widget::OnSaveImage);

    // 连接 slider -> this
    connect(ui->verticalSlider, &QSlider::valueChanged, this, &Widget::OnGreyScaleChanged);

    // 连接 L103Controller -> this
    connect(l103Controller, &L103Controller::SigFrameReady, this, &Widget::OnFrameReady);
    
    // 连接定时器 -> this
    connect(autoStopTimer, &QTimer::timeout, this, &Widget::OnAutoStopTimeout);


    connect(this, &Widget::scan_next_eng_sptrm_sig,
            this, &Widget::scan_next_eng_sptrm_hdlr, Qt::QueuedConnection);
    connect(this, &Widget::eng_sptrm_scan_finished_sig,
            this, &Widget::eng_sptrm_scan_finished_hdlr, Qt::QueuedConnection);
}

void Widget::OnStartScan()
{
    if(WORK_MODE_ENG_SPTRM_SCAN == current_work_mode())
    {
        QString err_str;
        if(!get_eng_sptrm_scan_info(&err_str))
        {
            QMessageBox::critical(this, "频谱扫描信息错误", err_str);
            return;
        }

        ui->scanStatusLbl->setText("Preparing...");
        ui->scanStatusLbl->repaint();
        m_preparing = true;
        if(!prepare_eng_sptrm_scan()) //this take long time, so we add a flag for protecting.
        {
            err_str = "准备期间设置能谱阈值错误！";
            DIY_LOG(LOG_ERROR, err_str);
            QMessageBox::critical(this, "Error", err_str);
            m_preparing = false;
            return;
        }
        m_preparing = false;

        err_str = QString("eng sptrm start: %1, end: %2, step: %3").arg(m_eng_sptrm_scan_info.start)
                .arg(m_eng_sptrm_scan_info.end).arg(m_eng_sptrm_scan_info.step);
        DIY_LOG(LOG_INFO, err_str);
        ui->scanStatusLbl->setText(err_str);

        m_eng_sptrm_raw_data.clear();
        DIY_LOG(LOG_INFO, QString("engergy sptrum %1 scan start").arg(m_eng_sptrm_scan_info.current));
        emit scan_next_eng_sptrm_sig();
    }
    else
    {
        if (!l103Controller->SetStartScanning())
        {
            QMessageBox::critical(this, "启动失败", "启动扫描发生错误, 错误代码为" + QString::number(static_cast<quint8>(l103Controller->GetLastError())));
        }
    }
    
    update_ui_for_eng_sptrm_scan(true);

    // 清空累积缓冲区
    accumulatedBuffer1.clear();
    accumulatedBuffer2.clear();
    accumulatedBuffer3.clear();

    // 禁用灰度滑块和保存按钮
    ui->verticalSlider->setEnabled(false);
    ui->pushButton_5->setEnabled(false);
    
    if(WORK_MODE_MAN_TIMER_SCAN == current_work_mode())
    {
        // 启动自动停止定时器
        autoStopTimer->setInterval(10000);
        autoStopTimer->start();
    }
}

void Widget::OnStopScan()
{
    if(m_preparing) return;

    if(WORK_MODE_ENG_SPTRM_SCAN == current_work_mode())
    {
        emit eng_sptrm_scan_finished_sig();
    }
    else
    {
        // 停止自动停止定时器
        autoStopTimer->stop();

        if (!l103Controller->SetStopScanning())
        {
            QMessageBox::critical(this, "停止失败", "停止扫描发生错误, 错误代码为" + QString::number(static_cast<quint8>(l103Controller->GetLastError())));
        }
    }
    update_ui_for_eng_sptrm_scan(false);

    // 恢复灰度滑块和保存按钮
    ui->verticalSlider->setEnabled(true);
    ui->pushButton_5->setEnabled(true);
}

void Widget::OnSetFrameLineNum()
{
    if(WORK_MODE_MAN_TIMER_SCAN == current_work_mode())
    {
        // 停止自动停止定时器
        autoStopTimer->stop();
    }

    // 暂停图像接收
    l103Controller->blockSignals(true);

    quint16 tmp_f_size = ui->lineEdit->text().toUShort();
    // 设置帧大小
    if (!l103Controller->SetFrameSize(tmp_f_size))
    {
        QMessageBox::critical(this, "设置失败", "设置帧大小发生错误, 错误代码为" + QString::number(static_cast<quint8>(l103Controller->GetLastError())));
        ui->lineEdit->setText(QString::number(frameSize));
        l103Controller->blockSignals(false);
        return;
    }

    // 更新帧大小
    frameSize = ui->lineEdit->text().toUShort();

    // 更新单次展示帧数
    double width = cardNum * 64.0 * 611 / 131;
    displayNum = static_cast<quint16>(width / frameSize);
    if (displayNum == 0) displayNum = 1;

    // 清空图像历史缓冲区
    image1History.clear();
    image2History.clear();
    image3History.clear();
    
    // 清空累积缓冲区
    accumulatedBuffer1.clear();
    accumulatedBuffer2.clear();
    accumulatedBuffer3.clear();

    // 恢复图像接收
    l103Controller->blockSignals(false);

    if(WORK_MODE_MAN_TIMER_SCAN == current_work_mode())
    {
        // 恢复自动停止定时器
        autoStopTimer->start();
    }
}

void Widget::OnSetThreshold1()
{
    if (!l103Controller->SetThreshold(1, ui->lineEdit_2->text().toUShort()))
    {
        QMessageBox::critical(this, "设置失败", "设置阈值1发生错误, 错误代码为" + QString::number(static_cast<quint8>(l103Controller->GetLastError())));
        return;
    }

    // 更新阈值表显示
    update_th_list_display(1);
}

void Widget::OnSetThreshold2()
{
    if (!l103Controller->SetThreshold(2, ui->lineEdit_4->text().toUShort()))
    {
        QMessageBox::critical(this, "设置失败", "设置阈值2发生错误, 错误代码为" + QString::number(static_cast<quint8>(l103Controller->GetLastError())));
        return;
    }

    // 更新阈值表显示
    update_th_list_display(2);
}

void Widget::OnSetThreshold3()
{
    if (!l103Controller->SetThreshold(3, ui->lineEdit_5->text().toUShort()))
    {
        QMessageBox::critical(this, "设置失败", "设置阈值3发生错误, 错误代码为" + QString::number(static_cast<quint8>(l103Controller->GetLastError())));
        return;
    }

    // 更新阈值表显示
    update_th_list_display(3);
}

void Widget::update_th_list_display(quint8 th_idx)
{
    if(!m_init_ok) return;

    QList<std::array<quint8,64>> thresholdList;
    if(!l103Controller->GetThresholdList(th_idx, thresholdList))
    {
        QString err_str = QString("获取阈值%1发生错误, 错误代码为%2").arg(th_idx)
                .arg(static_cast<quint8>(l103Controller->GetLastError()));
        DIY_LOG(LOG_ERROR, err_str);
        return;
    }
    for (int card = 0; card < thresholdList.size(); ++card) {
        for (int channel = 0; channel < 64; ++channel) {
            int col = card * 64 + channel;
            quint8 value = thresholdList[card][channel];
            ui->tableWidget->setItem(th_idx - 1, col, new QTableWidgetItem(QString::number(value)));
        }
    }
}

void Widget::OnGreyScaleChanged(int value)
{
    // 更新灰度拉伸上限
    greyScaleMax = static_cast<quint16>(value * 100);
    
    // 重绘图像
    if (!image1History.empty())
    {
        DisplayImage();
    }
}

void Widget::OnFrameReady(quint16 *data)
{
    quint16 pixelNumPerLine = cardNum * 64;
    quint32 elementsPerFrame = pixelNumPerLine * frameSize;

    if(WORK_MODE_ENG_SPTRM_SCAN == current_work_mode())
    {
        QString err_str ;
        if (!l103Controller->SetStopScanning())
        {
            emit eng_sptrm_scan_finished_sig();

            err_str = QString("Stop scanning error at eng-level %1, err code: %2")
                    .arg(m_eng_sptrm_scan_info.current).arg(static_cast<quint8>(l103Controller->GetLastError()));
            DIY_LOG(LOG_ERROR, err_str);
            QMessageBox::critical(this, "停止失败",  err_str);
        }

        QVector<acc_px_data_type> one_eng_sptrm_data(pixelNumPerLine, 0);
        acc_px_data_type * tgt = one_eng_sptrm_data.data();

        for (quint16 y = 0; y < frameSize; ++y)
        {
            for(quint16 p = 0; p < pixelNumPerLine; ++p)
            {
                /*NOTICE:
                 * the data collected here should be consistent with the set in
                 * scan_next_eng_sptrm_hdlr
                 */
                tgt[p] += (*(data
                    + (y * g_sptrum_threshold_num + g_sptrum_threshold_num - 1) * pixelNumPerLine
                             + p));
            }
        }
        m_eng_sptrm_raw_data.append(one_eng_sptrm_data);
        err_str = QString("%1 eng-level data recorded.").arg(m_eng_sptrm_scan_info.current);
        DIY_LOG(LOG_INFO, err_str);
        ui->scanStatusLbl->setText(err_str);

        inc_curr_eng_sptrm_scan_lvl();
        emit scan_next_eng_sptrm_sig();

        //return;
    }

    // 追加到三个累积缓冲区
    int oldSize1 = accumulatedBuffer1.size();
    int oldSize2 = accumulatedBuffer2.size();
    int oldSize3 = accumulatedBuffer3.size();

    accumulatedBuffer1.resize(oldSize1 + elementsPerFrame);
    accumulatedBuffer2.resize(oldSize2 + elementsPerFrame);
    accumulatedBuffer3.resize(oldSize3 + elementsPerFrame);

    quint16 *dst1 = accumulatedBuffer1.data() + oldSize1;
    quint16 *dst2 = accumulatedBuffer2.data() + oldSize2;
    quint16 *dst3 = accumulatedBuffer3.data() + oldSize3;

    // 拷贝数据到累积缓冲区
    for (quint16 y = 0; y < frameSize; ++y)
    {
        memcpy(dst1 + y * pixelNumPerLine, data + y * pixelNumPerLine * 3, pixelNumPerLine * sizeof(quint16));
        memcpy(dst2 + y * pixelNumPerLine, data + y * pixelNumPerLine * 3 + pixelNumPerLine, pixelNumPerLine * sizeof(quint16));
        memcpy(dst3 + y * pixelNumPerLine, data + y * pixelNumPerLine * 3 + pixelNumPerLine * 2, pixelNumPerLine * sizeof(quint16));
    }

    // 创建单帧图像
    QImage image1(pixelNumPerLine, frameSize, QImage::Format_Grayscale16);
    QImage image2(pixelNumPerLine, frameSize, QImage::Format_Grayscale16);
    QImage image3(pixelNumPerLine, frameSize, QImage::Format_Grayscale16);
    
    // 填充图像数据，并同步写入三个累积缓冲区
    for (quint16 y = 0; y < frameSize; ++y)
    {
        quint16* line1 = reinterpret_cast<quint16*>(image1.scanLine(y));
        quint16* line2 = reinterpret_cast<quint16*>(image2.scanLine(y));
        quint16* line3 = reinterpret_cast<quint16*>(image3.scanLine(y));

        memcpy(line1, dst1 + y * pixelNumPerLine, pixelNumPerLine * sizeof(quint16));
        memcpy(line2, dst2 + y * pixelNumPerLine, pixelNumPerLine * sizeof(quint16));
        memcpy(line3, dst3 + y * pixelNumPerLine, pixelNumPerLine * sizeof(quint16));
    }
    
    // 将图像加入历史缓冲区
    image1History.push_front(image1);
    image2History.push_front(image2);
    image3History.push_front(image3);
    
    // 限制历史缓冲区大小
    if (image1History.size() > displayNum) image1History.pop_back();
    if (image2History.size() > displayNum) image2History.pop_back();
    if (image3History.size() > displayNum) image3History.pop_back();
    
    // 将图像历史缓冲区显示到图像上
    DisplayImage();
}

void Widget::DisplayImage()
{ 
    quint16 width = cardNum * 64;
    quint16 totalHeight = displayNum * frameSize;

    // 创建用于展示的组合图像
    QImage combinedImage1(width, totalHeight, QImage::Format_Grayscale16);
    QImage combinedImage2(width, totalHeight, QImage::Format_Grayscale16);
    QImage combinedImage3(width, totalHeight, QImage::Format_Grayscale16);

    combinedImage1.fill(0);
    combinedImage2.fill(0);
    combinedImage3.fill(0);
    
    // 将历史图像拼接
    for (quint16 i = 0; i < image1History.size(); ++i)
    {
        const QImage& img1 = image1History[i];
        const QImage& img2 = image2History[i];
        const QImage& img3 = image3History[i];
        
        for (int y = frameSize - 1; y >= 0; --y)
        {
            const quint16* srcLine1 = reinterpret_cast<const quint16*>(img1.constScanLine(y));
            const quint16* srcLine2 = reinterpret_cast<const quint16*>(img2.constScanLine(y));
            const quint16* srcLine3 = reinterpret_cast<const quint16*>(img3.constScanLine(y));

            quint16* dstLine1 = reinterpret_cast<quint16*>(combinedImage1.scanLine(i * frameSize + frameSize - 1 - y));
            quint16* dstLine2 = reinterpret_cast<quint16*>(combinedImage2.scanLine(i * frameSize + frameSize - 1 - y));
            quint16* dstLine3 = reinterpret_cast<quint16*>(combinedImage3.scanLine(i * frameSize + frameSize - 1 - y));

            // 应用灰度缩放
            for (quint16 x = 0; x < width; ++x) {
                dstLine1[x] = (srcLine1[x] > greyScaleMax) ? 65535 : static_cast<quint16>((65535ULL * srcLine1[x]) / greyScaleMax);
                dstLine2[x] = (srcLine2[x] > greyScaleMax) ? 65535 : static_cast<quint16>((65535ULL * srcLine2[x]) / greyScaleMax);
                dstLine3[x] = (srcLine3[x] > greyScaleMax) ? 65535 : static_cast<quint16>((65535ULL * srcLine3[x]) / greyScaleMax);
            }
        }
    }

    QPixmap pixmap1 = QPixmap::fromImage(combinedImage1);
    QPixmap pixmap2 = QPixmap::fromImage(combinedImage2);
    QPixmap pixmap3 = QPixmap::fromImage(combinedImage3);

    // 创建变换矩阵
    QTransform transform;
    transform.rotate(270);

    // 应用变换矩阵
    pixmap1 = pixmap1.transformed(transform);
    pixmap2 = pixmap2.transformed(transform);
    pixmap3 = pixmap3.transformed(transform);

    // 显示图像
    ui->label_6->setPixmap(pixmap1);
    ui->label_5->setPixmap(pixmap2);
    ui->label_4->setPixmap(pixmap3);
}

void Widget::OnSaveImage()
{
    // 检查累积缓冲区
    if (accumulatedBuffer1.isEmpty())
    {
        QMessageBox::warning(this, "保存失败", "当前没有可保存的数据");
        return;
    }
    
    // 创建保存目录
    QDir dir("D:/scanResult");
    if (!dir.exists())
    {
        if (!dir.mkpath("."))
        {
            QMessageBox::critical(this, "保存失败", "无法创建保存目录");
            return;
        }
    }
    
    // 生成文件名时间戳
    QDateTime now = QDateTime::currentDateTime();
    QString timestamp = QString("%1年%2月%3日_%4：%5：%6")
        .arg(now.date().year())
        .arg(now.date().month())
        .arg(now.date().day())
        .arg(now.time().hour(), 2, 10, QChar('0'))
        .arg(now.time().minute(), 2, 10, QChar('0'))
        .arg(now.time().second(), 2, 10, QChar('0'));
    
    // 保存累积缓冲区为单个raw文件
    QString fileName1 = QString("D:/scanResult/%1_1.raw").arg(timestamp);
    QString fileName2 = QString("D:/scanResult/%1_2.raw").arg(timestamp);
    QString fileName3 = QString("D:/scanResult/%1_3.raw").arg(timestamp);

    QFile file1(fileName1);
    QFile file2(fileName2);
    QFile file3(fileName3);

    // 保存缓冲区1
    if (!file1.open(QIODevice::WriteOnly))
    {
        QMessageBox::critical(this, "保存失败", QString("无法创建文件: %1").arg(fileName1));
        return;
    }
    file1.write(reinterpret_cast<const char*>(accumulatedBuffer1.data()), accumulatedBuffer1.size() * sizeof(quint16));
    file1.close();

    // 保存缓冲区2
    if (!file2.open(QIODevice::WriteOnly))
    {
        QMessageBox::critical(this, "保存失败", QString("无法创建文件: %1").arg(fileName2));
        return;
    }
    file2.write(reinterpret_cast<const char*>(accumulatedBuffer2.data()), accumulatedBuffer2.size() * sizeof(quint16));
    file2.close();

    // 保存缓冲区3
    if (!file3.open(QIODevice::WriteOnly))
    {
        QMessageBox::critical(this, "保存失败", QString("无法创建文件: %1").arg(fileName3));
        return;
    }
    file3.write(reinterpret_cast<const char*>(accumulatedBuffer3.data()), accumulatedBuffer3.size() * sizeof(quint16));
    file3.close();
    
    QMessageBox::information(this, "保存成功", QString("累积扫描图像已保存到:\n   D:/scanResult\n大小: %1 MB").arg(accumulatedBuffer1.size() * sizeof(quint16) / 1024.0 / 1024.0, 0, 'f', 2));
}

void Widget::OnAutoStopTimeout()
{
    // 自动停止扫描
    if (!l103Controller->SetStopScanning())
    {
        QMessageBox::critical(this, "停止失败", "停止扫描发生错误, 错误代码为" + QString::number(static_cast<quint8>(l103Controller->GetLastError())));
    }

    // 恢复灰度滑块和保存按钮
    ui->verticalSlider->setEnabled(true);
    ui->pushButton_5->setEnabled(true);
    
    // 创建自定义消息框
    QMessageBox msgBox(this);
    msgBox.setWindowTitle("提示");
    msgBox.setText("扫描图像已达最大保存上限，继续扫描将丢失数据，是否继续？   ");
    msgBox.setStyleSheet("QMessageBox { min-width: 450px;}");
    
    QPushButton *continueBtn = msgBox.addButton("确定", QMessageBox::AcceptRole);
    QPushButton *okBtn = msgBox.addButton("取消", QMessageBox::RejectRole);
    
    continueBtn->setFixedWidth(75);
    continueBtn->setMinimumHeight(30);
    okBtn->setFixedWidth(75);
    okBtn->setMinimumHeight(30);
    
    msgBox.exec();
    
    if (msgBox.clickedButton() == continueBtn)
    {
        OnStartScan();
    }
    else
    {
        update_ui_for_eng_sptrm_scan(false);
    }
}

Widget::work_mode_e_t Widget::current_work_mode()
{
    if(ui->manTimerScanRBtn->isChecked()) return WORK_MODE_MAN_TIMER_SCAN;
    else return WORK_MODE_ENG_SPTRM_SCAN;
}

bool Widget::check_eng_sptrm_range(int s, int e, int step, QString * ret_str)
{
    bool ret = true;
    QString err_str;

    if((s > e && step >= 0) || (s < e && step <= 0))
    {
        ret = false;
        err_str = "起始能谱->结束能谱与步长方向不一致";
    }
    else if((s != e) && (0 == step))
    {
        ret = false;
        err_str = "起始能谱与结束能谱不等时，步长不能为0";
    }
    if(ret_str) *ret_str = err_str;
    return ret;
}

bool Widget::get_eng_sptrm_scan_info(QString *ret_str)
{
    bool ret = true;
    QString err_str;
    int vs, ve, vstep;

    vs = ui->startEngSpinBox->value();
    ve = ui->endEngSpinBox->value();
    vstep = ui->stepEngSpinBox->value();

    ret = check_eng_sptrm_range(vs, ve, vstep, &err_str);

    if(ret)
    {
        m_eng_sptrm_scan_info.start = vs;
        m_eng_sptrm_scan_info.end = ve;
        m_eng_sptrm_scan_info.step = vstep;

        m_eng_sptrm_scan_info.current = vs;
    }

    if(ret_str) *ret_str = err_str;

    return ret;
}

void Widget::scan_next_eng_sptrm_hdlr()
{
    if(all_eng_sptrm_scanned())
    {
        emit eng_sptrm_scan_finished_sig();
        return;
    }

    QString log_str;

    /* the set-th op costs long time, so we just set 1 since we use 1 only.
       if just set the 1st, there may be error since the latter th should be larger than the
       former(?). so we set the last th.
       and so NOTICE: in OnFrameReady, the data collected should be accordingly.
    */
    for(quint8 th = g_sptrum_threshold_num /* 1 */; th <= g_sptrum_threshold_num; ++th)
    {
        if (!l103Controller->SetThreshold(th, (quint16)(m_eng_sptrm_scan_info.current)))
        {
            emit eng_sptrm_scan_finished_sig();

            log_str = QString("设置阈值%1发生错误(%2), 错误代码为%3").arg(th)
                    .arg((quint16)(m_eng_sptrm_scan_info.current))
                    .arg(QString::number(static_cast<quint8>(l103Controller->GetLastError())));
            DIY_LOG(LOG_ERROR, log_str);
            QMessageBox::critical(this, "设置失败", log_str);

            return;
        }
    }

    if (!l103Controller->SetStartScanning())
    {
        emit eng_sptrm_scan_finished_sig();

        log_str = "启动扫描发生错误, 错误代码为" + QString::number(static_cast<quint8>(l103Controller->GetLastError()));
        DIY_LOG(LOG_ERROR, log_str);
        QMessageBox::critical(this, "启动失败", log_str);

        return;
    }
}

void Widget::inc_curr_eng_sptrm_scan_lvl()
{
    if(m_eng_sptrm_scan_info.start == m_eng_sptrm_scan_info.end)
    {
        m_eng_sptrm_scan_info.current += ((0 == m_eng_sptrm_scan_info.step) ?
                                              1 : m_eng_sptrm_scan_info.step);
    }
    else if(m_eng_sptrm_scan_info.current == m_eng_sptrm_scan_info.end)
    {
        m_eng_sptrm_scan_info.current += m_eng_sptrm_scan_info.step;
    }
    else
    {
        m_eng_sptrm_scan_info.current += m_eng_sptrm_scan_info.step;
        if(((m_eng_sptrm_scan_info.step > 0)
                && (m_eng_sptrm_scan_info.current > m_eng_sptrm_scan_info.end))
            ||((m_eng_sptrm_scan_info.step < 0)
                && (m_eng_sptrm_scan_info.current < m_eng_sptrm_scan_info.end)))
        {
            m_eng_sptrm_scan_info.current = m_eng_sptrm_scan_info.end;
        }
    }
}

bool Widget::all_eng_sptrm_scanned()
{
    bool finished = true;

    if(m_eng_sptrm_scan_info.start == m_eng_sptrm_scan_info.end)
    {
        finished = (m_eng_sptrm_scan_info.current != m_eng_sptrm_scan_info.start);
    }
    else
    {
        finished = (((m_eng_sptrm_scan_info.step > 0)
                        && (m_eng_sptrm_scan_info.current > m_eng_sptrm_scan_info.end))
                || ((m_eng_sptrm_scan_info.step < 0)
                        && (m_eng_sptrm_scan_info.current < m_eng_sptrm_scan_info.end)));
    }

    return finished;
}

void Widget::eng_sptrm_scan_finished_hdlr()
{
    update_ui_for_eng_sptrm_scan(false);

    QString info_err_str;

    if(m_eng_sptrm_raw_data.size() <= 0)
    {
        info_err_str = "No energy-sptrum data scanned.";
        DIY_LOG(LOG_WARN, info_err_str);
        QMessageBox::warning(this, "Warning", info_err_str);
        ui->scanStatusLbl->setText("");
        return;
    }

    QString pth_str(g_data_save_folder);
    if(!chk_mk_pth_and_warn(pth_str, this)) return;

    QString eng_range_str = QString("%1-%2-%3").arg(m_eng_sptrm_scan_info.start)
                                               .arg(m_eng_sptrm_scan_info.end)
                                               .arg(m_eng_sptrm_scan_info.step);
    QString data_file_name_str = QString("%1-%2-%3%4").arg(g_eng_sptrm_scan_data_bfn,
                              common_tool_get_curr_dt_str(), eng_range_str, g_eng_sptrm_scan_data_ext);
    QString fpn = pth_str + "/" + data_file_name_str;
    QFile data_file(fpn);
    if(!data_file.open(QIODevice::WriteOnly))
    {
        info_err_str = QString("创建文件 %1 失败：%2").arg(fpn, data_file.errorString());
        DIY_LOG(LOG_ERROR, info_err_str);
        QMessageBox::critical(this, "Error", info_err_str);
        return;
    }
    QTextStream t_stream(&data_file);
    QString line_str;
    //table head
    line_str = QString("%1,%2").arg(g_str_sptrum_range, gen_eng_sptrm_range_str());
    t_stream << line_str << "\n";

    //data
    if(m_eng_sptrm_raw_data.size() == 1)
    {
        for(int p = 0; p < m_eng_sptrm_raw_data[0].size(); ++p)
        {
            line_str = QString("%1-%2,%3").arg(g_str_pixel).arg(p).arg(m_eng_sptrm_raw_data[0][p]);
            t_stream << line_str << "\n";
        }
    }
    else
    {
        int range_cnt = m_eng_sptrm_raw_data.size() - 1;
        for(int p = 0; p < m_eng_sptrm_raw_data[0].size(); ++p)
        {
            line_str = QString("%1-%2").arg(g_str_pixel).arg(p);
            for(int r = 0; r < range_cnt; ++r)
            {
                acc_px_data_type diff = (m_eng_sptrm_scan_info.step > 0) ?
                                        (m_eng_sptrm_raw_data[r][p] - m_eng_sptrm_raw_data[r+1][p])
                                      : (m_eng_sptrm_raw_data[r+1][p] - m_eng_sptrm_raw_data[r][p]);
                line_str += QString(",%1").arg(diff);
            }
            t_stream << line_str << "\n";
        }
    }

    data_file.close();

    QMessageBox::information(this, "OK", QString("能谱扫描已完成:\n%1").arg(fpn));
    ui->scanStatusLbl->setText("");
}

QString Widget::gen_eng_sptrm_range_str(QString s_e_sep, QString r_sep)
{
    if(!check_eng_sptrm_range(m_eng_sptrm_scan_info.start, m_eng_sptrm_scan_info.end,
                              m_eng_sptrm_scan_info.step))
    {
        return "";
    }

    if(m_eng_sptrm_scan_info.start == m_eng_sptrm_scan_info.end)
    {
        return QString("%1").arg(m_eng_sptrm_scan_info.start);
    }

    bool inc = (m_eng_sptrm_scan_info.step > 0);
    QString line_str;
    int s = m_eng_sptrm_scan_info.start, e;
    while((inc && (s < m_eng_sptrm_scan_info.end)) || (!inc && (s > m_eng_sptrm_scan_info.end)))
    {
        e = s + m_eng_sptrm_scan_info.step;
        if((inc && (e > m_eng_sptrm_scan_info.end)) || (!inc && (e < m_eng_sptrm_scan_info.end)))
        {
            e = m_eng_sptrm_scan_info.end;
        }

        line_str += QString("%1%2%3%4").arg(s).arg(s_e_sep).arg(e).arg(r_sep);

        s += m_eng_sptrm_scan_info.step;
    }
    line_str.chop(r_sep.length()); //remove the lase extra r_sep.
    return line_str;
}

int Widget::get_eng_sptrm_range_cnt()
{
    return 0;
}

void Widget::update_ui_according_to_work_mode()
{
    work_mode_e_t curr_mode = current_work_mode();

    ui->groupBox->setVisible(curr_mode != WORK_MODE_ENG_SPTRM_SCAN);

    ui->engSptrmScanSetGpBox->setVisible(curr_mode == WORK_MODE_ENG_SPTRM_SCAN);

    if(WORK_MODE_MAN_TIMER_SCAN == curr_mode)
    {
        for(quint8 idx = 1; idx <= g_sptrum_threshold_num; ++ idx)
        {
            update_th_list_display(idx);
        }
    }
    ui->tableWidget->setVisible(curr_mode != WORK_MODE_ENG_SPTRM_SCAN);
    ui->scanStatusLbl->setVisible(curr_mode == WORK_MODE_ENG_SPTRM_SCAN);
}

void Widget::update_ui_for_eng_sptrm_scan(bool scan_start)
{
    ui->pushButton->setEnabled(!scan_start);
    ui->pushButton_4->setEnabled(!scan_start);
    ui->lineEdit->setEnabled(!scan_start);

    ui->pushButton_2->setEnabled(scan_start); //stop scan;

    ui->manTimerScanRBtn->setEnabled(!scan_start);
    ui->engSptrmScanRBtn->setEnabled(!scan_start);
}

void Widget::on_engSptrmScanRBtn_toggled(bool /*checked*/)
{
     update_ui_according_to_work_mode();
}

bool Widget::prepare_eng_sptrm_scan()
{
    bool ret = true;
    QString log_str;
    quint16 ini_th;

    ini_th = (m_eng_sptrm_scan_info.step >= 0) ?
                (quint16)(m_eng_sptrm_scan_info.start) :
                (quint16)(m_eng_sptrm_scan_info.end);

    for(quint8 th = 1; th <= g_sptrum_threshold_num; ++th)
    {
        if (!l103Controller->SetThreshold(th, ini_th))
        {
            ret = false;

            log_str = QString("设置阈值%1发生错误(%2), 错误代码为%3").arg(th)
                    .arg((quint16)(m_eng_sptrm_scan_info.current))
                    .arg(QString::number(static_cast<quint8>(l103Controller->GetLastError())));
            DIY_LOG(LOG_ERROR, log_str);
            QMessageBox::critical(this, "设置失败", log_str);

            break;
        }
    }
    return ret;
}
