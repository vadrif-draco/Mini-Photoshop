
#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent), ui(new Ui::MainWindow) {
    ui->setupUi(this);
    ui->imageLabel->setScaledContents(true);
    ui->imageScrollArea->setBackgroundRole(QPalette::Midlight);
    this->setCentralWidget(ui->imageScrollArea);

    seqRB = new QRadioButton("Sequential", this);
    ompRB = new QRadioButton("OpenMP", this);
    mpiRB = new QRadioButton("MPI", this);
    ui->statusbar->addPermanentWidget(seqRB);
    ui->statusbar->addPermanentWidget(ompRB);
    ui->statusbar->addPermanentWidget(mpiRB);
    seqRB->setChecked(true);
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

bool MainWindow::loadFile(const QString& fileName) {
    QImageReader reader(fileName);
    reader.setAutoTransform(true);
    QImage image = reader.read();
    if (image.isNull()) {
        QMessageBox::information(
            this,
            QGuiApplication::applicationDisplayName(),
            tr("Cannot load %1: %2").arg(QDir::toNativeSeparators(fileName), reader.errorString())
        );
        return false;
    }
    this->scaleFactor = 1.0;
    ui->imageLabel->setPixmap(QPixmap::fromImage(image));
    ui->imageLabel->setFixedSize(image.size());
    ui->statusbar->showMessage(QString("Loaded image of size: %1x%2").arg(ui->imageLabel->width()).arg(ui->imageLabel->height()));
    return true;
}

bool MainWindow::saveFile(const QString& fileName) {
    QImageWriter writer(fileName);

    if (!writer.write(ui->imageLabel->pixmap().toImage())) {
        QMessageBox::information(
            this,
            QGuiApplication::applicationDisplayName(),
            tr("Cannot write %1: %2").arg(QDir::toNativeSeparators(fileName)), writer.errorString()
        );
        return false;
    }
    return true;
}

QByteArray MainWindow::convertQPixmapToHeaderlessQByteArray(QPixmap pixmap) {
    QByteArray ba;
    QBuffer buff(&ba);
    buff.open(QIODeviceBase::WriteOnly);
    pixmap.save(&buff, "PPM");
    // The PPM format puts data in the form of a header that spans three lines then all the pixels
    // So here I strip out the header by counting lines
    uchar newLineOccurrences = 0, index = 0;
    while (newLineOccurrences < 3) {
        if (ba.at(index) == '\n') newLineOccurrences++;
        index++;
    }
    this->header = QString(ba.left(index));
    return ba.mid(index);
}

QPixmap MainWindow::convertHeaderlessQByteArrayToQPixmap(QByteArray ba) {
    QPixmap pixmap;
    pixmap.loadFromData(ba.prepend(this->header.toUtf8()), "PPM");
    return pixmap.copy();
}

void MainWindow::run_script(const QString& scriptNamePrefix) {
    QFile temp(QApplication::applicationDirPath().append("/bin/temp"));
    temp.open(QIODevice::WriteOnly);
    QByteArray ba = convertQPixmapToHeaderlessQByteArray(ui->imageLabel->pixmap());
//    for (int i = 0; i < ba.size(); i++) printf("%d ", (unsigned char) ba[i]);
//    fflush(stdout);
    const char separator = 0;
    temp.write(QString::number(ui->imageLabel->pixmap().height()).toUtf8());
    temp.write(&separator, 1);
    temp.write(QString::number(ui->imageLabel->pixmap().width()).toUtf8());
    temp.write(&separator, 1);
    temp.write(ba);
    temp.flush();
    temp.close();

    QProcess* process = new QProcess(this);
    process->setWorkingDirectory(QApplication::applicationDirPath());
    if (seqRB->isChecked()) process->start(QString("bin/%1_%2.exe").arg(scriptNamePrefix, "seq"));
    else if (ompRB->isChecked()) process->start(QString("bin/%1_%2.exe").arg(scriptNamePrefix, "omp"));
    else process->start("mpiexec", QStringList() << "-n 16" << QString("bin/%1_%2.exe").arg(scriptNamePrefix, "mpi"));
    process->waitForFinished();
    printf(process->readAllStandardError());
    fflush(stdout);
    delete process;

    temp.open(QIODevice::ReadOnly);
    ui->imageLabel->setPixmap(convertHeaderlessQByteArrayToQPixmap(temp.read(ba.size())));
    ui->statusbar->showMessage(QString("Time taken: %1 ms").arg(temp.readAll()));
    temp.close();
    temp.remove();
}

void MainWindow::on_actionOpen_triggered() {
    QFileDialog dialog(this, tr("Open File"));
    initializeImageFileDialog(dialog, QFileDialog::AcceptOpen);
    while (dialog.exec() == QDialog::Accepted && !loadFile(dialog.selectedFiles().constFirst())) {}
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
    run_script(QString("lpf"));
}

void MainWindow::on_actionSharpen_triggered() {
    run_script(QString("hpf"));
}

void MainWindow::on_actionCluster_triggered() {
    run_script(QString("cluster"));
}

void MainWindow::on_actionEqualize_Histogram_triggered() {
    run_script(QString("equalize"));
}

void MainWindow::on_actionInvert_triggered() {
    run_script(QString("invert"));
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

void MainWindow::on_zoomInBtn_clicked() {
    this->scaleFactor *= 1.25;
    ui->imageLabel->setFixedSize(this->scaleFactor * ui->imageLabel->pixmap(Qt::ReturnByValue).size());
    ui->statusbar->showMessage(QString("Enlarged to: %1x%2").arg(ui->imageLabel->width()).arg(ui->imageLabel->height()));
}

void MainWindow::on_zoomFitBtn_clicked() {
    this->scaleFactor = 1.0f;
    ui->imageLabel->setFixedSize(this->scaleFactor * ui->imageLabel->pixmap(Qt::ReturnByValue).size());
    ui->statusbar->showMessage(QString("Restored to: %1x%2").arg(ui->imageLabel->width()).arg(ui->imageLabel->height()));
}

void MainWindow::on_zoomOutBtn_clicked() {
    this->scaleFactor *= 0.8f;
    ui->imageLabel->setFixedSize(this->scaleFactor * ui->imageLabel->pixmap(Qt::ReturnByValue).size());
    ui->statusbar->showMessage(QString("Shrunk to: %1x%2").arg(ui->imageLabel->width()).arg(ui->imageLabel->height()));
}
