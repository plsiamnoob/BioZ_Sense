#include "mainwindow.h"
#include <QDebug>
#include <iostream>
#include <cmath>
#include <algorithm>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    
    serialPort = new QSerialPort(this);
    timer.start();
    // Board Selector UI
    portComboBox = new QComboBox(this);
    refreshButton = new QPushButton("Refresh Ports", this);

    // --- SWEEP UI SETUP ---
    sweepModeBox = new QComboBox(this);
    sweepModeBox->addItems({"One-Shot", "Linear Sweep", "Logarithmic Sweep"});
    
    sweepParamsWidget = new QStackedWidget(this);

    // Page 0: One-Shot
    QWidget *oneShotWidget = new QWidget();
    QHBoxLayout *oneShotLayout = new QHBoxLayout(oneShotWidget);
    freqInput = new QDoubleSpinBox(); freqInput->setRange(1, 200000); freqInput->setValue(100000);
    oneShotLayout->addWidget(new QLabel("Freq (Hz):")); oneShotLayout->addWidget(freqInput);
    oneShotLayout->setContentsMargins(0,0,0,0);

    // Page 1: Linear Sweep
    QWidget *linWidget = new QWidget();
    QHBoxLayout *linLayout = new QHBoxLayout(linWidget);
    linStart = new QDoubleSpinBox(); linStart->setRange(1, 200000); linStart->setValue(1000);
    linMaxLabel = new QLabel(); // Replaces linEnd // Max boundary
    linStep = new QDoubleSpinBox(); linStep->setRange(0.1, 200000); linStep->setValue(100);

    linSamples = new QSpinBox(); 
    linSamples->setRange(2, 2000); 
    linSamples->setValue(100);


    linLayout->addWidget(new QLabel("Start:")); linLayout->addWidget(linStart);
    linLayout->addWidget(linStep);
    linLayout->addWidget(new QLabel("Step:")); linLayout->addWidget(linStep);
    linLayout->addWidget(new QLabel("Samples:")); 
    linLayout->addWidget(linSamples);
        linLayout->addWidget(linMaxLabel); // Add it to the layout
    linLayout->setContentsMargins(0,0,0,0);

    connect(linStart, &QDoubleSpinBox::valueChanged, this, &MainWindow::updateLinearMaxLabel);
    connect(linStep, &QDoubleSpinBox::valueChanged, this, &MainWindow::updateLinearMaxLabel);
    updateLinearMaxLabel(); // Initialize text
    // Page 2: Logarithmic Sweep
    QWidget *logWidget = new QWidget();
    QHBoxLayout *logLayout = new QHBoxLayout(logWidget);
    logStart = new QDoubleSpinBox(); logStart->setRange(1, 200000); logStart->setValue(1000);
    logEnd = new QDoubleSpinBox(); logEnd->setRange(1, 200000); logEnd->setValue(100000);
    logSamples = new QSpinBox(); logSamples->setRange(2, 2000); logSamples->setValue(100);
    logLayout->addWidget(new QLabel("Start:")); logLayout->addWidget(logStart);
    logLayout->addWidget(new QLabel("End:")); logLayout->addWidget(logEnd);
    logLayout->addWidget(new QLabel("Samples:")); logLayout->addWidget(logSamples);
    logLayout->setContentsMargins(0,0,0,0);

    sweepParamsWidget->addWidget(oneShotWidget);
    sweepParamsWidget->addWidget(linWidget);
    sweepParamsWidget->addWidget(logWidget);

    sendButton = new QPushButton("Start Sweep", this);
    clearButton = new QPushButton("Clear Graphs", this); // <-- ADD THIS

    // ... down in the connections section ...


    // Plotting setup
    impSeries = new QScatterSeries();
    phaseSeries = new QScatterSeries();
