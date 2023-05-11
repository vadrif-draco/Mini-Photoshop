
#ifndef MYQLABEL_H
#define MYQLABEL_H

#include <QLabel>
#include <QMouseEvent>
#include <QMainWindow>
#include <mainwindow.h>

class MyQLabel : public QLabel {
public:
    explicit MyQLabel(QWidget*& widget) : QLabel(widget) {}
    ~MyQLabel() {}
    MainWindow* mainWindow = nullptr;
protected:
    void mouseMoveEvent(QMouseEvent* event);
};

#endif // MYQLABEL_H
