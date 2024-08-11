#include "qhexview.h"

#include <QApplication>
#include <QByteArray>
#include <QFile>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // Get data:
    QFile file(QApplication::applicationFilePath());
    QByteArray bytes;
    if (file.exists()) {
        file.open(QFile::ReadOnly);
        bytes = file.readAll();
        file.close();
    }

    // Create hexview
    QHexView view;
    view.setData(bytes);
    view.show();

    return a.exec();
}
