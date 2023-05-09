
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QBuffer>
#include <QByteArray>
#include <QDir>
#include <QFileDialog>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QImageReader>
#include <QImageWriter>
#include <QMainWindow>
#include <QMessageBox>
#include <QPixmap>
#include <QProcess>
#include <QRadioButton>
#include <QScrollArea>
#include <QScrollBar>
#include <QStandardPaths>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow

{
    Q_OBJECT

public:
    MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

private slots:
    void on_actionOpen_triggered();

    void on_actionExit_triggered();

    void on_actionSave_triggered();

    void on_actionBlur_triggered();

    void on_actionSharpen_triggered();

    void on_actionCluster_triggered();

    void on_actionEqualize_Histogram_triggered();

    void on_actionInvert_triggered();

    void on_selNoneBtn_clicked();

    void on_selRectBtn_clicked();

    void on_selEllipseBtn_clicked();

    void on_selLassoBtn_clicked();

    void on_zoomInBtn_clicked();

    void on_zoomFitBtn_clicked();

    void on_zoomOutBtn_clicked();

private:
    Ui::MainWindow* ui;
    double scaleFactor = 1;
    QString header;
    QRadioButton* seqRB;
    QRadioButton* ompRB;
    QRadioButton* mpiRB;
    bool loadFile(const QString& fileName);
    bool saveFile(const QString& fileName);
    void run_script(const QString& scriptNamePrefix);
    QByteArray convertQPixmapToHeaderlessQByteArray(QPixmap pixmap);
    QPixmap convertHeaderlessQByteArrayToQPixmap(QByteArray ba);
};

#endif // MAINWINDOW_H
