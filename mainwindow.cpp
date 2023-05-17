
#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent), ui(new Ui::MainWindow) {
    ui->setupUi(this);
    ui->imageScrollArea->setBackgroundRole(QPalette::Midlight);
    setCentralWidget(ui->imageScrollArea);
    ui->imageLabel->mainWindow = this;

    pixelLabel = new QLabel(this);
    pixelLabel->setForegroundRole(QPalette::Shadow);
    ui->statusbar->addPermanentWidget(pixelLabel);

    seqRB = new QRadioButton("Sequential", this);
    ompRB = new QRadioButton("OpenMP", this);
    mpiRB = new QRadioButton("MPI", this);
    seqRB->setChecked(true);
    ui->statusbar->addPermanentWidget(seqRB);
    ui->statusbar->addPermanentWidget(ompRB);
    ui->statusbar->addPermanentWidget(mpiRB);

    ui->statusbar->showMessage("Hello and welcome to Mini-Photoshop!");
}

MainWindow::~MainWindow() {
    delete ui;
}

QColor MainWindow::getPixelAt(unsigned int x, unsigned int y) {
    return image.pixel(QPoint(x, y));
}

static void initializeImageFileDialog(QFileDialog& dialog, QFileDialog::AcceptMode acceptMode) {
    static bool firstPrompt = true;

    if (firstPrompt) {
        firstPrompt = false;
        const QStringList downloadLocations = QStandardPaths::standardLocations(QStandardPaths::DownloadLocation);
        dialog.setDirectory(downloadLocations.isEmpty() ? QDir::currentPath() : downloadLocations.last());
    }

    QStringList mimeTypeFilters;
    const QByteArrayList supportedMimeTypes = (acceptMode == QFileDialog::AcceptOpen) ? QImageReader::supportedMimeTypes() : QImageWriter::supportedMimeTypes();
    for (const QByteArray& mimeTypeName : supportedMimeTypes) mimeTypeFilters.append(mimeTypeName);
    mimeTypeFilters.sort();
    dialog.setMimeTypeFilters(mimeTypeFilters);
    dialog.selectMimeTypeFilter("image/jpeg");
    dialog.setAcceptMode(acceptMode);
    if (acceptMode == QFileDialog::AcceptSave) dialog.setDefaultSuffix("jpg");
}

bool MainWindow::loadFile(const QString& fileName, bool retainScaleFactor = false) {
    QImageReader reader(fileName);
    reader.setAutoTransform(true);
    image = reader.read();
    if (image.isNull()) {
        QMessageBox::information(
            this,
            QGuiApplication::applicationDisplayName(),
            tr("Cannot load %1: %2").arg(QDir::toNativeSeparators(fileName), reader.errorString())
        );
        return false;
    }
    if (retainScaleFactor == false) scaleFactor = 1.0;
    ui->imageLabel->setPixmap(QPixmap::fromImage(image));
    ui->imageLabel->setFixedSize(image.size());
    ui->statusbar->showMessage(QString("Loaded image of size: %1x%2").arg(ui->imageLabel->width()).arg(ui->imageLabel->height()));
    return true;
}

bool MainWindow::saveFile(const QString& fileName) {
    QImageWriter writer(fileName);
    writer.setQuality(100);
    writer.setCompression(0);
    writer.setOptimizedWrite(true);
    writer.setProgressiveScanWrite(true);
    if (!writer.write(image)) {
        QMessageBox::information(
            this,
            QGuiApplication::applicationDisplayName(),
            tr("Cannot write %1: %2").arg(QDir::toNativeSeparators(fileName)), writer.errorString()
        );
        return false;
    }
    return true;
}

QByteArray MainWindow::getImageBytesAndRemoveHeader() {
    QByteArray ba;
    QBuffer buff(&ba);
    buff.open(QIODeviceBase::WriteOnly);
    image.save(&buff, "PPM");
    // The PPM format puts data in the form of a header that spans three lines then all the pixels
    // So here I strip out the header by counting lines
    uchar newLineOccurrences = 0, index = 0;
    while (newLineOccurrences < 3) {
        if (ba.at(index) == '\n') newLineOccurrences++;
        index++;
    }
    imageHeader = QString(ba.left(index));
    return ba.mid(index);
}

