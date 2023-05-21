
#ifndef MYIMAGEQLABEL_H
#define MYIMAGEQLABEL_H

#include <QLabel>
#include <QList>
#include <QMouseEvent>
#include <QObject>
#include <QPainter>
#include <QPainterPath>
#include <QPixmap>
#include <QPoint>
#include <QRegion>

#include "selection.h"

class MyImageQLabel : public QLabel {

    Q_OBJECT

public:
    explicit MyImageQLabel(QWidget*& widget) : QLabel(widget) {}
    ~MyImageQLabel() {}
    QImage loadedImage;
    double getScaleFactor();
    void setScaleFactor(double);
    void scaleImagePixmap();
    void setSelectionMode(Selection);
    QPixmap getSelectionPixmap();
    void replaceSelectionPixmap(QPixmap);
    QPoint getPixelCoordinatesFromOriginalImageAt(QPoint);

signals:
    void pixelTextInfo(const QString&);

protected:
    void mousePressEvent(QMouseEvent*) override;
    void mouseMoveEvent(QMouseEvent*) override;
    void mouseReleaseEvent(QMouseEvent*) override;

private:
    bool selecting = false;
    bool noSelection = true;
    double scaleFactor = 1.0;
    QPoint selectionEnd;
    QPoint selectionStart;
    Selection selectionMode;
    QPoint selectionTopLeft;
    QPoint selectionBottRight;
    QList<QPoint> lassoPoints;
    void clearSelectionArea();
    void displayPixelInfoAtPoint(QPoint);
    void highlightPixelAtPoint(QPoint);
    void updateSelectionArea(QPoint);
    void drawSelectionArea();
};

#endif // MYIMAGEQLABEL_H
