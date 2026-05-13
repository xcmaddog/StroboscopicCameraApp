#include "StrobeWidget.h"
#include "CameraController.h"

#include <QComboBox>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QPushButton>
#include <QLabel>
#include <QGroupBox>
#include <QFormLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

StrobeWidget::StrobeWidget(CameraController* controller, QWidget* parent)
    : QWidget(parent)
    , m_controller(controller)
{
    buildUI();
}

void StrobeWidget::buildUI()
{
    auto* outerLayout = new QVBoxLayout(this);
    outerLayout->setContentsMargins(0, 0, 0, 0);

    // ---- Trigger --------------------------------------------------------
    {
        auto* grp  = new QGroupBox("Hardware Trigger", this);
        auto* form = new QFormLayout(grp);

        m_triggerSourceCombo = new QComboBox(this);
        // These are placeholder items — populate() replaces them with the
        // list the camera actually reports.
        m_triggerSourceCombo->addItem("Line0 (Trigger in, opto-coupled)", "Line0");
        m_triggerSourceCombo->addItem("Line2 (GPIO 1, 3.3 V)",            "Line2");
        m_triggerSourceCombo->addItem("Line3 (GPIO 2, 3.3 V)",            "Line3");

        m_triggerDividerSpin = new QSpinBox(this);
        m_triggerDividerSpin->setRange(1, 255);
        m_triggerDividerSpin->setValue(1);
        m_triggerDividerSpin->setToolTip(
            "Capture every Nth trigger pulse.\n"
            "Set to 1 to capture on every pulse.");

        m_triggerDelaySpin = new QDoubleSpinBox(this);
        m_triggerDelaySpin->setSuffix(" µs");
        m_triggerDelaySpin->setDecimals(1);
        m_triggerDelaySpin->setRange(0.0, 1000000.0);
        m_triggerDelaySpin->setSingleStep(10.0);
        m_triggerDelaySpin->setToolTip(
            "Delay between the trigger edge and the start of exposure.");

        form->addRow("Trigger source:",  m_triggerSourceCombo);
        form->addRow("Trigger divider:", m_triggerDividerSpin);
        form->addRow("Trigger delay:",   m_triggerDelaySpin);
        outerLayout->addWidget(grp);

        connect(m_triggerSourceCombo,
                QOverload<int>::of(&QComboBox::currentIndexChanged),
                this, &StrobeWidget::onTriggerSourceChanged);
        connect(m_triggerDividerSpin,
                QOverload<int>::of(&QSpinBox::valueChanged),
                this, &StrobeWidget::onTriggerDividerChanged);
        connect(m_triggerDelaySpin,
                QOverload<double>::of(&QDoubleSpinBox::valueChanged),
                this, &StrobeWidget::onTriggerDelayChanged);
    }

    // ---- Flash / strobe -------------------------------------------------
    {
        auto* grp  = new QGroupBox("Flash / Strobe Output  (Line1, pins 2 & 5)", this);
        auto* form = new QFormLayout(grp);

        m_flashRefCombo = new QComboBox(this);
        m_flashRefCombo->addItem("ExposureStart (recommended)", "ExposureStart");
        m_flashRefCombo->addItem("ExposureActive",              "ExposureActive");
        m_flashRefCombo->setToolTip(
            "ExposureStart: strobe timing is set by Start Delay + Duration.\n"
            "ExposureActive: strobe mirrors the exposure window; Duration\n"
            "                is not independently settable on UI models.");

        m_flashStartDelaySpin = new QDoubleSpinBox(this);
        m_flashStartDelaySpin->setSuffix(" µs");
        m_flashStartDelaySpin->setDecimals(1);
        m_flashStartDelaySpin->setRange(0.0, 500000.0);
        m_flashStartDelaySpin->setSingleStep(10.0);
        m_flashStartDelaySpin->setToolTip(
            "Time after exposure start before the strobe output goes high.");

        m_flashDurationSpin = new QDoubleSpinBox(this);
        m_flashDurationSpin->setSuffix(" µs");
        m_flashDurationSpin->setDecimals(1);
        m_flashDurationSpin->setRange(0.0, 500000.0);
        m_flashDurationSpin->setSingleStep(10.0);
        m_flashDurationSpin->setToolTip(
            "Duration the strobe output stays high.\n"
            "Only available when Flash Reference = ExposureStart.");

        form->addRow("Flash reference:",   m_flashRefCombo);
        form->addRow("Flash start delay:", m_flashStartDelaySpin);
        form->addRow("Flash duration:",    m_flashDurationSpin);
        outerLayout->addWidget(grp);

        connect(m_flashRefCombo,
                QOverload<int>::of(&QComboBox::currentIndexChanged),
                this, &StrobeWidget::onFlashReferenceChanged);
        connect(m_flashStartDelaySpin,
                QOverload<double>::of(&QDoubleSpinBox::valueChanged),
                this, &StrobeWidget::onFlashStartDelayChanged);
        connect(m_flashDurationSpin,
                QOverload<double>::of(&QDoubleSpinBox::valueChanged),
                this, &StrobeWidget::onFlashDurationChanged);
    }

    // ---- Acquisition control --------------------------------------------
    {
        auto* grp    = new QGroupBox("Acquisition", this);
        auto* hbox   = new QHBoxLayout(grp);

        m_statusIndicator = new QLabel(this);
        m_statusIndicator->setFixedSize(16, 16);
        m_statusIndicator->setStyleSheet(
            "background-color: #555; border-radius: 8px;");
        m_statusIndicator->setToolTip("Grey = stopped, Green = acquiring");

        m_startStopBtn = new QPushButton("Start Acquisition", this);
        m_startStopBtn->setCheckable(false);
        m_startStopBtn->setMinimumHeight(32);

        hbox->addWidget(m_statusIndicator);
        hbox->addWidget(m_startStopBtn);
        outerLayout->addWidget(grp);

        connect(m_startStopBtn, &QPushButton::clicked,
                this, &StrobeWidget::onStartStopClicked);
    }

    outerLayout->addStretch();

    // Grey out flash duration initially (correct for ExposureStart default).
    updateFlashDurationState();
}

