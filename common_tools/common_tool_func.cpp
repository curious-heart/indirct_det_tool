#include <QDateTime>
#include <QHostAddress>
#include <QNetworkAddressEntry>
#include <QList>
#include <QProcess>
#include <QDir>
#include <QColor>
#include <QFont>
#include <QtMath>
#include <QStorageInfo>
#include <QMessageBox>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

#include "literal_strings/literal_strings.h"
#include "common_tool_func.h"
#include "logger/logger.h"

QString common_tool_get_curr_dt_str(QString sep_d, QString sep_d_t, QString sep_t)
{
    QDateTime curDateTime = QDateTime::currentDateTime();
    QString fmt_str = QString("yyyy%1MM%2dd%3hh%4mm%5ss")
                        .arg(sep_d, sep_d, sep_d_t, sep_t, sep_t);

    QString dtstr = curDateTime.toString(fmt_str);
    return dtstr;
}

QString common_tool_get_curr_date_str()
{
    QDateTime curDateTime = QDateTime::currentDateTime();
    QString datestr = curDateTime.toString("yyyyMMdd");
    return datestr;
}

QString common_tool_get_curr_time_str()
{
    QTime curTime = QTime::currentTime();
    QString time_str = curTime.toString("hh:mm:ss");
    return time_str;
}

bool mkpth_if_not_exists(const QString &pth_str)
{
    QDir data_dir(pth_str);
    bool ret = true;
    if(!data_dir.exists())
    {
        data_dir = QDir();
        ret = data_dir.mkpath(pth_str);
    }
    return ret;
}

bool chk_mk_pth_and_warn(QString &pth_str, QWidget * parent, bool warn_caller)
{
    if(!mkpth_if_not_exists(pth_str))
    {
        QString err_str = QString("%1%2:%3").arg(g_str_create_folder, g_str_fail, pth_str);
        DIY_LOG(LOG_ERROR, err_str);
        if(warn_caller) QMessageBox::critical(parent, "Error", err_str);
        return false;
    }
    return true;
}

QString shutdown_system(QString reason_str,int wait_time)
{
#ifdef Q_OS_UNIX
    if(wait_time > 0) QThread::sleep(wait_time);

    QProcess::execute("sync", {});
    QProcess::execute("systemctl", {"poweroff"});

    return "";
#else
    QString s_c = "shutdown";
    QStringList ps_a;
    QProcess ps;
    ps.setProgram(s_c);
    ps_a << "/s" << "/t" << QString("%1").arg(wait_time);
    ps_a << "/d" << "u:4:1" << "/c" << reason_str;
    ps.setArguments(ps_a);
    ps.startDetached();
    return s_c + " " + ps_a.join(QChar(' '));
#endif
}

/*begin of RangeChecker------------------------------*/
#undef EDGE_ITEM
#define EDGE_ITEM(a) #a
template <typename T> const char* RangeChecker<T>::range_edge_strs[] =
{
    EDGE_LIST
};
#undef EDGE_ITEM
static const char* gs_range_checker_err_msg_invalid_edge_p1 = "low/up edge should be";
static const char* gs_range_checker_err_msg_invalid_edge_p2 = "and can't both be";
static const char* gs_range_checker_err_msg_invalid_eval =
        "Invalid range: min must be less than or equal to max!";

template <typename T>
RangeChecker<T>::RangeChecker(T min, T max, QString unit_str,
                           range_edge_enum_t low_edge, range_edge_enum_t up_edge)
{
    if((low_edge > EDGE_INFINITE) || (up_edge > EDGE_INFINITE)
        || (low_edge < EDGE_INCLUDED) || (up_edge < EDGE_INCLUDED)
        || (EDGE_INFINITE == low_edge && EDGE_INFINITE == up_edge))
    {
        DIY_LOG(LOG_ERROR, QString("%1 %2, %3, or %4, %5").arg(
                    gs_range_checker_err_msg_invalid_edge_p1,
                    range_edge_strs[EDGE_INCLUDED],
                    range_edge_strs[EDGE_EXCLUDED],
                    range_edge_strs[EDGE_INFINITE],
                    gs_range_checker_err_msg_invalid_edge_p2
                    ));
        return;
    }
    valid = (EDGE_INFINITE == low_edge || EDGE_INFINITE == up_edge) ? true : (min <= max);
    this->min = min; this->max = max;
    this->low_edge = low_edge; this->up_edge = up_edge;
    this->unit_str = unit_str;

    if(!valid)
    {
        DIY_LOG(LOG_ERROR, QString(gs_range_checker_err_msg_invalid_eval));
    }
}

