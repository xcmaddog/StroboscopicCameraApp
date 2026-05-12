#include "CameraWorker.h"
#include "CameraController.h"

CameraWorker::CameraWorker(CameraController* controller, QObject* parent)
    : QObject(parent)
    , m_controller(controller)
{}

CameraWorker::~CameraWorker()
{
    stopAcquisition();
}

void CameraWorker::openCamera()
{
    if (!m_controller->open()) {
        emit error(QString::fromStdString(m_controller->errorMessage()));
        return;
    }
    emit cameraOpened();
}

void CameraWorker::closeCamera()
{
    if (m_running)
        stopAcquisition();
    m_controller->close();
    emit cameraClosed();
}

void CameraWorker::startAcquisition()
{
    if (m_running)
        return;

    m_controller->startAcquisition();
    if (!m_controller->isAcquiring()) {
        emit error(QString::fromStdString(m_controller->errorMessage()));
        return;
    }

    m_running = true;
    emit acquisitionStarted();

    // Tight acquisition loop — runs on the worker thread.
    // acquireFrame() blocks up to 2 s waiting for a buffer; if none
    // arrives it returns a null QImage and we loop again, which lets
    // m_running checks remain responsive.
    while (m_running) {
        QImage frame = m_controller->acquireFrame(2000);
        if (!frame.isNull())
            emit frameReady(std::move(frame));
        else if (!m_controller->errorMessage().empty())
            emit error(QString::fromStdString(m_controller->errorMessage()));
    }

    m_controller->stopAcquisition();
    emit acquisitionStopped();
}

void CameraWorker::stopAcquisition()
{
    m_running = false;
}