
#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent), ui(new Ui::MainWindow) {
    ui->setupUi(this);

    ui->imageScrollArea->setBackgroundRole(QPalette::Midlight);
    setCentralWidget(ui->imageScrollArea);

    pixelInfoLabel = new QLabel(this);
    pixelInfoLabel->setForegroundRole(QPalette::Shadow);
    QObject::connect(ui->imageLabel, &MyImageQLabel::pixelTextInfo, pixelInfoLabel, &QLabel::setText);
    ui->statusbar->addPermanentWidget(pixelInfoLabel);

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
    ui->imageLabel->loadedImage = reader.read();
    if (ui->imageLabel->loadedImage.isNull()) {
        QMessageBox::information(
            this,
            QGuiApplication::applicationDisplayName(),
            tr("Cannot load %1: %2").arg(QDir::toNativeSeparators(fileName), reader.errorString())
        );
        return false;
    }
    if (retainScaleFactor == false) ui->imageLabel->setScaleFactor(1.0);
    ui->imageLabel->setPixmap(QPixmap::fromImage(ui->imageLabel->loadedImage));
    ui->imageLabel->setFixedSize(ui->imageLabel->loadedImage.size());
    ui->statusbar->showMessage(QString("Loaded image of size: %1x%2").arg(ui->imageLabel->width()).arg(ui->imageLabel->height()));
    ui->selNoneBtn->toggle();
    on_selNoneBtn_toggled(true);
    return true;
}

bool MainWindow::saveFile(const QString& fileName) {
    QImageWriter writer(fileName);
    writer.setQuality(100);
    writer.setCompression(0);
    writer.setOptimizedWrite(true);
    writer.setProgressiveScanWrite(true);
    if (!writer.write(ui->imageLabel->loadedImage)) {
        QMessageBox::information(
            this,
            QGuiApplication::applicationDisplayName(),
            tr("Cannot write %1: %2").arg(QDir::toNativeSeparators(fileName)), writer.errorString()
        );
        return false;
    }
    return true;
}

QByteArray MainWindow::getImageBytesAndRemoveHeader(QImage imageIn) {
    QByteArray ba;
    QBuffer buff(&ba);
    buff.open(QIODeviceBase::WriteOnly);
    imageIn.save(&buff, "PPM");
    // The PPM format puts data in the form of a header that spans three lines then all the pixels
    // So here I strip out the header by counting lines
    uchar newLineOccurrences = 0, index = 0;
    while (newLineOccurrences < 3) {
        if (ba.at(index) == '\n') newLineOccurrences++;
        index++;
    }
    loadedImageHeader = QString(ba.left(index));
    return ba.mid(index);
}

void MainWindow::runScript(const QString& scriptNamePrefix) {
    // First, get the current selection as a QImage
    QImage selectionToProcess(ui->imageLabel->getSelectionPixmap().toImage());

    // Next, create a temporary file and write the pixel data into it with our custom header in place of the image's header
    QFile temp(QApplication::applicationDirPath().append("/bin/temp"));
    temp.open(QIODevice::WriteOnly);
    QByteArray ba = getImageBytesAndRemoveHeader(selectionToProcess);
    const char separator = 0;
    temp.write(QString::number(selectionToProcess.height()).toUtf8());
    temp.write(&separator, 1);
    temp.write(QString::number(selectionToProcess.width()).toUtf8());
    temp.write(&separator, 1);
    temp.write(ba);
    temp.flush();
    temp.close();

    // Now create a new process to run the currently chosen script
    QProcess process(this);
    process.setWorkingDirectory(QApplication::applicationDirPath());
    if (seqRB->isChecked()) process.start(QString("bin/%1_%2.exe").arg(scriptNamePrefix, "seq"));
    else if (ompRB->isChecked()) process.start(QString("bin/%1_%2.exe").arg(scriptNamePrefix, "omp"));
    else process.start("mpiexec", QStringList() << "-n" << "16" << QString("bin/%1_%2.exe").arg(scriptNamePrefix, "mpi"));
    process.waitForFinished();
    printf(process.readAllStandardOutput());
    printf(process.readAllStandardError());
    fflush(stdout);

    // Then, open the temporary file once more (it should've been processed and overwritten by the script) and load it as a QPixmap
    temp.open(QIODevice::ReadOnly);
    QPixmap processedSelection;
    processedSelection.loadFromData(temp.read(ba.size()).prepend(loadedImageHeader.toUtf8()), "PPM");

    // Replace the selected pixmap area with this pixmap and update the status bar to show the time that was taken to process it
    ui->imageLabel->replaceSelectionPixmap(processedSelection);
    ui->statusbar->showMessage(QString("Time taken: %1 ms").arg(temp.readAll()));
    temp.close();
    temp.remove();
}

