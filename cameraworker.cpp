#include "CameraWorker.h"
#include "CameraController.h"
#include <QCoreApplication>

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

    while (m_running) {
        // Process any pending slot calls (e.g. stopAcquisition()) before
        // blocking on the next frame.
        QCoreApplication::processEvents();

        QImage frame = m_controller->acquireFrame(200);  // shorter timeout so we loop faster
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
