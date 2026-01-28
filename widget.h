#ifndef WIDGET_H
#define WIDGET_H

// 必须先引用, 避免其他库引入的宏或枚举冲突
#include "L103Controller.h"

#include <deque>

#include <QWidget>
#include <QLabel>
#include <QImage>
#include <QTimer>
#include <QVector>

#include "config_recorder/uiconfigrecorder.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class Widget;
}
QT_END_NAMESPACE

typedef qint64 acc_px_data_type;

class Widget : public QWidget
{
    Q_OBJECT

public:
    explicit Widget(QWidget *parent = nullptr);
    ~Widget();

    typedef enum
    {
        WORK_MODE_MAN_TIMER_SCAN,
        WORK_MODE_ENG_SPTRM_SCAN,
    }work_mode_e_t;

    typedef struct
    {
        int start, end, step;
        int current;
    }eng_sptrm_scan_s_t;

signals:
    void scan_next_eng_sptrm_sig();
    void eng_sptrm_scan_finished_sig();

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

    void scan_next_eng_sptrm_hdlr();
    void eng_sptrm_scan_finished_hdlr();

    void on_engSptrmScanRBtn_toggled(bool checked);
    bool prepare_eng_sptrm_scan();
    void update_th_list_display(quint8 th_idx);

private:
    // 初始化
    void InitMember();
    void InitUI();
    void InitConnect();

    // 显示图像
    void DisplayImage();

    work_mode_e_t current_work_mode();
    bool check_eng_sptrm_range(int s, int e, int step, QString * ret_str = nullptr);
    bool get_eng_sptrm_scan_info(QString *ret_str = nullptr);
    void inc_curr_eng_sptrm_scan_lvl();
    bool all_eng_sptrm_scanned();
    QString gen_eng_sptrm_range_str(QString s_e_sep = "-", QString r_sep = ",");
    int get_eng_sptrm_range_cnt();

    void update_ui_according_to_work_mode();
    void update_ui_for_eng_sptrm_scan(bool scan_start);

private:
    Ui::Widget *ui;

    UiConfigRecorder m_ui_cfg_rec;
    qobj_ptr_set_t m_ui_cfg_rec_filter_in, m_ui_cfg_rec_filter_out;
    bool m_init_ok = false, m_preparing = false;

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

    eng_sptrm_scan_s_t m_eng_sptrm_scan_info;
    QVector<QVector<acc_px_data_type>> m_eng_sptrm_raw_data;
};

#endif