void MainWindow::on_actionOpen_triggered() {
    QFileDialog dialog(this, tr("Open File"));
    initializeImageFileDialog(dialog, QFileDialog::AcceptOpen);
    while (dialog.exec() == QDialog::Accepted) {
        currentImageFileLocation = dialog.selectedFiles().constFirst();
        if (loadFile(currentImageFileLocation) == true) break;
    }
}

void MainWindow::on_actionRestore_triggered() {
    loadFile(currentImageFileLocation, true);
    ui->imageLabel->scaleImagePixmap();
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

void MainWindow::on_actionEqualizeHistogram_triggered() {
    runScript(QString("equalize"));
}

void MainWindow::on_actionInvert_triggered() {
    runScript(QString("invert"));
}

void MainWindow::on_selNoneBtn_toggled(bool checked) {
    if (checked) {
        ui->imageLabel->setSelectionMode(Selection::None);
    }
}


void MainWindow::on_selRectBtn_toggled(bool checked) {
    if (checked) {
        ui->imageLabel->setSelectionMode(Selection::Rectangular);
    }
}


void MainWindow::on_selEllipseBtn_toggled(bool checked) {
    if (checked) {
        ui->imageLabel->setSelectionMode(Selection::Elliptical);
    }
}


void MainWindow::on_selLassoBtn_toggled(bool checked) {
    if (checked) {
        ui->imageLabel->setSelectionMode(Selection::Lasso);
    }
}

void MainWindow::on_zoomInBtn_clicked() {
    // Arbitrarily setting the zoom-in limit to 1024 * 1024 * 255
    if ((long long) ui->imageLabel->width() * (long long) ui->imageLabel->height() > (long long) 267386880) {
        ui->statusbar->showMessage("You've zoomed in too far! Please be kinder to your memory... :(");
    } else {
        ui->imageLabel->setScaleFactor(ui->imageLabel->getScaleFactor() * 1.25);
        ui->imageLabel->scaleImagePixmap();
        ui->statusbar->showMessage(QString("Enlarged to: %1x%2").arg(ui->imageLabel->width()).arg(ui->imageLabel->height()));
    }
}

void MainWindow::on_zoomFitBtn_clicked() {
    ui->imageLabel->setScaleFactor(
        fmin(
            float(ui->imageScrollArea->width()) / float(ui->imageLabel->loadedImage.width()),
            float(ui->imageScrollArea->height()) / float(ui->imageLabel->loadedImage.height())
        ) * 0.9);
    ui->imageLabel->scaleImagePixmap();
    ui->statusbar->showMessage(QString("Image fit to display"));
}

void MainWindow::on_zoomRestoreBtn_clicked() {
    ui->imageLabel->setScaleFactor(1.0);
    ui->imageLabel->scaleImagePixmap();
    ui->statusbar->showMessage(QString("Restored to: %1x%2").arg(ui->imageLabel->width()).arg(ui->imageLabel->height()));
}

void MainWindow::on_zoomOutBtn_clicked() {
    // Arbitrarily setting the zoom-out limit to 255
    if ((long long) ui->imageLabel->width() * (long long) ui->imageLabel->height() < 255) {
        ui->statusbar->showMessage("That's enough zooming out, the pic has been obliterated...");
    } else {
        ui->imageLabel->setScaleFactor(ui->imageLabel->getScaleFactor() * 0.8);
        ui->imageLabel->scaleImagePixmap();
        ui->statusbar->showMessage(QString("Shrunk to: %1x%2").arg(ui->imageLabel->width()).arg(ui->imageLabel->height()));
    }
}
