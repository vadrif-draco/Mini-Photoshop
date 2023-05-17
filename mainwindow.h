
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QBuffer>
#include <QByteArray>
#include <QColor>
#include <QDir>
#include <QFileDialog>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QImage>
#include <QImageReader>
#include <QImageWriter>
#include <QLabel>
#include <QMainWindow>
#include <QMessageBox>
#include <QPixmap>
#include <QProcess>
#include <QRadioButton>
#include <QRect>
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
    double scaleFactor = 1.0;
    QImage image;
    QLabel* pixelLabel;
    QString currentImageFileLocation;
    QColor getPixelAt(unsigned int x, unsigned int y);
    enum Selection { None, Rectangular, Elliptical, Lasso };
    Selection getCurrentSelectionMode();

private slots:
    void on_actionOpen_triggered();

    void on_actionRestore_triggered();

    void on_actionSave_triggered();

    void on_actionExit_triggered();

    void on_actionBlur_triggered();

    void on_actionSharpen_triggered();

    void on_actionCluster_triggered();

    void on_actionEqualize_Histogram_triggered();

    void on_actionInvert_triggered();

    void on_zoomInBtn_clicked();

    void on_zoomFitBtn_clicked();

    void on_zoomOutBtn_clicked();

    void on_zoomRestoreBtn_clicked();

private:
    Ui::MainWindow* ui;
    QString imageHeader;
    QRadioButton* seqRB;
    QRadioButton* ompRB;
    QRadioButton* mpiRB;
    bool loadFile(const QString& fileName, bool retainScaleFactor);
    bool saveFile(const QString& fileName);
    void runScript(const QString& scriptNamePrefix);
    void scaleImagePixmap();
    QByteArray getImageBytesAndRemoveHeader(QImage*);
};

#endif // MAINWINDOW_H
