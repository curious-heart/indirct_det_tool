#include <QApplication>
#include <QThread>

#include "widget.h"

#include "version_def/version_def.h"
#include "logger/logger.h"

int main(int argc, char *argv[])
{
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    if (QT_VERSION >= QT_VERSION_CHECK(5, 6, 0))
    {
        QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
        QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
    }
#endif

    // 创建Qt应用程序对象
    QApplication app(argc, argv); 
    
    QThread th;
    start_log_thread(th);

    // 创建主窗口Widget
    Widget widget;

    QString title_str = QString("%1 %2").arg(app.applicationName(), APP_VER_STR);
    widget.setWindowTitle(title_str);

    // 显示窗口
    widget.show();
    
    // 运行应用程序事件循环
    int app_ret = app.exec();

    end_log_thread(th);

    return app_ret;
}
