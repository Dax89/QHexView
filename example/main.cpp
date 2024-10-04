#include <QApplication>
#include <QHexView/qhexview.h>

int main(int argc, char** argv) {
    QApplication a{argc, argv};

    QHexDocument* doc =
        QHexDocument::fromFile(QApplication::applicationFilePath());

    QHexView view;
    view.setDocument(doc); // No parent, take the ownership of 'doc'
    view.show();

    return a.exec();
}