connect(impSeries, &QScatterSeries::hovered, this, [this](const QPointF &point, bool state) {
    if (state) {
        // Find point index in the series
        int index = impSeries->points().indexOf(point);

        if (index >= 0 && index < measurementsDisplayed.size()) {
            const Measurement &m = measurementsDisplayed.at(index);

            QString text = QString("Time: %1 s\n"
                                   "Impedance: %2 Ω\n"
                                   "Phase: %3°\n"
                                   "Freq: %4 Hz")
                               .arg(m.timestamp, 0, 'f', 3)
                               .arg(m.impedance, 0, 'f', 2)
                               .arg(m.phase, 0, 'f', 2)
                               .arg(m.frequency, 0, 'f', 1);

            QToolTip::showText(QCursor::pos(), text);
        }
    } else {
        QToolTip::hideText();
    }
});
connect(phaseSeries, &QScatterSeries::hovered, this, [this](const QPointF &point, bool state) {
    if (state) {
        // Find point index in the series
        int index = phaseSeries->points().indexOf(point);

        if (index >= 0 && index < measurementsDisplayed.size()) {
            const Measurement &m = measurementsDisplayed.at(index);

            QString text = QString("Time: %1 s\n"
                                   "Impedance: %2 Ω\n"
                                   "Phase: %3°\n"
                                   "Freq: %4 Hz")
                               .arg(m.timestamp, 0, 'f', 3)
                               .arg(m.impedance, 0, 'f', 2)
                               .arg(m.phase, 0, 'f', 2)
                               .arg(m.frequency, 0, 'f', 1);

            QToolTip::showText(QCursor::pos(), text);
        }
    } else {
        QToolTip::hideText();
    }
});
    impChart = new QChart();
    phaseChart = new QChart();
    impChart->addSeries(impSeries);
        phaseChart->addSeries(phaseSeries);



    impChart->createDefaultAxes();
    phaseChart->createDefaultAxes();
    
    impChart->setTitle("Impedance vs. Time");
    phaseChart->setTitle("Phase vs. Time");



    QChartView *impView = new QChartView(impChart);
    QChartView *phaseView = new QChartView(phaseChart);

    // Layout
    QWidget *centralWidget = new QWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);

    QHBoxLayout *portLayout = new QHBoxLayout();
    portLayout->addWidget(new QLabel("Select Board:"));
    portLayout->addWidget(portComboBox, 1);
    portLayout->addWidget(refreshButton);

    QHBoxLayout *inputLayout = new QHBoxLayout();
    inputLayout->addWidget(sweepModeBox);
    inputLayout->addWidget(sweepParamsWidget, 1); // Expands to fit active widget
    inputLayout->addWidget(sendButton);
    inputLayout->addWidget(clearButton); // <-- ADD THIS

    mainLayout->addLayout(portLayout);
    mainLayout->addLayout(inputLayout);
    mainLayout->addWidget(impView);
    mainLayout->addWidget(phaseView);
    setCentralWidget(centralWidget);

    // Connections
    connect(sweepModeBox, QOverload<int>::of(&QComboBox::currentIndexChanged), sweepParamsWidget, &QStackedWidget::setCurrentIndex);
    connect(refreshButton, &QPushButton::clicked, this, &MainWindow::populateSerialPorts);
    connect(portComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::onPortSelected);
    connect(serialPort, &QSerialPort::readyRead, this, &MainWindow::readData);
    connect(clearButton, &QPushButton::clicked, this, &MainWindow::clearGraphs);
    connect(sendButton, &QPushButton::clicked, this, &MainWindow::onStartSweepClicked); // Connect to new slot

    serialPollTimer = new QTimer(this);
    connect(serialPollTimer, &QTimer::timeout, this, &MainWindow::readData);
    serialPollTimer->start(10); // Poll every 10 milliseconds

    populateSerialPorts();
}