template <typename T> bool RangeChecker<T>::range_check(T val)
{
    bool ret = true;
    if(!valid)
    {
        DIY_LOG(LOG_ERROR, "Invalid Range Checker!")
        return false;
    }

    if(EDGE_INCLUDED == low_edge) ret = (ret && (val >= min));
    else if(EDGE_EXCLUDED == low_edge) ret = (ret && (val > min));

    if(EDGE_INCLUDED == up_edge) ret = (ret && (val <= max));
    else if(EDGE_EXCLUDED == up_edge) ret = (ret && (val < max));
    return ret;
}

template <typename T> range_edge_enum_t RangeChecker<T>::range_low_edge()
{
    return low_edge;
}

template <typename T> range_edge_enum_t RangeChecker<T>::range_up_edge()
{
    return up_edge;
}

template <typename T> T RangeChecker<T>::range_min()
{
    return min;
}

template <typename T> T RangeChecker<T>::range_max()
{
    return max;
}

template <typename T> QString RangeChecker<T>::
range_str(double factor, QString new_unit_str )
{
    QString ret_str;

    if(!valid) return ret_str;

    ret_str = (EDGE_INCLUDED == low_edge ? "[" : "(");
    ret_str += (EDGE_INFINITE == low_edge) ? "" : QString::number((T)(min * factor));
    ret_str += ", ";
    ret_str += (EDGE_INFINITE == up_edge) ? "" : QString::number((T)(max * factor));
    ret_str += (EDGE_INCLUDED == up_edge) ? "]" : ")";
    //ret_str += ((1 == factor) || new_unit_str.isEmpty()) ? QString(unit_str) : new_unit_str;
    ret_str += (new_unit_str.isEmpty()) ? QString(unit_str) : new_unit_str;
    return ret_str;
}

template <typename T> void RangeChecker<T>::set_min_max(T min_v, T max_v)
{
    if(EDGE_INFINITE == low_edge || EDGE_INFINITE == up_edge || min_v <= max_v)
    {
        min = min_v;
        max = max_v;
    }
}

template <typename T> void RangeChecker<T>::set_edge(range_edge_enum_t low_e, range_edge_enum_t up_e)
{
    low_edge = low_e; up_edge = up_e;
}

template <typename T> void RangeChecker<T>::set_unit_str(QString unit_s)
{
    unit_str = unit_s;
}

template class RangeChecker<int>;
template class RangeChecker<float>;
template class RangeChecker<double>;
/*end of RangeChecker------------------------------*/

const char* g_prop_name_def_color = "def_color";
const char* g_prop_name_def_font = "def_font";
/*the following two macros MUST be used as a pair.*/
#define SAVE_DEF_COLOR_FONT(ctrl) \
{\
    QColor def_color;\
    QFont def_font;\
    QVariant var_prop;\
    var_prop = (ctrl)->property(g_prop_name_def_color);\
    if(!(var_prop.isValid() && (def_color = var_prop.value<QColor>()).isValid()))\
    {\
        def_color = (ctrl)->textColor();\
    }\
    var_prop = (ctrl)->property(g_prop_name_def_font);\
    def_font = var_prop.isValid() ? var_prop.value<QFont>() : (ctrl)->currentFont();

#define RESTORE_DEF_COLOR_FONT(ctrl) \
    (ctrl)->setTextColor(def_color);\
    (ctrl)->setCurrentFont(def_font);\
}

void append_str_with_color_and_weight(QTextEdit* ctrl, QString str,
                                      QColor new_color, int new_font_weight)
{
    if(!ctrl) return;

    ctrl->moveCursor(QTextCursor::End);

    SAVE_DEF_COLOR_FONT(ctrl);

    QFont new_font = def_font;
    if(!new_color.isValid()) new_color = def_color;
    if(new_font_weight > 0) new_font.setWeight((QFont::Weight)new_font_weight);

    ctrl->setTextColor(new_color);
    ctrl->setCurrentFont(new_font);
    ctrl->append(str);

    ctrl->moveCursor(QTextCursor::End);

    RESTORE_DEF_COLOR_FONT(ctrl);
}

