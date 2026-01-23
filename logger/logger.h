#ifndef LOGGER_H
#define LOGGER_H

#include <QObject>
#include <QThread>
#include <QDateTime>

#include "common_tools/common_macros.h"
#include "version_def/version_def.h"

#define LOG_LEVEL_ITEM(lvl) LOG_##lvl
#define LOG_LEVEL_LIST \
    ENUM_NAME_DEF(LOG_LEVEL_ITEM(DEBUG))\
    ENUM_NAME_DEF(LOG_LEVEL_ITEM(INFO))\
    ENUM_NAME_DEF(LOG_LEVEL_ITEM(WARN))\
    ENUM_NAME_DEF(LOG_LEVEL_ITEM(ERROR))

enum LOG_LEVEL {
    LOG_LEVEL_LIST
};
#define VALID_LOG_LVL(lvl) ((LOG_DEBUG <= (lvl)) && ((lvl) <= LOG_ERROR))
Q_DECLARE_METATYPE(LOG_LEVEL)

class Logger : public QObject
{
    Q_OBJECT
public:
    explicit Logger(QObject *parent = nullptr);

public slots:
    //void receive_log(LOG_LEVEL level, QString loc_str, QString log_str);
    void receive_log(int level, QString loc_str, QString log_str);
};

class LogSigEmitter: public QObject
{
    Q_OBJECT
signals:
    //void record_log(LOG_LEVEL level, QString loc_str, QString log_str);
    void record_log(int level, QString loc_str, QString log_str);
};
extern LogSigEmitter *g_LogSigEmitter;

/* !!!
 * DO NOT call this function directly. It is defined just for warning-elimination.
 * Use DIY_LOG macro.
*/
//void __emit_log_signal__(LOG_LEVEL level, QString loc_str, QString log);
void __emit_log_signal__(int level, QString loc_str, QString log);


/*
 * After start_log_thread is invoked with a QThread instance can DIY_LOG work.
 * Note: the life-cycle of th must expands to the whole thread.
 * E.g. you can define a QThread obj in main function, and call start/end_log_thread
 * with that obj, as below:
 *
    QThread log_thread;
    start_log_thread(log_thread);
    ....
    end_log_thread(log_thread);
 *
 * start_log_thread should be invoked as early as possible and end_log_thread
 * should be invoked as late as possible. Place them at proper position in your
 * program.
 *
*/
bool start_log_thread(QThread &th, LOG_LEVEL display_lvl = LOG_INFO);
void end_log_thread(QThread &th);
void update_log_level(LOG_LEVEL display_lvl);

extern const char* g_log_level_strs[];

/*DO NOT call this function, just use DIY_LOG macro.*/
void writeLog(int level, QString loc_str, QString msg);

/*
 * Use example:
 *     DIY_LOG(LOG_LEVEL::LOG_INFO, "info str.");
 *     QString err_str;
 *     DIY_LOG(LOG_LEVEL::LOG_WARN, err_str);
*/
#define DIY_LOG(level, log) \
    {\
        QString loc_str = QString(__FILE__) + QString("  [%1][%2]").arg(__LINE__).arg(__FUNCTION__);\
        if(g_LogSigEmitter)\
        {\
            __emit_log_signal__((int)level, loc_str, (log));\
        }\
        else\
        {\
            writeLog((level), loc_str, (log));\
        }\
    }

extern const char* log_dir_str, *log_file_str;
/*
 * Use example:
 *     DIY_LOG(LOG_LEVEL::LOG_INFO, "info str.");
 *     DIY_LOG(LOG_LEVEL::LOG_ERROR, "error code:%d", (int)err);
 *
 *     QString ws("warning...");
 *     DIY_LOG(LOG_LEVEL::LOG_WARN, "warn message:%ls", ws.utf16());
*/
/*
#define DIY_LOG(level, fmt_str, ...) \
    {\
        QString log = QString::asprintf(fmt_str, ##__VA_ARGS__);\
        QString loc_str = QString(__FILE__) + QString("  [%1]").arg(__LINE__);\
        if(g_LogSigEmitter)\
        {\
            __emit_log_signal__((int)level, loc_str, log);\
        }\
    }
*/

#define LOCAL_DIY_LOG(level, log_fn, msg) \
{\
    if((level) >= g_sys_configs_block.log_level)\
    {\
        QString level_str;\
        QFile log_file(log_fn);\
        if(log_file.open(QFile::WriteOnly | QFile::Append)) \
        {\
\
            if(!VALID_LOG_LVL(level))\
            {\
                level_str = QString("[%1]").arg("Unknown Level");\
            }\
            else\
            {\
                level_str = QString("[%1]").arg(g_log_level_strs[(level)]);\
            }\
            QString date=QDateTime::currentDateTime().toString("yyyy-MM-dd"); \
            QString time=QDateTime::currentDateTime().toString("hh:mm:ss.zzz"); \
            QString loc_str = QString(__FILE__) + QString("  [%1][%2]").arg(__LINE__).arg(__FUNCTION__);\
            QString str1, str2; \
            str1 = "\t" + loc_str + "\t@" + APP_VER_STR + "\n"; \
            str2 = date + " " + time + " " + level_str + " " + (msg) + "\n"; \
            QTextStream qts; \
            qts.setDevice(&log_file);\
            qts << str1 << str2;\
            log_file.flush();\
            log_file.close();\
        }\
    }\
}

#endif // LOGGER_H
