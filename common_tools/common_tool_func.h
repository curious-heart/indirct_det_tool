#ifndef COMMON_TOOL_FUNC_H
#define COMMON_TOOL_FUNC_H

#include <QString>
#include <QNetworkInterface>
#include <QTextEdit>
#include <QFont>
#include <QSet>
#include <QKeyEvent>

#include <type_traits>  // std::make_unsigned
#include <QImage>

typedef enum
{
    IP_SET_TYPE_IPV4_DYNAMIC = 0,
    IP_SET_TYPE_IPV4_FIXED,
}ip_set_type_t;

typedef enum
{
    /*every bit corresponds to one kind of intf.*/
    IP_INTF_OTHER = 0,
    IP_INTF_WIFI = 0x01,
    IP_INTF_ETHERNET = 0x02,
}ip_intf_type_t;

typedef enum
{
    APP_EXIT_NORMAL = 0,
    APP_EXIT_APP_POWER_OFF,
    APP_EXIT_HD_POWER_OFF,
}app_exit_mode_e_t;

QString common_tool_get_curr_dt_str(QString sep_d = "", QString sep_d_t = "", QString sep_t = "");
QString common_tool_get_curr_date_str();
QString common_tool_get_curr_time_str();

bool mkpth_if_not_exists(const QString &pth_str);
bool chk_mk_pth_and_warn(QString &pth_str, QWidget * parent = nullptr, bool warn_caller = true);

#define DEF_SHUTDOWN_WAIT_TIME 3
/*return the shutdown command line.*/
QString shutdown_system(QString reason_str = "", int wait_time = DEF_SHUTDOWN_WAIT_TIME);

#define ROUNDUP_UINT16_TO_10(x) ((quint16)(((quint16)(((x) + 5) / 10)) * 10))
#define ARRAY_COUNT(a) (sizeof((a)) / sizeof((a)[0]))

typedef enum
{
    INT_DATA, FLOAT_DATA,
}common_data_type_enum_t;
#define EDGE_ITEM(a) a
#define EDGE_LIST \
        EDGE_ITEM(EDGE_INCLUDED),\
        EDGE_ITEM(EDGE_EXCLUDED),\
        EDGE_ITEM(EDGE_INFINITE),\
        EDGE_ITEM(EDGE_COUNT),
typedef enum
{
    EDGE_LIST
}range_edge_enum_t;
#undef EDGE_ITEM
template <typename T> class RangeChecker
{
public:
static  const char* range_edge_strs[];

private:
    bool valid;
    T min, max;
    range_edge_enum_t low_edge, up_edge;
    QString unit_str;
public:
    RangeChecker(T min, T max, QString unit_str = "",
                 range_edge_enum_t low_edge = EDGE_INCLUDED, range_edge_enum_t up_edge = EDGE_INCLUDED);
public:
    bool range_check(T val);

    void set_min_max(T min_v, T max_v);
    void set_edge(range_edge_enum_t low_e, range_edge_enum_t up_e);
    void set_unit_str(QString unit_s);
    range_edge_enum_t range_low_edge();
    range_edge_enum_t range_up_edge();
    T range_min();
    T range_max();
    QString range_str(double factor = 1, QString new_unit_str = "");
};

int count_discrete_steps(double low_edge, double up_edge, double step);
int count_discrete_steps(float low_edge, float up_edge, float step);
int count_discrete_steps(int low_edge, int up_edge, int step);

extern const char* g_prop_name_def_color;
extern const char* g_prop_name_def_font;
void append_str_with_color_and_weight(QTextEdit* ctrl, QString str,
                             QColor new_color = QColor(), int new_font_weight = -1);
typedef struct
{
    QString str;
    QColor color;
    int weight;
}str_with_style_s_t;
typedef QList<str_with_style_s_t> str_line_with_styles_t;
void append_line_with_styles(QTextEdit* ctrl, str_line_with_styles_t &style_line);

class CToolKeyFilter : public QObject
{
    Q_OBJECT

private:
    QObject * m_cared_obj = nullptr;
    QSet<Qt::Key> m_keys_to_filter;

protected:
    bool eventFilter(QObject * obj, QEvent * evt) override;

public:
    CToolKeyFilter(QObject* obj = nullptr, QObject * parent = nullptr);
    ~CToolKeyFilter();
    void add_keys_to_filter(Qt::Key key);
    void add_keys_to_filter(const QSet<Qt::Key> & keys);
};

// 求最大公约数
template <typename T>
T gcd(T a, T b)
{
    // 保证 a 和 b 为非负数（支持负数输入）
    a = qAbs(a);
    b = qAbs(b);
    while (b != 0) {
        T t = b;
        b = a % b;
        a = t;
    }
    return a;
}

// 求最小公倍数
template <typename T>
T lcm(T a, T b)
{
    if (a == 0 || b == 0)
        return 0;

    // 为防止乘法溢出，用无符号类型存储中间结果
    using U = typename std::make_unsigned<T>::type;
    U ua = qAbs(a);
    U ub = qAbs(b);

    return static_cast<T>((ua / gcd(ua, ub)) * ub);
}

bool ip_addr_valid(QString ip_str);
extern RangeChecker<int> g_ip_port_ranger;

extern const qint64 g_Byte_unit;
extern const qint64 g_KB_unit;
extern const qint64 g_MB_unit;
extern const qint64 g_GB_unit;
extern const qint64 g_TB_unit;

typedef struct
{
    qint64 total, total_used, total_ava;
}storage_space_info_s_t;
void get_total_storage_amount(storage_space_info_s_t &storage_info);
QString trans_bytes_cnt_unit(qint64 cnt, qint64 *unit = nullptr);

#endif // COMMON_TOOL_FUNC_H