void MainWindow::runScript(const QString& scriptNamePrefix) {
    QFile temp(QApplication::applicationDirPath().append("/bin/temp"));
    temp.open(QIODevice::WriteOnly);
    QByteArray ba = getImageBytesAndRemoveHeader();
    const char separator = 0;
    temp.write(QString::number(image.height()).toUtf8());
    temp.write(&separator, 1);
    temp.write(QString::number(image.width()).toUtf8());
    temp.write(&separator, 1);
    temp.write(ba);
    temp.flush();
    temp.close();

    QProcess* process = new QProcess(this);
    process->setWorkingDirectory(QApplication::applicationDirPath());
    if (seqRB->isChecked()) process->start(QString("bin/%1_%2.exe").arg(scriptNamePrefix, "seq"));
    else if (ompRB->isChecked()) process->start(QString("bin/%1_%2.exe").arg(scriptNamePrefix, "omp"));
    else process->start("mpiexec", QStringList() << "-n" << "4" << QString("bin/%1_%2.exe").arg(scriptNamePrefix, "mpi"));
    process->waitForFinished();
    printf(process->readAllStandardError());
    fflush(stdout);
    delete process;

    temp.open(QIODevice::ReadOnly);
    image.loadFromData(temp.read(ba.size()).prepend(imageHeader.toUtf8()), "PPM");
    scaleImagePixmap();
    ui->statusbar->showMessage(QString("Time taken: %1 ms").arg(temp.readAll()));
    temp.close();
    temp.remove();
}

void MainWindow::on_actionOpen_triggered() {
    QFileDialog dialog(this, tr("Open File"));
    initializeImageFileDialog(dialog, QFileDialog::AcceptOpen);
    currentImageFileLocation = NULL;
    while (dialog.exec() == QDialog::Accepted) {
        currentImageFileLocation = dialog.selectedFiles().constFirst();
        if (loadFile(currentImageFileLocation) == true) break;
    }
}

void MainWindow::on_actionRestore_triggered() {
    loadFile(currentImageFileLocation, true);
    scaleImagePixmap();
    ui->statusbar->showMessage(QString("Restored image (zoom scale factor retained)"));
}

void MainWindow::on_actionSave_triggered() {
    QFileDialog dialog(this, tr("Save File"));
    initializeImageFileDialog(dialog, QFileDialog::AcceptSave);
    while (dialog.exec() == QDialog::Accepted && !saveFile(dialog.selectedFiles().constFirst())) {}
}

void MainWindow::on_actionExit_triggered() {
    QCoreApplication::quit();
}

void MainWindow::on_actionBlur_triggered() {
    runScript(QString("lpf"));
}

void MainWindow::on_actionSharpen_triggered() {
    runScript(QString("hpf"));
}

void MainWindow::on_actionCluster_triggered() {
    runScript(QString("cluster"));
}

void MainWindow::on_actionEqualize_Histogram_triggered() {
    runScript(QString("equalize"));
}

void MainWindow::on_actionInvert_triggered() {
    runScript(QString("invert"));
}

void MainWindow::on_selNoneBtn_clicked() {
//    TODO
}

void MainWindow::on_selRectBtn_clicked() {
//    TODO
//    ui->imageLabel->pixmap().copy(); can take a rectangle argument!
}

void MainWindow::on_selEllipseBtn_clicked() {
//    TODO
}

void MainWindow::on_selLassoBtn_clicked() {
//    TODO
}

void MainWindow::scaleImagePixmap() {
    ui->imageLabel->setPixmap(
        QPixmap::fromImage(image).scaled(
            QPixmap::fromImage(image).width() * scaleFactor,
            QPixmap::fromImage(image).height() * scaleFactor,
            Qt::KeepAspectRatio,
            Qt::FastTransformation
        )
    );
    ui->imageLabel->setFixedSize(ui->imageLabel->pixmap().size());
}

void MainWindow::on_zoomInBtn_clicked() {
    // Arbitrarily setting the zoom-in limit to 1024 * 1024 * 255
    if ((long long) ui->imageLabel->width() * (long long) ui->imageLabel->height() > (long long) 267386880) {
        ui->statusbar->showMessage("You've zoomed in too far! Please be kinder to your memory... :(");
    } else {
        scaleFactor *= 1.25;
        scaleImagePixmap();
        ui->statusbar->showMessage(QString("Enlarged to: %1x%2").arg(ui->imageLabel->width()).arg(ui->imageLabel->height()));
    }
}

void MainWindow::on_zoomFitBtn_clicked() {
    scaleFactor = fmin(
        float(ui->imageScrollArea->width()) / float(image.width()),
        float(ui->imageScrollArea->height()) / float(image.height())) * 0.9;
    scaleImagePixmap();
    ui->statusbar->showMessage(QString("Image fit to display"));
}

void MainWindow::on_zoomRestoreBtn_clicked() {
    scaleFactor = 1.0f;
    scaleImagePixmap();
    ui->statusbar->showMessage(QString("Restored to: %1x%2").arg(ui->imageLabel->width()).arg(ui->imageLabel->height()));
}

void MainWindow::on_zoomOutBtn_clicked() {
    // Arbitrarily setting the zoom-out limit to 255
    if ((long long) ui->imageLabel->width() * (long long) ui->imageLabel->height() < 255) {
        ui->statusbar->showMessage("That's enough zooming out, the pic has been obliterated...");
    } else {
        scaleFactor *= 0.8f;
        scaleImagePixmap();
        ui->statusbar->showMessage(QString("Shrunk to: %1x%2").arg(ui->imageLabel->width()).arg(ui->imageLabel->height()));
    }
}
