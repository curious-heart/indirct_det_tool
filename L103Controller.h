#ifndef L103CONTROLLER_H
#define L103CONTROLLER_H

#include <QObject>
#include <QUdpSocket>
#include <QHostAddress>
#include <QScopedPointer>
#include <QVector>
#include <QMap>
#include <QPair>

#if defined(L103CONTROLLER_LIBRARY)
#  define L103CONTROLLER_EXPORT Q_DECL_EXPORT
#else
#  define L103CONTROLLER_EXPORT Q_DECL_IMPORT
#endif

class L103Command;

class L103CONTROLLER_EXPORT L103Controller : public QObject
{
    Q_OBJECT

public:
    enum ErrorCode : quint8
    {
        NO_ERROR = 0,      // 无错误
        PARAMETER_ERROR,   // 参数错误, 原因: 使用者输入参数范围错误
        NETWORK_ERROR,     // 网络错误, 原因: 数据包数据错误, 网络权限问题, 端口被占用绑定失败, 网络波动或物理设备拥塞导致的丢包、超时问题
        MEMORY_ERROR       // 内存错误, 原因: 内存不足, 空间分配失败
    };
    
public:
    // 构造函数, 仅初始化部分简单成员, 线程安全且不会报错, 完整初始化位于 Init 函数
    explicit L103Controller(quint16 frameLineNum, quint16 waitTime = 20000, QObject *parent = nullptr);
    ~L103Controller();

    // 初始化, 创建线程敏感的套接字成员, 分配缓冲区, 与探测器通信确定状态参数成员, 绑定探测器图像端口, 开始监听
    bool Init();

    // 开始采集数据
    bool SetStartScanning();
    // 停止采集数据
    bool SetStopScanning();

    // 获取图像每列点数
    bool GetPixelNum(quint16 &pixelNumPerLine);
    // 获取卡的数量
    bool GetCardNum(quint16 &cardNum);
    // 获取每张卡的温度
    bool GetTempList(QList<quint16> &temperatureList);
    // 获取每张卡所有通道的阈值门限值列表 (阈值类型为 1、2、3)
    bool GetThresholdList(quint8 thresholdType, QList<std::array<quint8,64>> &thresholdList);

    // 设置通信超时时间 (单位: ms)
    bool SetWaitTime(quint16 waitTime);
    // 设置帧大小
    bool SetFrameSize(quint16 frameLineNum);
    // 设置所有卡所有通道的阈值门限值
    bool SetThreshold(quint8 thresholdType, quint16 thresholdValue);

    // 获取上一次错误
    ErrorCode GetLastError() const { return lastError; }

signals:
    // 一帧数据就绪信号 (每列长度为 pixelNum, 依次为阈值1的所有设备通道值、阈值2的所有设备通道值、阈值3的所有设备通道值, 共 frameLineNum 列)
    void SigFrameReady(quint16 *data);

private slots:
    // 连接到探测器图像端口接收图像数据包
    void OnReadyRead();

public:
    // 一个有效通道的数据格式结构体
    struct ChannelData
    {
        quint8 deviceIndex = 0x00;                // 设备号 (高四位代表链条编号，低四位代表从机编号)
        quint8 channelIndex = 0x00;               // 通道号
        quint16 photonCount1 = 0x0000;            // 第一个阈值的计数 (0x0100 表示 256)
        quint16 photonCount2 = 0x0000;            // 第二个阈值的计数
        quint16 photonCount3 = 0x0000;            // 第三个阈值的计数

        quint8 GetChain() const { return (deviceIndex >> 4) & 0x0F; }
        quint8 GetSlave() const { return deviceIndex & 0x0F; }
    };

    // 一个有效设备的数据格式结构体
    struct DeviceData
    {
        std::array<ChannelData,64> channelDatas;  // 一个有效设备包含64个通道
    };

    // 一个完整的图像数据包数据格式结构体
    struct ImgPacket
    {
        static constexpr quint16 EndCode = 0x050D;

        quint32 macLow4Bytes = 0x00000000;        // MAC 地址低4个字节
        std::array<DeviceData,10> deviceDatas;    // 一个图像数据包至多包含10个有效设备的数据, 不足用0填充
        quint32 timeStamp = 0x00000000;           // 时间戳

        // 从大端字节流，为数据包赋值
        bool FromByteArray(const QByteArray& raw); 
    };

    // 向探测器命令通道 IP 端口发送控制指令数据包 cmd 并接收应答数据包 ack
    bool SendCmdAndRecvACK(const QHostAddress &dstIp, quint16 dstPort, const QByteArray &cmd, QByteArray &ack);
    // 处理接收到的图像数据包, 写入缓冲区并负责适时发送
    void ProcessImgPacket(const QByteArray& dataGram);


private:
    // ************ 套接字 ************
    QScopedPointer<QUdpSocket> cmdSocket;                            // 命令端口套接字
    QScopedPointer<QUdpSocket> imgSocket;                            // 图像端口套接字

    // ************ 网络通信参数 ************
    QHostAddress ip;                                                 // 探测器 IP 地址
    QByteArray mac;                                                  // 探测器 MAC 地址
    quint16 cmdPort;                                                 // 探测器命令端口
    quint16 imgPort;                                                 // 探测器图像端口

    // ************ 探测器参数 ************
    bool scanState;                                                  // 扫描状态
    quint16 frameLineNum;                                            // 每帧列数
    std::array<quint8,4> slaveNumList;                               // 每个链条的从机数量
    quint16 validDeviceNum;                                          // 有效设备数量 (即板卡数)
    QMap<QPair<quint8,quint8>,quint16> auxMap;                       // 辅助映射表 (<链路编号, 从机编号>对应连续有效设备编号)
    QList<std::array<std::array<quint16,64>,3>> ec1Lists;            // 每个有效设备的 EC1 列表 (索引为连续有效设备编号)
    QList<std::array<std::array<quint16,64>,3>> ec2Lists;            // 每个有效设备的 EC2 列表 (索引为连续有效设备编号)
    QList<std::array<std::array<quint8,64>,3>> thresholdValueLists;  // 每个有效设备的阈值列表 (索引为连续有效设备编号)

    // ************ 缓冲区参数 ************
    quint16 *masterFrameBuffer;                                      // 帧的主缓冲区 (负责存储接收到但未满一帧的图像数据)
    quint16 *slaveFrameBuffer;                                       // 帧的从缓冲区 (负责提供给用户进行一帧图像数据读取)
    QVector<quint16> bitmap;                                         // 帧缓冲区填充位图 (负责记录缓冲区每列的填充情况, bitmap[i]表示第i个连续有效设备收到的数据包数量)

    // ************ 其他参数 ************
    L103Command *command;                                            // 指令生成和回复解析单元
    quint16 waitTime;                                                // 通信超时时间 (单位: ms)
    ErrorCode lastError;                                             // 上一次错误
};

#endif
