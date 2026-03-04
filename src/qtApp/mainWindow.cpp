// mainWindow.cpp
//
// Qt6 main window implementation for euterpe_qt.

#include "mainWindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QFileDialog>
#include <QApplication>
#include <QStatusBar>

MainWindow::MainWindow(GUIBase* params, QWidget* parent)
    : QMainWindow(parent)
    , params_(params) {
    setWindowTitle("Euterpe");
    setMinimumWidth(420);

    auto* central = new QWidget(this);
    auto* vbox    = new QVBoxLayout(central);

    // --- Key slider ---
    auto* keyGroup  = new QGroupBox("Key (semitones)", this);
    auto* keyLayout = new QHBoxLayout(keyGroup);
    keySlider_ = new QSlider(Qt::Horizontal, this);
    keySlider_->setRange(-12, 12);
    keySlider_->setValue(0);
    keySlider_->setTickInterval(1);
    keySlider_->setTickPosition(QSlider::TicksBelow);
    keyLabel_  = new QLabel("0", this);
    keyLabel_->setMinimumWidth(30);
    keyLayout->addWidget(keySlider_);
    keyLayout->addWidget(keyLabel_);
    connect(keySlider_, &QSlider::valueChanged, this, &MainWindow::onKeyChanged);

    // --- Volume slider ---
    auto* volGroup  = new QGroupBox("Volume", this);
    auto* volLayout = new QHBoxLayout(volGroup);
    volumeSlider_ = new QSlider(Qt::Horizontal, this);
    volumeSlider_->setRange(0, 200);
    volumeSlider_->setValue(100);
    volumeLabel_  = new QLabel("100", this);
    volumeLabel_->setMinimumWidth(35);
    volLayout->addWidget(volumeSlider_);
    volLayout->addWidget(volumeLabel_);
    connect(volumeSlider_, &QSlider::valueChanged, this, &MainWindow::onVolumeChanged);

    // --- Input source ---
    auto* srcGroup  = new QGroupBox("Input Source", this);
    auto* srcLayout = new QVBoxLayout(srcGroup);
    lineInRadio_ = new QRadioButton("Line-in (microphone)", this);
    fileRadio_   = new QRadioButton("Audio file (WAV)", this);
    lineInRadio_->setChecked(true);

    auto* fileRow   = new QHBoxLayout();
    fileSelectBtn_  = new QPushButton("Select file...", this);
    fileSelectBtn_->setEnabled(false);
    filePathLabel_  = new QLabel("(no file selected)", this);
    filePathLabel_->setStyleSheet("color: gray;");
    fileRow->addWidget(fileSelectBtn_);
    fileRow->addWidget(filePathLabel_, 1);

    srcLayout->addWidget(lineInRadio_);
    srcLayout->addWidget(fileRadio_);
    srcLayout->addLayout(fileRow);

    connect(fileRadio_, &QRadioButton::toggled, fileSelectBtn_, &QPushButton::setEnabled);
    connect(fileSelectBtn_, &QPushButton::clicked, this, &MainWindow::onFileSelectClicked);

    // --- Buttons ---
    auto* btnRow = new QHBoxLayout();
    startBtn_ = new QPushButton("Start", this);
    quitBtn_  = new QPushButton("Quit", this);
    startBtn_->setStyleSheet("background-color: #336633; color: white; font-weight: bold;");
    quitBtn_->setStyleSheet("background-color: #663333; color: white;");
    btnRow->addWidget(startBtn_);
    btnRow->addWidget(quitBtn_);
    connect(startBtn_, &QPushButton::clicked, this, &MainWindow::onStartClicked);
    connect(quitBtn_,  &QPushButton::clicked, this, [this]() {
        params_->setquit();
        QApplication::quit();
    });

    vbox->addWidget(keyGroup);
    vbox->addWidget(volGroup);
    vbox->addWidget(srcGroup);
    vbox->addLayout(btnRow);
    setCentralWidget(central);
    statusBar()->showMessage("Ready");
}

void MainWindow::onKeyChanged(int value) {
    keyLabel_->setText(QString::number(value));
    params_->key.set(value);
}

void MainWindow::onVolumeChanged(int value) {
    volumeLabel_->setText(QString::number(value));
    params_->volume.set(value);
}

void MainWindow::onFileSelectClicked() {
    QString path = QFileDialog::getOpenFileName(
        this, "Select audio file", QString(),
        "Audio files (*.wav *.flac *.ogg *.aif *.aiff);;All files (*)");
    if (!path.isEmpty()) {
        selectedFilePath_ = path;
        filePathLabel_->setText(QFileInfo(path).fileName());
        filePathLabel_->setStyleSheet("");
    }
}

void MainWindow::onStartClicked() {
    startBtn_->setEnabled(false);
    lineInRadio_->setEnabled(false);
    fileRadio_->setEnabled(false);
    fileSelectBtn_->setEnabled(false);
    statusBar()->showMessage("Running...");
    emit startRequested();
}

std::filesystem::path MainWindow::selectedFile() const {
    return selectedFilePath_.toStdString();
}

bool MainWindow::isFileMode() const {
    return fileRadio_->isChecked();
}