void append_line_with_styles(QTextEdit* ctrl, str_line_with_styles_t &style_line)
{
    if(!ctrl) return;

    ctrl->moveCursor(QTextCursor::End);

    SAVE_DEF_COLOR_FONT(ctrl);

    QFont new_font = def_font;
    ctrl->insertPlainText("\n");
    for(int idx = 0; idx < style_line.count(); ++idx)
    {
        ctrl->setTextColor(style_line[idx].color);
        new_font.setWeight((QFont::Weight)(style_line[idx].weight));
        ctrl->setCurrentFont(new_font);
        ctrl->insertPlainText(style_line[idx].str);
        ctrl->moveCursor(QTextCursor::End);
    }

    RESTORE_DEF_COLOR_FONT(ctrl);
}

template<typename T> int count_discrete_steps_T(T low_edge, T up_edge, T step)
{
    if(low_edge == up_edge) return 1;
    if(0 == step) return 0;
    if((low_edge < up_edge && step < 0) || (low_edge > up_edge && step > 0)) return 0;

    int cnt = 1;
    T curr = low_edge;
    while(true)
    {
        ++cnt;
        curr += step;
        if((step > 0 && curr >= up_edge) || (step < 0 && curr <= up_edge)) break;
    }
    return cnt;

    /*
     * we use template function instead of one function of double type parameter for the following reason:
     * if use a single function of double type parameter, and a caller passes in float value, the result
     * may be incorrect due to accurancy differency. e.g.:
     *
     * caller passes in the following parameters of float value:
     * low_edge = 0.5, up_edge = 0.6, step = 0.1. we expect the result is 2.
     * but, since 0.6 and 0.1 can't be accurate enough in float, they are something like 0.6000000238
     * and 0.10000000149011612. when calculated in double, the small differences leads to the
     * final result of 3.
     *
     * due to similiar reason, the following method may also give out incorrect result.
    T tmp = (up_edge - low_edge) / step;
    if(tmp < 0) return 0;
    return qCeil(tmp) + 1;
    */
}

int count_discrete_steps(double low_edge, double up_edge, double step)
{
    return count_discrete_steps_T<double>(low_edge, up_edge, step);
}

int count_discrete_steps(float low_edge, float up_edge, float step)
{
    return count_discrete_steps_T<float>(low_edge, up_edge, step);
}

int count_discrete_steps(int low_edge, int up_edge, int step)
{
    return count_discrete_steps_T<int>(low_edge, up_edge, step);
}

CToolKeyFilter::CToolKeyFilter(QObject* obj, QObject * parent)
    :QObject{parent}, m_cared_obj(obj)
{}

CToolKeyFilter::~CToolKeyFilter()
{
    m_keys_to_filter.clear();
}

void CToolKeyFilter::add_keys_to_filter(Qt::Key key)
{
    m_keys_to_filter.insert(key);
}

void CToolKeyFilter::add_keys_to_filter(const QSet<Qt::Key> & keys)
{
    m_keys_to_filter.unite(keys);
}

bool CToolKeyFilter::eventFilter(QObject * obj, QEvent * evt)
{
    if(!obj || !evt || obj != m_cared_obj) return false;

    if(evt->type() == QEvent::KeyPress)
    {
        QKeyEvent * key_evt = static_cast<QKeyEvent *>(evt);
        if(m_keys_to_filter.contains((Qt::Key)key_evt->key()))
        {
            return true;
        }
    }
    return QObject::eventFilter(obj, evt);
}


bool ip_addr_valid(QString ip_str)
{
    QHostAddress addr_checker;
    return addr_checker.setAddress(ip_str);
}
RangeChecker<int> g_ip_port_ranger(0, 65535, "", EDGE_INCLUDED, EDGE_INCLUDED);

