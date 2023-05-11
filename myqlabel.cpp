
#include "myqlabel.h"

void MyQLabel::mouseMoveEvent(QMouseEvent* event) {
    unsigned int x = float(event->pos().x()) / mainWindow->scaleFactor;
    unsigned int y = float(event->pos().y()) / mainWindow->scaleFactor;
    QColor colors = mainWindow->getPixelAt(x, y);
    mainWindow->pixelLabel->setText(
        QString("(%1, %2): %3 %4 %5 ")
            .arg(x)
            .arg(y)
            .arg(colors.red())
            .arg(colors.green())
            .arg(colors.blue()));
//    TODO:
//    mainWindow->highlightPixelAt(x, y);
};