void MainWindow::populateSerialPorts() {
    portComboBox->blockSignals(true);
    portComboBox->clear();

    // Define your specific board's USB Vendor ID (Set to 0 if not filtering)
    // Example: 0x0483 (STM32), 0x2341 (Arduino), 0x0403 (FTDI)
    const quint16 TARGET_VID = 0x0483; 

    const auto ports = QSerialPortInfo::availablePorts();
    for (const QSerialPortInfo &port : ports) {
        
        // 1. Extract Hardware Identifiers
        quint16 vid = port.hasVendorIdentifier() ? port.vendorIdentifier() : 0;
        quint16 pid = port.hasProductIdentifier() ? port.productIdentifier() : 0;
        QString mfg = port.manufacturer();
        QString desc = port.description();

        // 2. OPTIONAL FILTERING: Skip ports without USB VIDs (filters out Bluetooth/Legacy COM ports)
        if (!port.hasVendorIdentifier()) {
            continue; 
        }

        // 3. OPTIONAL FILTERING: Target specific board by Vendor ID
        // if (vid != TARGET_VID) {
        //     continue; 
        // }

        // 4. Build Detailed Label for Dropdown
        // Output format: "COM3 - STM32 Virtual COM Port [VID:0483 PID:5740]"
        QString label = port.portName();
        
        if (!desc.isEmpty()) {
            label += " - " + desc;
        } else if (!mfg.isEmpty()) {
            label += " - " + mfg;
        }

        if (port.hasVendorIdentifier()) {
            label += QString(" [VID:%1 PID:%2]")
                        .arg(vid, 4, 16, QChar('0'))
                        .arg(pid, 4, 16, QChar('0')).toUpper();
        }

        // Add to combobox with raw port name ("COM3") stored in hidden UserRole data
        portComboBox->addItem(label, port.portName());
    }

    if (portComboBox->count() == 0) {
        portComboBox->addItem("No compatible boards found", "");
    }

    portComboBox->blockSignals(false);

    if (portComboBox->count() > 0 && !portComboBox->itemData(0).toString().isEmpty()) {
        onPortSelected(0);
    }
}
void MainWindow::updateLinearMaxLabel() {
    double start = linStart->value();
    double step = linStep->value();
    int samples = linSamples->value();
    
    // Final frequency = start + (step * (samples - 1))
    double maxFreq = start + (step * (samples - 1));
    
    linMaxLabel->setText(QString("Max End: %1 Hz").arg(maxFreq, 0, 'f', 1));
}
void MainWindow::onPortSelected(int index) {
    if (index < 0) return;

    QString portName = portComboBox->itemData(index).toString();
    if (portName.isEmpty()) return;

    // Close active connection before switching
    if (serialPort->isOpen()) {
        serialPort->close();
    }

        serialPort->setPortName(portName);
        serialPort->setBaudRate(230400); // Changed from 115200 to match Python
        serialPort->setDataBits(QSerialPort::Data8);
        serialPort->setParity(QSerialPort::NoParity);
        serialPort->setStopBits(QSerialPort::OneStop);
        serialPort->setFlowControl(QSerialPort::NoFlowControl);

    if (serialPort->open(QIODevice::ReadWrite)) {
        serialPort->setDataTerminalReady(true); // Matches pySerial default
        serialPort->setRequestToSend(true);      // Matches pySerial default
        qDebug() << "Successfully connected to" << portName;
    } else {
        qDebug() << "Failed to open port" << portName << ":" << serialPort->errorString();
    }

}

void MainWindow::clearGraphs() {
    // Stop any active sweep
    sweepQueue.clear();
    measurementsPending.clear();
    measurementsDisplayed.clear();
    accumulatedTime = 0;
    // Clear chart series
    impSeries->clear();
    phaseSeries->clear();

    // Reset Impedance Axes
    auto impX = impChart->axes(Qt::Horizontal);
    auto impY = impChart->axes(Qt::Vertical);
    if (!impX.isEmpty() && !impY.isEmpty()) {
        impX.first()->setRange(0, 1.0);
        impY.first()->setRange(0, 1.0);
    }

    // Reset Phase Axes
    auto phaseX = phaseChart->axes(Qt::Horizontal);
    auto phaseY = phaseChart->axes(Qt::Vertical);
    if (!phaseX.isEmpty() && !phaseY.isEmpty()) {
        phaseX.first()->setRange(0, 1.0);
        phaseY.first()->setRange(0, 1.0);
    }

    // Restart the timer so the next sweep begins at T=0
    timer.restart();
}

void MainWindow::stopSweep(){

    qDebug() << "Stopping Sweep.";
    isSweepActive = false;

    // Halt your hardware/timer sweep logic here
    // sweepTimer->stop();

    // Reset button visual state
    sendButton->setText("Start Sweep");
    sendButton->setStyleSheet(""); // Reset to default application style
    sweepQueue.clear(); // Clear any remaining frequencies
}
void MainWindow::onStartSweepClicked()
{
    if (!isSweepActive) {
        startSweep();
    }else{
        stopSweep();
    }


}

void MainWindow::startSweep() {
    if (!serialPort->isOpen()) return;
    
    // Update button visual state
    sendButton->setText("Stop Sweep");
    sendButton->setStyleSheet("QPushButton { background-color: #d32f2f; color: white; }"); // Red highlight for stop

    // FIX FOR INSTANT COMPLETION: Clear stale data before starting!
    serialPort->clear();
    serialPort->readAll(); 
    measurementsPending.clear(); 
    sweepQueue.clear();
    
            // If we interrupted a running sweep, save its time before restarting
    isSweepActive = true;
    lastTime = timer.elapsed();


    int mode = sweepModeBox->currentIndex();

    if (mode == 0) { // One-Shot
        sweepQueue.enqueue(freqInput->value());
    } 
    else if (mode == 1) { // Linear Sweep
        double start = linStart->value();
        double step = linStep->value();
        int samples = linSamples->value();

        // Loop capped at exactly 2000 samples
        for (int i = 0; i < samples; ++i) {
            double f = start + (step * i);
            sweepQueue.enqueue(f);
        }
    } 
    else if (mode == 2) { // Logarithmic Sweep
        double start = logStart->value();
        double end = logEnd->value();
        int samples = logSamples->value();

        for (int i = 0; i < samples; ++i) {
            double f = start * std::pow(end / start, (double)i / (samples - 1));
            sweepQueue.enqueue(f);
        }
    }

    sendNextSweepPoint(); 
}

