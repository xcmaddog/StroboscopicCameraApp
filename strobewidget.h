#ifndef STROBEWIDGET_H
#define STROBEWIDGET_H

#include <QWidget>

#pragma once

#include <QWidget>

class QComboBox;
class QSpinBox;
class QDoubleSpinBox;
class QPushButton;
class QLabel;
class CameraController;

// ---------------------------------------------------------------------------
// StrobeWidget
//
// Controls the stroboscopic imaging configuration:
//
//   Trigger section
//     - Trigger source (Line0 = optocoupled input, Line2/3 = GPIO)
//     - Trigger divider (capture every Nth pulse)
//     - Trigger delay   (µs from trigger edge to exposure start)
//
//   Flash / strobe section
//     - Flash reference ("ExposureStart" recommended for stroboscopy)
//     - Flash start delay (µs from exposure start to strobe on)
//     - Flash duration    (µs — only editable when ref = ExposureStart)
//
//   Acquisition control
//     - Start / Stop button with a coloured status indicator
//
// Call populate() after the camera opens to fill combo boxes and read
// initial values.
// ---------------------------------------------------------------------------
class StrobeWidget : public QWidget
{
    Q_OBJECT

public:
    explicit StrobeWidget(CameraController* controller, QWidget* parent = nullptr);

    /// Populate controls from current camera state.
    void populate();

    /// Enable/disable the parameter controls (not the start/stop button).
    void setParameterControlsEnabled(bool enabled);

signals:
    /// Emitted when the user clicks Start.
    void acquisitionStartRequested();

    /// Emitted when the user clicks Stop.
    void acquisitionStopRequested();

public slots:
    /// Called by MainWindow when acquisition actually starts.
    void onAcquisitionStarted();

    /// Called by MainWindow when acquisition actually stops.
    void onAcquisitionStopped();

private slots:
    void onTriggerSourceChanged(int index);
    void onTriggerDividerChanged(int value);
    void onTriggerDelayChanged(double value);
    void onFlashReferenceChanged(int index);
    void onFlashStartDelayChanged(double value);
    void onFlashDurationChanged(double value);
    void onStartStopClicked();

private:
    void buildUI();
    void updateFlashDurationState();

    CameraController* m_controller;
    bool m_acquiring   = false;
    bool m_populating  = false;

    // Trigger
    QComboBox*      m_triggerSourceCombo   = nullptr;
    QSpinBox*       m_triggerDividerSpin   = nullptr;
    QDoubleSpinBox* m_triggerDelaySpin     = nullptr;

    // Flash
    QComboBox*      m_flashRefCombo        = nullptr;
    QDoubleSpinBox* m_flashStartDelaySpin  = nullptr;
    QDoubleSpinBox* m_flashDurationSpin    = nullptr;

    // Acquisition control
    QPushButton*    m_startStopBtn         = nullptr;
    QLabel*         m_statusIndicator      = nullptr;
};

#endif // STROBEWIDGET_H
