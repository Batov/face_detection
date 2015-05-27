#include <QtCore>


#include <QObject>
#include <QDebug>

#warning TEMPORARY
#if 1
#include <QTimer>
#endif


#include "internal/main.h"
#include "internal/v4l2.h"
#include "internal/codecengine.h"


int main(int _argc, char* _argv[])
{
    QCoreApplication a(_argc, _argv);

#warning TEMPORARY
#if 1
    trik::V4L2 v4l2(NULL);
    v4l2.open();
    v4l2.start();

    QTimer timeout(NULL);
    timeout.setSingleShot(false);
    timeout.setInterval(5000);
    v4l2.connect(&timeout, SIGNAL(timeout()), &v4l2, SLOT(reportFps()));
    timeout.start();
#endif

    return a.exec();
}