// ---------------------------------------------------------------------------
// populate()
// ---------------------------------------------------------------------------

void StrobeWidget::populate()
{
    m_populating = true;

    // Trigger source — replace placeholder items with camera-reported list.
    m_triggerSourceCombo->clear();
    auto sources = m_controller->availableTriggerSources();
    for (const auto& src : sources) {
        QString label = QString::fromStdString(src);
        // Add human-readable annotations for known lines.
        if (src == "Line0") label += " (Trigger input, opto-coupled)";
        else if (src == "Line2") label += " (GPIO 1, 3.3 V)";
        else if (src == "Line3") label += " (GPIO 2, 3.3 V)";
        m_triggerSourceCombo->addItem(label, QString::fromStdString(src));
    }
    auto curSrc = m_controller->getTriggerSource();
    for (int i = 0; i < m_triggerSourceCombo->count(); ++i) {
        if (m_triggerSourceCombo->itemData(i).toString().toStdString() == curSrc) {
            m_triggerSourceCombo->setCurrentIndex(i);
            break;
        }
    }

    m_triggerDividerSpin->setValue(m_controller->getTriggerDivider());
    m_triggerDelaySpin->setValue(m_controller->getTriggerDelay());

    // Flash start delay and duration.
    m_flashStartDelaySpin->setValue(m_controller->getFlashStartDelay());
    m_flashDurationSpin->setValue(m_controller->getFlashDuration());

    m_populating = false;

    updateFlashDurationState();
}

void StrobeWidget::setParameterControlsEnabled(bool enabled)
{
    m_triggerSourceCombo->setEnabled(enabled);
    m_triggerDividerSpin->setEnabled(enabled);
    m_triggerDelaySpin->setEnabled(enabled);
    m_flashRefCombo->setEnabled(enabled);
    m_flashStartDelaySpin->setEnabled(enabled);
    // Flash duration obeys its own enable rule on top of this.
    if (enabled)
        updateFlashDurationState();
    else
        m_flashDurationSpin->setEnabled(false);
}

// ---------------------------------------------------------------------------
// Public slots — called by MainWindow
// ---------------------------------------------------------------------------

void StrobeWidget::onAcquisitionStarted()
{
    m_acquiring = true;
    m_startStopBtn->setText("Stop Acquisition");
    m_statusIndicator->setStyleSheet(
        "background-color: #22cc44; border-radius: 8px;");
    setParameterControlsEnabled(false);
}

void StrobeWidget::onAcquisitionStopped()
{
    m_acquiring = false;
    m_startStopBtn->setText("Start Acquisition");
    m_statusIndicator->setStyleSheet(
        "background-color: #555; border-radius: 8px;");
    setParameterControlsEnabled(true);
}

// ---------------------------------------------------------------------------
// Private slots
// ---------------------------------------------------------------------------

void StrobeWidget::onStartStopClicked()
{
    if (m_acquiring)
        emit acquisitionStopRequested();
    else
        emit acquisitionStartRequested();
}

void StrobeWidget::onTriggerSourceChanged(int index)
{
    if (m_populating) return;
    auto line = m_triggerSourceCombo->itemData(index).toString().toStdString();
    m_controller->setTriggerSource(line);
}

void StrobeWidget::onTriggerDividerChanged(int value)
{
    if (m_populating) return;
    m_controller->setTriggerDivider(value);
}

void StrobeWidget::onTriggerDelayChanged(double value)
{
    if (m_populating) return;
    m_controller->setTriggerDelay(value);
}

void StrobeWidget::onFlashReferenceChanged(int /*index*/)
{
    if (m_populating) return;
    auto ref = m_flashRefCombo->currentData().toString().toStdString();
    m_controller->setFlashReference(ref);
    updateFlashDurationState();
}

void StrobeWidget::onFlashStartDelayChanged(double value)
{
    if (m_populating) return;
    m_controller->setFlashStartDelay(value);
}

void StrobeWidget::onFlashDurationChanged(double value)
{
    if (m_populating) return;
    m_controller->setFlashDuration(value);
}

void StrobeWidget::updateFlashDurationState()
{
    // FlashDuration is only writable on UI models when
    // FlashReference = "ExposureStart".
    bool isExposureStart =
        (m_flashRefCombo->currentData().toString() == "ExposureStart");
    m_flashDurationSpin->setEnabled(isExposureStart);
    m_flashDurationSpin->setToolTip(isExposureStart
                                        ? "Duration the strobe output stays high."
                                        : "Not available when Flash Reference = ExposureActive.\n"
                                          "Switch to ExposureStart to set duration independently.");
}
