#include "MainWindow.h"

#include "CameraController.h"
#include "CameraWorker.h"
#include "CameraWidget.h"
#include "StrobeWidget.h"
#include "LiveViewWidget.h"

#include <QThread>
#include <QScrollArea>
#include <QSplitter>
#include <QStatusBar>
#include <QMessageBox>
#include <QCloseEvent>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QWidget>

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , m_controller(std::make_unique<CameraController>())
{
    setWindowTitle("StrobeCam — UI-3370CP-NIR-GL");
    resize(1280, 800);

    // buildUI() creates the widgets — must happen before signal wiring.
    buildUI();

    // -------------------------------------------------------------------
    // Worker thread setup
    // -------------------------------------------------------------------
    m_workerThread = new QThread(this);

    // CameraController is owned by MainWindow but all calls to it happen
    // exclusively on the worker thread after it is started.
    m_worker = new CameraWorker(m_controller.get());
    m_worker->moveToThread(m_workerThread);

    // Worker has no parent (required for moveToThread), so connect
    // deleteLater to ensure it is cleaned up when the thread finishes.
    connect(m_workerThread, &QThread::finished,
            m_worker,       &QObject::deleteLater);

    // -------------------------------------------------------------------
    // Worker → GUI  (queued automatically — cross-thread signals)
    // -------------------------------------------------------------------
    connect(m_worker, &CameraWorker::cameraOpened,
            this,     &MainWindow::onCameraOpened);

    connect(m_worker, &CameraWorker::cameraClosed,
            this,     &MainWindow::onCameraClosed);

    connect(m_worker, &CameraWorker::acquisitionStarted,
            this,     &MainWindow::onAcquisitionStarted);

    connect(m_worker, &CameraWorker::acquisitionStopped,
            this,     &MainWindow::onAcquisitionStopped);

    connect(m_worker, &CameraWorker::error,
            this,     &MainWindow::onWorkerError);

    // Frame delivery — QImage is implicitly shared so the queued copy is cheap.
    connect(m_worker,   &CameraWorker::frameReady,
            m_liveView, &LiveViewWidget::updateFrame);

    // -------------------------------------------------------------------
    // GUI → Worker  (explicit QueuedConnection — GUI thread → worker thread)
    // -------------------------------------------------------------------
    connect(m_strobeWidget, &StrobeWidget::acquisitionStartRequested,
            m_worker,       &CameraWorker::startAcquisition,
            Qt::QueuedConnection);

    connect(m_strobeWidget, &StrobeWidget::acquisitionStopRequested,
            m_worker,       &CameraWorker::stopAcquisition,
            Qt::QueuedConnection);

    // -------------------------------------------------------------------
    // Start thread and open camera
    // -------------------------------------------------------------------
    m_workerThread->start();
    QMetaObject::invokeMethod(m_worker, "openCamera", Qt::QueuedConnection);

    statusBar()->showMessage("Opening camera…");
}

MainWindow::~MainWindow()
{
    shutdownWorkerThread();
}

// ---------------------------------------------------------------------------
// UI layout
// ---------------------------------------------------------------------------

void MainWindow::buildUI()
{
    auto* central    = new QWidget(this);
    auto* mainLayout = new QVBoxLayout(central);
    mainLayout->setContentsMargins(4, 4, 4, 4);
    mainLayout->setSpacing(4);
    setCentralWidget(central);

    // Horizontal splitter: live view on the left, controls on the right.
    auto* mainSplitter = new QSplitter(Qt::Horizontal, this);

    // Live view fills the left side.
    m_liveView = new LiveViewWidget(this);

    // Controls stacked vertically on the right, in a single scroll area.
    m_cameraWidget = new CameraWidget(m_controller.get(), this);
    m_strobeWidget = new StrobeWidget(m_controller.get(), this);

    auto* controlContainer = new QWidget(this);
    auto* controlLayout    = new QVBoxLayout(controlContainer);
    controlLayout->setContentsMargins(0, 0, 0, 0);
    controlLayout->setSpacing(4);
    controlLayout->addWidget(m_cameraWidget);
    controlLayout->addWidget(m_strobeWidget);
    controlLayout->addStretch();

    auto* controlScroll = new QScrollArea(this);
    controlScroll->setWidget(controlContainer);
    controlScroll->setWidgetResizable(true);
    controlScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    controlScroll->setFixedWidth(300);

    mainSplitter->addWidget(m_liveView);
    mainSplitter->addWidget(controlScroll);
    mainSplitter->setStretchFactor(0, 1);  // live view stretches
    mainSplitter->setStretchFactor(1, 0);  // controls panel stays fixed

    mainLayout->addWidget(mainSplitter);

    m_cameraWidget->setControlsEnabled(false);
    m_strobeWidget->setParameterControlsEnabled(false);
}

// ---------------------------------------------------------------------------
// Slots — all execute on the GUI thread
// ---------------------------------------------------------------------------

void MainWindow::onCameraOpened()
{
    statusBar()->showMessage("Camera opened.");
    m_cameraWidget->populate();
    m_strobeWidget->populate();
    m_cameraWidget->setControlsEnabled(true);
    m_strobeWidget->setParameterControlsEnabled(true);
    m_liveView->showPlaceholder("Camera ready — click \"Start Acquisition\"");
}

void MainWindow::onCameraClosed()
{
    statusBar()->showMessage("Camera closed.");
    m_cameraWidget->setControlsEnabled(false);
    m_strobeWidget->setParameterControlsEnabled(false);
    m_liveView->showPlaceholder("No camera connected");
}

void MainWindow::onAcquisitionStarted()
{
    m_strobeWidget->onAcquisitionStarted();
    // Disable camera parameter changes during acquisition — ROI, decimation,
    // and pixel format cannot be changed while the data stream is running.
    m_cameraWidget->setControlsEnabled(false);
    statusBar()->showMessage("Acquiring…");
}

void MainWindow::onAcquisitionStopped()
{
    m_strobeWidget->onAcquisitionStopped();
    m_cameraWidget->setControlsEnabled(true);
    statusBar()->showMessage("Acquisition stopped.");
    m_liveView->showPlaceholder("Camera ready — click \"Start Acquisition\"");
}

void MainWindow::onWorkerError(const QString& message)
{
    statusBar()->showMessage("Error: " + message);
    QMessageBox::warning(this, "Camera Error", message);
}

// ---------------------------------------------------------------------------
// Shutdown
// ---------------------------------------------------------------------------

void MainWindow::closeEvent(QCloseEvent* event)
{
    shutdownWorkerThread();
    event->accept();
}

void MainWindow::shutdownWorkerThread()
{
    if (!m_workerThread || !m_workerThread->isRunning())
        return;

    // Stop acquisition and close camera before quitting the thread.
    // BlockingQueuedConnection means we wait for each call to return.
    QMetaObject::invokeMethod(m_worker, "stopAcquisition", Qt::BlockingQueuedConnection);
    QMetaObject::invokeMethod(m_worker, "closeCamera",     Qt::BlockingQueuedConnection);

    m_workerThread->quit();
    if (!m_workerThread->wait(5000)) {
        // Safety net — should not be reached in normal operation.
        m_workerThread->terminate();
        m_workerThread->wait();
    }
    m_workerThread = nullptr;
}
