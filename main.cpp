//  main.cpp  -  program entry point

#include <QApplication>
#include <QFile>
#include <QFont>
#include <QStyleFactory>

#include "mainwindow.h"

int main(int argc, char* argv[]) {
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);  
    QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
#endif

    QApplication app(argc, argv);
    QCoreApplication::setOrganizationName(QStringLiteral("BookVerse"));
    QCoreApplication::setApplicationName(QStringLiteral("BookVerse"));


    app.setStyle(QStyleFactory::create(QStringLiteral("Fusion")));
    QFont font(QStringLiteral("Segoe UI"), 10);
    font.setStyleHint(QFont::SansSerif);
    app.setFont(font);

    QFile qss(QStringLiteral(":/style.qss"));
    if (qss.open(QIODevice::ReadOnly)) app.setStyleSheet(QString::fromUtf8(qss.readAll()));

    MainWindow window;
    window.show();
    return app.exec();
}
