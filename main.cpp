#include <QApplication>
#include "window.h"

int main(int argc, char ** argv)
{
    QApplication app(argc, argv);

    Window wnd;
    wnd.show();

    return app.exec();
}