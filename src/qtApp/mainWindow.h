// mainWindow.h
//
// Qt6 main window for euterpe_qt.
// Provides sliders for key/volume, input source selection (Line-in / File),
// and Start/Quit buttons.

#pragma once
#include <QMainWindow>
#include <QSlider>
#include <QLabel>
#include <QRadioButton>
#include <QPushButton>
#include <QString>
#include <filesystem>
#include "../guiBase.h"

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(GUIBase* params, QWidget* parent = nullptr);

    // Returns selected audio file path (empty if Line-in mode selected)
    std::filesystem::path selectedFile() const;

    // Returns true if file input mode is selected
    bool isFileMode() const;

signals:
    void startRequested();
    void quitRequested();

private slots:
    void onKeyChanged(int value);
    void onVolumeChanged(int value);
    void onFileSelectClicked();
    void onStartClicked();

private:
    GUIBase*      params_;

    QSlider*      keySlider_;
    QLabel*       keyLabel_;
    QSlider*      volumeSlider_;
    QLabel*       volumeLabel_;

    QRadioButton* lineInRadio_;
    QRadioButton* fileRadio_;
    QPushButton*  fileSelectBtn_;
    QLabel*       filePathLabel_;

    QPushButton*  startBtn_;
    QPushButton*  quitBtn_;

    QString       selectedFilePath_;
};
