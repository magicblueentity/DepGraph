#include <QApplication>
#include <QFile>
#include <QTextStream>

#include "gui/MainWindow.h"

static void loadAppStyle()
{
    QFile f(":/style.qss");
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
        return;
    QTextStream ts(&f);
    qApp->setStyleSheet(ts.readAll());
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QApplication::setApplicationName("DepGraph");
    QApplication::setOrganizationName("DepGraph");

    loadAppStyle();

    MainWindow w;
    w.resize(1400, 850);
    w.show();

    return app.exec();
}