void MainWindow::sendNextSweepPoint() {
    if (sweepQueue.isEmpty()) {
        qDebug() << "Sweep complete.";
        stopSweep();
        return;
    }
    qint64 currentTime = timer.elapsed();
    
    if (isSweepActive) {
        // Add the difference (in seconds) to our running total
        accumulatedTime += (currentTime - lastTime) / 1000.0;
    }
    
    lastTime = currentTime; // Lock in the time for the next point
    double freq = sweepQueue.dequeue();
    
    Measurement m;
    m.frequency = freq;
    m.timestamp = accumulatedTime;
    measurementsPending.push_back(m);

    QString payload = QString::number(freq) + "\n";
    serialPort->write(payload.toUtf8());
    serialPort->flush();

}
void MainWindow::readData() {
    while (serialPort->canReadLine()) {
        QString data = QString::fromUtf8(serialPort->readLine()).trimmed();
        QStringList parts = data.split(", ");
        
        if (parts.size() == 2 && !measurementsPending.isEmpty()) {
            bool ok1, ok2;
            double impedance = parts[0].toDouble(&ok1);
            double phase = parts[1].toDouble(&ok2);
            
            // Validate that we successfully parsed actual, finite numbers
            if (!ok1 || !ok2 || !std::isfinite(impedance) || !std::isfinite(phase)) {
                qDebug() << "Ignored invalid data from board:" << data;
                
                // Drop this pending measurement so we don't get out of sync
                measurementsPending.removeFirst(); 
                
                // Keep the sweep moving
               
                    sendNextSweepPoint();
                
                continue; // Skip graphing this point
            }
            
            Measurement first = measurementsPending.takeFirst();
            first.impedance = impedance;
            first.phase = phase;
            
            measurementsDisplayed.push_back(first);
            updatePlots();

            
                sendNextSweepPoint();
            
        }
    }
}

void MainWindow::updatePlots() {
    if (measurementsDisplayed.isEmpty()) return;

    // Append latest data point to graphs
    const Measurement &latest = measurementsDisplayed.last();
    impSeries->append(latest.timestamp, latest.impedance);
    phaseSeries->append(latest.timestamp, latest.phase);

    // --- ENFORCE 2000 ITEM LIMIT ---
    if (measurementsDisplayed.size() > 2000) {
        measurementsDisplayed.removeFirst();
        impSeries->removePoints(0, 1);
        phaseSeries->removePoints(0, 1);
    }

    double minTime = measurementsDisplayed.first().timestamp;
    double maxTime = measurementsDisplayed.last().timestamp;

    // 2. Auto-Scale Impedance Chart
    auto impX = impChart->axes(Qt::Horizontal);
    auto impY = impChart->axes(Qt::Vertical);
    if (!impX.isEmpty() && !impY.isEmpty()) {
        // Shift X-axis forward if we start dropping old points
        impX.first()->setRange(std::max(0.0, minTime - 0.5), maxTime + 1.0);

        auto [minIt, maxIt] = std::minmax_element(
            measurementsDisplayed.begin(), measurementsDisplayed.end(),
            [](const Measurement &a, const Measurement &b) { return a.impedance < b.impedance; }
        );

        double minI = minIt->impedance; double maxI = maxIt->impedance;
        double yMargin = (maxI == minI) ? std::abs(maxI) * 0.15 : (maxI - minI) * 0.15;
        if (yMargin == 0) yMargin = 1.0;
        impY.first()->setRange(minI - yMargin, maxI + yMargin);
    }

    // 3. Auto-Scale Phase Chart
    auto phaseX = phaseChart->axes(Qt::Horizontal);
    auto phaseY = phaseChart->axes(Qt::Vertical);
    if (!phaseX.isEmpty() && !phaseY.isEmpty()) {
        phaseX.first()->setRange(std::max(0.0, minTime - 0.5), maxTime + 1.0);

        auto [minIt, maxIt] = std::minmax_element(
            measurementsDisplayed.begin(), measurementsDisplayed.end(),
            [](const Measurement &a, const Measurement &b) { return a.phase < b.phase; }
        );

        double minP = minIt->phase; double maxP = maxIt->phase;
        double yMargin = (maxP == minP) ? std::abs(maxP) * 0.15 : (maxP - minP) * 0.15;
        if (yMargin == 0) yMargin = 1.0;
        phaseY.first()->setRange(minP - yMargin, maxP + yMargin);
    }
}