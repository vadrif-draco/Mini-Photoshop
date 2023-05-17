
#ifndef MYQLABEL_H
#define MYQLABEL_H

#include <QLabel>

#include <QMainWindow>
#include <QMouseEvent>
#include <QPainter>
#include <QPixmap>
#include <QPoint>

#include <mainwindow.h>

class MyQLabel : public QLabel {
public:
    explicit MyQLabel(QWidget*& widget) : QLabel(widget) {}
    ~MyQLabel() {}
    MainWindow* mainWindow = nullptr;
    QPoint* selectionStart = new QPoint();
    QPoint* selectionEnd = new QPoint();

protected:
    void mouseMoveEvent(QMouseEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

private:
    QPainter* painter;
    bool selecting = false;
};

#endif // MYQLABEL_H
