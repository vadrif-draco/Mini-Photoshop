
#include "myqlabel.h"

void MyQLabel::mouseMoveEvent(QMouseEvent* event) {

    // x and y scaled down
    int x = float(event->pos().x()) / mainWindow->scaleFactor;
    int y = float(event->pos().y()) / mainWindow->scaleFactor;
    int maxWidth = float(width()) / mainWindow->scaleFactor - 1;
    int maxHeight = float(height()) / mainWindow->scaleFactor - 1;
    if (x < 0) x = 0; else if (x > maxWidth) x = maxWidth;
    if (y < 0) y = 0; else if (y > maxHeight) y = maxHeight;

    QColor colors = mainWindow->getPixelAt(x, y);
    mainWindow->pixelLabel->setText(
        QString("(%1, %2): %3 %4 %5 ")
        .arg(x)
        .arg(y)
        .arg(colors.red())
        .arg(colors.green())
        .arg(colors.blue()));

    if (selecting) {

        int _x = event->pos().x();
        int _y = event->pos().y();
        int _maxWidth = width() - 1;
        int _maxHeight = height() - 1;
        if (_x < 0) _x = 0; else if (_x > _maxWidth) _x = _maxWidth;
        if (_y < 0) _y = 0; else if (_y > _maxHeight) _y = _maxHeight;
        selectionEnd->setX(_x);
        selectionEnd->setY(_y);

        QPixmap pixmapWithSelection = QPixmap::fromImage(mainWindow->image).scaled(
            QPixmap::fromImage(mainWindow->image).width() * mainWindow->scaleFactor,
            QPixmap::fromImage(mainWindow->image).height() * mainWindow->scaleFactor,
            Qt::KeepAspectRatio,
            Qt::FastTransformation
        );
        QPainter painter(&pixmapWithSelection);
        painter.setBrush(QBrush(QColor(0, 0, 255, 31)));
        painter.setPen(QPen(QBrush(QColor(0, 0, 0, 127)), 2));
        switch (mainWindow->getCurrentSelectionMode()) {
            case MainWindow::Selection::None:
                break;
            case MainWindow::Selection::Rectangular:
                painter.drawRect(QRect(*selectionStart, *selectionEnd));
                break;
            case MainWindow::Selection::Elliptical:
                break;
            case MainWindow::Selection::Lasso:
                break;
            default:
                break;
        }
        this->setPixmap(pixmapWithSelection);
    }

    //    TODO:
    //    highlight currently hovered-over pixel
};

void MyQLabel::mousePressEvent(QMouseEvent* event) {

    selecting = true;
    int x = event->pos().x();
    int y = event->pos().y();
    selectionStart->setX(x);
    selectionStart->setY(y);

}

void MyQLabel::mouseReleaseEvent(QMouseEvent* event) {

    selecting = false;

}
