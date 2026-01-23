# 项目类型: 应用程序
TEMPLATE = app

# 输出目录配置
DESTDIR = 

# QT模块配置
QT += core gui network
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
# 编译配置
CONFIG += c++17 console

# 平台特定配置
win32 {
    # 防止包含冗余的 Windows 头文件
    DEFINES += WIN32_LEAN_AND_MEAN
    DEFINES += NOMINMAX
}

# 头文件配置
HEADERS += \
           widget.h
# 源文件配置
SOURCES += main.cpp \
           widget.cpp
# UI 文件配置
FORMS += \
    widget.ui
# 资源文件配置
RESOURCES +=

# 包含路径配置
INCLUDEPATH += $$PWD
# 链接库配置
LIBS += -L$$PWD -lL103Controller