const qint64 g_Byte_unit = 1;
const qint64 g_KB_unit = 1024;
const qint64 g_MB_unit = g_KB_unit * 1024;
const qint64 g_GB_unit = g_MB_unit * 1024;
const qint64 g_TB_unit = g_GB_unit * 1024;
void get_total_storage_amount(storage_space_info_s_t &storage_info)
{
    storage_info.total = storage_info.total_used = storage_info.total_ava = 0;
    QList<QStorageInfo> volumes = QStorageInfo::mountedVolumes();
    for (const QStorageInfo &storage : volumes)
    {
        storage_info.total += storage.bytesTotal();
        storage_info.total_ava += storage.bytesAvailable();
    }
    storage_info.total_used = storage_info.total - storage_info.total_ava;
}

QString trans_bytes_cnt_unit(qint64 cnt, qint64 *unit)
{
    QString unit_str;
    qint64 unit_val;

    if(cnt < g_KB_unit)
    {
        unit_str = g_str_Byte;
        unit_val = g_Byte_unit;
    }
    else if(cnt <= g_MB_unit)
    {
        unit_str = g_str_KB;
        unit_val = g_KB_unit;
    }
    else if(cnt <= g_GB_unit)
    {
        unit_str = g_str_MB;
        unit_val = g_MB_unit;
    }
    else if(cnt <= g_TB_unit)
    {
        unit_str = g_str_GB;
        unit_val = g_GB_unit;
    }
    else
    {
        unit_str = g_str_TB;
        unit_val = g_TB_unit;
    }
    if(unit) *unit = unit_val;
    return unit_str;
}

// 从 raw 文件读取 (8/16bit 灰度，逐行紧密存储)
QImage read_gray_raw_img(const QString &fileName, int width, int height, QImage::Format img_fmt)
{
    if(img_fmt != QImage::Format_Grayscale8 && img_fmt != QImage::Format_Grayscale16)
    {
        DIY_LOG(LOG_WARN, "Only QImage::Format_Grayscale8 and QImage::Format_Grayscale16 "
                          "image can be processed.");
        return QImage();
    }

    QFile f(fileName);
    if (!f.open(QIODevice::ReadOnly))
    {
        DIY_LOG(LOG_ERROR, QString("Open file %1 error: %2").arg(fileName, f.errorString()));
        return QImage();
    }

    QImage img(width, height, img_fmt);
    if (img.isNull())
    {
        DIY_LOG(LOG_ERROR, "Failed to allocate image");
        f.close();
        return QImage();
    }
    img.fill(0);

    const int bpp       = img.depth() / 8;          // = 1 or 2
    const int lineBytes = width * bpp;

    bool ok = true;
    for (int y = 0; y < height; ++y)
    {
        char *dst = reinterpret_cast<char*>(img.scanLine(y));
        qint64 read = f.read(dst, lineBytes);
        if (read != lineBytes)
        {
            DIY_LOG(LOG_ERROR, QString("The %1 line read error.").arg(y));
            ok = false;
            break;
        }
    }

    f.close();
    return ok ? img : QImage();
}

// 将 QImage 写为 raw 文件 (8/16bit 灰度，逐行紧密存储)
bool write_gray_raw_img(const QString &fileName, const QImage &img)
{
    if (img.format() != QImage::Format_Grayscale16
        && img.format() != QImage::Format_Grayscale8)
    {
        DIY_LOG(LOG_WARN, "Only QImage::Format_Grayscale8 and QImage::Format_Grayscale16 "
                          "image can be supported");
        return false;
    }

    QFile f(fileName);
    if (!f.open(QIODevice::WriteOnly))
    {
        DIY_LOG(LOG_ERROR, QString("Open file %1 error: %2").arg(fileName, f.errorString()));
        return false;
    }

    const int bpp       = img.depth() / 8;          // = 1 or 2
    const int lineBytes = img.width() * bpp;

    bool ok = true;
    for (int y = 0; y < img.height(); ++y)
    {
        const char *src = reinterpret_cast<const char*>(img.constScanLine(y));
        qint64 written = f.write(src, lineBytes);
        if (written != lineBytes)
        {
            DIY_LOG(LOG_ERROR, QString("Write error at line %1: expected %2 bytes, got %3")
                                .arg(y).arg(lineBytes).arg(written));
            ok = false;
            break;
        }
    }

    return ok;
}
