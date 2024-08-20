#include <QApplication>
#include <qhexview.h>

int main(int argc, char** argv)
{
    QApplication a{argc, argv};

    QHexDocument* const doc = QHexDocument::fromFile(QApplication::applicationFilePath());

    QHexView view;
    view.setDocument(doc); // No parent, it takes the ownership of 'doc'
    view.show();

    return a.exec();
}
