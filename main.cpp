#include <QApplication>

#include "widget.h"

int main(int argc, char *argv[])
{
    // 创建Qt应用程序对象
    QApplication app(argc, argv); 
    
    // 创建主窗口Widget
    Widget widget;

    // 显示窗口
    widget.show();
    
    // 运行应用程序事件循环
    return app.exec();
}