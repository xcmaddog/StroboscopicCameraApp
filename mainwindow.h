#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#pragma once

#include <QMainWindow>
#include <memory>

class QThread;
class QScrollArea;
class QSplitter;
class QStatusBar;

class CameraController;
class CameraWorker;
class CameraWidget;
class StrobeWidget;
class LiveViewWidget;

// ---------------------------------------------------------------------------
// MainWindow
//
// Top-level window.  Owns:
//   - CameraController  (plain C++, created on main thread, used on worker)
//   - CameraWorker      (QObject, moved to m_workerThread)
//   - m_workerThread    (QThread)
//   - CameraWidget, StrobeWidget, LiveViewWidget (all on GUI thread)
//
// Layout:
//   +--------------------------------------+
//   | LiveViewWidget (centre, expanding)   |
//   +------------------+-------------------+
//   | CameraWidget     | StrobeWidget      |
//   | (scroll area)    | (scroll area)     |
//   +------------------+-------------------+
//   | Status bar                           |
//   +--------------------------------------+
// ---------------------------------------------------------------------------
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

protected:
    void closeEvent(QCloseEvent* event) override;

private slots:
    void onCameraOpened();
    void onCameraClosed();
    void onAcquisitionStarted();
    void onAcquisitionStopped();
    void onWorkerError(const QString& message);

private:
    void buildUI();
    void connectSignals();
    void shutdownWorkerThread();

    // Owned objects
    std::unique_ptr<CameraController> m_controller;
    CameraWorker*  m_worker       = nullptr;
    QThread*       m_workerThread = nullptr;

    // Widgets
    LiveViewWidget* m_liveView     = nullptr;
    CameraWidget*   m_cameraWidget = nullptr;
    StrobeWidget*   m_strobeWidget = nullptr;
};
#endif // MAINWINDOW_H
