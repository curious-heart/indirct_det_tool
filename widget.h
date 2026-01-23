#ifndef WIDGET_H
#define WIDGET_H

// 必须先引用, 避免其他库引入的宏或枚举冲突
#include "L103Controller.h"

#include <deque>

#include <QWidget>
#include <QLabel>
#include <QImage>
#include <QTimer>

QT_BEGIN_NAMESPACE
namespace Ui {
class Widget;
}
QT_END_NAMESPACE

class Widget : public QWidget
{
    Q_OBJECT

public:
    explicit Widget(QWidget *parent = nullptr);
    ~Widget();

private slots:
    // 按钮槽函数
    void OnStartScan();
    void OnStopScan();
    void OnSetFrameLineNum();
    void OnSetThreshold1();
    void OnSetThreshold2();
    void OnSetThreshold3();
    void OnSaveImage();

    // 滑块槽函数
    void OnGreyScaleChanged(int value);
    
    // 帧就绪槽函数
    void OnFrameReady(quint16 *data);
    
    // 定时器槽函数
    void OnAutoStopTimeout();

private:
    // 初始化
    void InitMember();
    void InitUI();
    void InitConnect();

    // 显示图像
    void DisplayImage();

private:
    Ui::Widget *ui;

    L103Controller *l103Controller;      // 探测器库对象
    quint16 frameSize;                   // 帧大小
    quint16 cardNum;                     // 板卡数
    quint16 displayNum;                  // 单次展示帧数 (最大历史缓冲数)
    quint16 greyScaleMax;                // 灰度拉伸上限

    std::deque<QImage> image1History;    // 阈值1图像历史缓冲区
    std::deque<QImage> image2History;    // 阈值2图像历史缓冲区
    std::deque<QImage> image3History;    // 阈值3图像历史缓冲区
    
    QVector<quint16> accumulatedBuffer1; // 累积保存缓冲区1
    QVector<quint16> accumulatedBuffer2; // 累积保存缓冲区2
    QVector<quint16> accumulatedBuffer3; // 累积保存缓冲区3
    
    QTimer *autoStopTimer;               // 自动停止定时器
};

#endif
