#include "mainwindow.h"
#include <QDebug>
#include <iostream>
#include <cmath>
#include <algorithm>
#include <QToolTip>
#include <fstream>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    
    serialPort = new QSerialPort(this);
    timer.start();

    // Board Selector UI
    portComboBox = new QComboBox(this);
    refreshButton = new QPushButton("Refresh Ports", this);

    // Console
    consoleOutput = new QPlainTextEdit(this);
    consoleOutput->setReadOnly(true);
    consoleOutput->setMaximumBlockCount(0); 
    consoleOutput->setFixedHeight(100);        

    // --- SWEEP UI SETUP ---
    sweepModeBox = new QComboBox(this);
    sweepModeBox->addItems({"Single Freq", "Linear Sweep", "Logarithmic Sweep"});
    
    sweepParamsWidget = new QStackedWidget(this);

    // Page 0: Single Frequency
    QWidget *singleFreqWidget = new QWidget();
    QHBoxLayout *singleFreqLayout = new QHBoxLayout(singleFreqWidget);
    
    freqInput = new QDoubleSpinBox(); 
    freqInput->setRange(1, 250000); 
    freqInput->setValue(100000);

    singlePointsBox = new QSpinBox(); 
    singlePointsBox->setRange(1, 100000); 
    singlePointsBox->setValue(500);
    
    singleFreqLayout->addWidget(new QLabel("Freq (Hz):")); 
    singleFreqLayout->addWidget(freqInput);
    singleFreqLayout->addWidget(new QLabel("Samples:")); 
    singleFreqLayout->addWidget(singlePointsBox);
    singleFreqLayout->setContentsMargins(0,0,0,0);

    // Page 1: Linear Sweep
    QWidget *linWidget = new QWidget();
    QHBoxLayout *linLayout = new QHBoxLayout(linWidget);
    linStart = new QDoubleSpinBox(); linStart->setRange(1, 50000); linStart->setValue(1000);
    linMaxLabel = new QLabel(); 
    linStep = new QDoubleSpinBox(); linStep->setRange(0.1, 200000); linStep->setValue(100);

    linSamples = new QSpinBox(); 
    linSamples->setRange(2, 2000); 
    linSamples->setValue(100);

    linLayout->addWidget(new QLabel("Start:")); linLayout->addWidget(linStart);
    linLayout->addWidget(new QLabel("Step:")); linLayout->addWidget(linStep);
    linLayout->addWidget(new QLabel("Samples:")); linLayout->addWidget(linSamples);
    linLayout->addWidget(linMaxLabel); 
    linLayout->setContentsMargins(0,0,0,0);

    connect(linStart, &QDoubleSpinBox::valueChanged, this, &MainWindow::updateLinearMaxLabel);
    connect(linStep, &QDoubleSpinBox::valueChanged, this, &MainWindow::updateLinearMaxLabel);
    connect(linSamples, QOverload<int>::of(&QSpinBox::valueChanged), this, &MainWindow::updateLinearMaxLabel);
    updateLinearMaxLabel();

    // Page 2: Logarithmic Sweep
    QWidget *logWidget = new QWidget();
    QHBoxLayout *logLayout = new QHBoxLayout(logWidget);
    logStart = new QDoubleSpinBox(); logStart->setRange(1, 50000); logStart->setValue(5000);
    logEnd = new QDoubleSpinBox(); logEnd->setRange(50000, 250000); logEnd->setValue(250000);
    logSamples = new QSpinBox(); logSamples->setRange(2, 2000); logSamples->setValue(100);
    
    logLayout->addWidget(new QLabel("Start:")); logLayout->addWidget(logStart);
    logLayout->addWidget(new QLabel("End:")); logLayout->addWidget(logEnd);
    logLayout->addWidget(new QLabel("Samples:")); logLayout->addWidget(logSamples);
    logLayout->setContentsMargins(0,0,0,0);

    // Stacked Widget Pages
    sweepParamsWidget->addWidget(singleFreqWidget);
    sweepParamsWidget->addWidget(linWidget);
    sweepParamsWidget->addWidget(logWidget);

    sendButton = new QPushButton("Start Sweep", this);
    clearButton = new QPushButton("Clear Graphs", this);

    // Plotting Setup
    impSeries = new QScatterSeries();
    phaseSeries = new QScatterSeries();

    auto setupHoverTooltip = [this](QScatterSeries *series) {
        connect(series, &QScatterSeries::hovered, this, [this](const QPointF &point, bool state) {
            if (state) {
                int index = impSeries->points().indexOf(point);
                if (index < 0) index = phaseSeries->points().indexOf(point);

                if (index >= 0 && index < measurementsDisplayed.size()) {
                    const Measurement &m = measurementsDisplayed.at(index);
                    QString text = QString("Time: %1 s\nImpedance: %2 Ω\nPhase: %3°\nFreq: %4 Hz")
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
    };

    setupHoverTooltip(impSeries);
    setupHoverTooltip(phaseSeries);

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

    // Main Layout Assembly
    QWidget *centralWidget = new QWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);

    QHBoxLayout *portLayout = new QHBoxLayout();
    portLayout->addWidget(new QLabel("Select Board:"));
    portLayout->addWidget(portComboBox, 1);
    portLayout->addWidget(refreshButton);

    QHBoxLayout *inputLayout = new QHBoxLayout();
    inputLayout->addWidget(sweepModeBox);
    inputLayout->addWidget(sweepParamsWidget, 1);
    inputLayout->addWidget(sendButton);
    inputLayout->addWidget(clearButton);

    mainLayout->addLayout(portLayout);
    mainLayout->addLayout(inputLayout);
    mainLayout->addWidget(impView);
    mainLayout->addWidget(phaseView);
    mainLayout->addWidget(consoleOutput);
    
    setCentralWidget(centralWidget);

    // Signal/Slot Connections
    connect(sweepModeBox, QOverload<int>::of(&QComboBox::currentIndexChanged), sweepParamsWidget, &QStackedWidget::setCurrentIndex);
    connect(refreshButton, &QPushButton::clicked, this, &MainWindow::populateSerialPorts);
    connect(portComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::onPortSelected);
    connect(clearButton, &QPushButton::clicked, this, &MainWindow::clearGraphs);
    connect(sendButton, &QPushButton::clicked, this, &MainWindow::onStartSweepClicked);

    serialPollTimer = new QTimer(this);
    connect(serialPollTimer, &QTimer::timeout, this, &MainWindow::readData);
    serialPollTimer->start(10);

    populateSerialPorts();
}

void MainWindow::populateSerialPorts() {
    portComboBox->blockSignals(true);
    portComboBox->clear();

    const auto ports = QSerialPortInfo::availablePorts();
    for (const QSerialPortInfo &port : ports) {
        if (!port.hasVendorIdentifier()) continue; 

        quint16 vid = port.vendorIdentifier();
        quint16 pid = port.productIdentifier();
        QString mfg = port.manufacturer();
        QString desc = port.description();

        QString label = port.portName();
        if (!desc.isEmpty()) label += " - " + desc;
        else if (!mfg.isEmpty()) label += " - " + mfg;

        label += QString(" [VID:%1 PID:%2]")
                    .arg(vid, 4, 16, QChar('0'))
                    .arg(pid, 4, 16, QChar('0')).toUpper();

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
    double maxFreq = start + (step * (samples - 1));
    
    linMaxLabel->setText(QString("Max End: %1 Hz").arg(maxFreq, 0, 'f', 1));
}

void MainWindow::onPortSelected(int index) {
    if (index < 0) return;

    QString portName = portComboBox->itemData(index).toString();
    if (portName.isEmpty()) return;

    if (serialPort->isOpen()) {
        serialPort->close();
    }

    serialPort->setPortName(portName);
    serialPort->setBaudRate(230400); 
    serialPort->setDataBits(QSerialPort::Data8);
    serialPort->setParity(QSerialPort::NoParity);
    serialPort->setStopBits(QSerialPort::OneStop);
    serialPort->setFlowControl(QSerialPort::NoFlowControl);

    if (serialPort->open(QIODevice::ReadWrite)) {
        serialPort->setDataTerminalReady(true);
        serialPort->setRequestToSend(true);
        qDebug() << "Successfully connected to" << portName;
    } else {
        qDebug() << "Failed to open port" << portName << ":" << serialPort->errorString();
    }
}

void MainWindow::clearGraphs() {
    sweepQueue.clear();
    measurementsPending.clear();
    measurementsDisplayed.clear();
    accumulatedTime = 0;

    impSeries->clear();
    phaseSeries->clear();

    auto impX = impChart->axes(Qt::Horizontal);
    auto impY = impChart->axes(Qt::Vertical);
    if (!impX.isEmpty() && !impY.isEmpty()) {
        impX.first()->setRange(0, 1.0);
        impY.first()->setRange(0, 1.0);
    }

    auto phaseX = phaseChart->axes(Qt::Horizontal);
    auto phaseY = phaseChart->axes(Qt::Vertical);
    if (!phaseX.isEmpty() && !phaseY.isEmpty()) {
        phaseX.first()->setRange(0, 1.0);
        phaseY.first()->setRange(0, 1.0);
    }

    timer.restart();
}

void MainWindow::stopSweep() {
    qDebug() << "Stopping Sweep.";
    isSweepActive = false;

    if (serialPort && serialPort->isOpen()) {
        serialPort->write("STOP\n"); // Interrupt command for C firmware
        serialPort->flush();
    }

    sendButton->setText("Start Sweep");
    sendButton->setStyleSheet("");
    measurementsPending.clear();
    sweepQueue.clear();
}

void MainWindow::onStartSweepClicked() {
    if (!isSweepActive) {
        startSweep();
    } else {
        stopSweep();
    }
}

void MainWindow::startSweep() {
    if (!serialPort->isOpen()) return;

    sendButton->setText("Stop Sweep");
    sendButton->setStyleSheet("QPushButton { background-color: #d32f2f; color: white; }");

    serialPort->clear();
    serialPort->readAll(); 
    measurementsPending.clear(); 
    measurementsDisplayed.clear();
    impSeries->clear();
    phaseSeries->clear();

    std::fstream file("../../../sweep_data.csv", std::ios::out | std::ios::app);

    if(file.is_open()){
        file << "Impedance,Phase\n"; 
        file.close();
    } else {
        qWarning() << "Failed to open sweep_data.csv for writing.";
    }

    isSweepActive = true;
    lastTime = timer.elapsed();
    accumulatedTime = 0.0;

    int mode = sweepModeBox->currentIndex();
    QString payload;

    if (mode == 0) { // Single Frequency
        double freq = freqInput->value();
        int pts = singlePointsBox->value(); // Dynamic value from UI

        for (int i = 0; i < pts; ++i) {
            Measurement m;
            m.frequency = freq;
            measurementsPending.push_back(m);
        }

        impChart->setTitle(QString("Impedance vs. Time (%1 Hz)").arg(freq));
        phaseChart->setTitle(QString("Phase vs. Time (%1 Hz)").arg(freq));

        payload = QString("SINGLE,%1,%2\n").arg(freq, 0, 'f', 1).arg(pts);
    } 
    else if (mode == 1) { // Linear Sweep
        double start = linStart->value();
        double step = linStep->value();
        int pts = linSamples->value();
        double stop = start + (step * (pts - 1));

        for (int i = 0; i < pts; ++i) {
            Measurement m;
            m.frequency = start + (step * i);
            measurementsPending.push_back(m);
        }

        impChart->setTitle("Impedance vs. Frequency (Linear)");
        phaseChart->setTitle("Phase vs. Frequency (Linear)");

        payload = QString("LIN,%1,%2,%3,%4\n")
                    .arg(start, 0, 'f', 1)
                    .arg(stop, 0, 'f', 1)
                    .arg(step, 0, 'f', 1)
                    .arg(pts);
    } 
    else if (mode == 2) { // Logarithmic Sweep
        double start = logStart->value();
        double stop = logEnd->value();
        int pts = logSamples->value();

        for (int i = 0; i < pts; ++i) {
            Measurement m;
            m.frequency = start * std::pow(stop / start, (double)i / (pts - 1));
            measurementsPending.push_back(m);
        }

        impChart->setTitle("Impedance vs. Frequency (Logarithmic)");
        phaseChart->setTitle("Phase vs. Frequency (Logarithmic)");

        payload = QString("LOG,%1,%2,%3,%4\n")
                    .arg(start, 0, 'f', 1)
                    .arg(stop, 0, 'f', 1)
                    .arg(0.0, 0, 'f', 1)
                    .arg(pts);
    }

    serialPort->write(payload.toUtf8());
    serialPort->flush();
}

void MainWindow::readData() {
    while (serialPort->canReadLine()) {
        QString data = QString::fromUtf8(serialPort->readLine()).trimmed();
        if (data.isEmpty()) continue;

        consoleOutput->appendPlainText("Debug: " + data);

        if (data == "SWEEPCOMPLETE" || data == "SWEEPSTOPPED") {
            qDebug() << "Hardware notification:" << data;
            isSweepActive = false;
            sendButton->setText("Start Sweep");
            sendButton->setStyleSheet("");
            measurementsPending.clear();
            continue;
        }
        else if (data.startsWith("ERR ")) {
            qWarning() << "Hardware Error:" << data;
            stopSweep();
            continue;
        }

        if (data.startsWith("DATA ") || data.contains(",")) {
            QString payload = data.startsWith("DATA ") ? data.mid(5).trimmed() : data;
            QStringList parts = payload.split(",");

            if (parts.size() == 2) {
                bool ok1, ok2;
                double impedance = parts[0].trimmed().toDouble(&ok1);
                double phase = parts[1].trimmed().toDouble(&ok2);


                if (ok1 && ok2 && std::isfinite(impedance) && std::isfinite(phase)) {
                    Measurement m;
                    std::fstream file("../../../sweep_data.csv", std::ios::out | std::ios::app);
                    if(file.is_open()){
                        file << impedance << "," << phase << "\n";
                        file.close();
                    }
                    if (!measurementsPending.isEmpty()) {
                        m = measurementsPending.takeFirst();
                    } else {
                        m.frequency = 0;
                    }

                    qint64 currentTime = timer.elapsed();
                    accumulatedTime += (currentTime - lastTime) / 1000.0;
                    lastTime = currentTime;
                    
                    m.timestamp = accumulatedTime;
                    m.impedance = impedance;
                    m.phase = phase;

                    measurementsDisplayed.push_back(m);
                    updatePlots();
                }
            }
        }
    }
}

void MainWindow::updatePlots() {
    if (measurementsDisplayed.isEmpty()) return;

    const Measurement &latest = measurementsDisplayed.last();
    int mode = sweepModeBox->currentIndex();

    double xVal = (mode == 0) ? latest.timestamp : latest.frequency;

    impSeries->append(xVal, latest.impedance);
    phaseSeries->append(xVal, latest.phase);

    if (measurementsDisplayed.size() > 2000) {
        measurementsDisplayed.removeFirst();
        impSeries->removePoints(0, 1);
        phaseSeries->removePoints(0, 1);
    }

    auto [minXIt, maxXIt] = std::minmax_element(
        measurementsDisplayed.begin(), measurementsDisplayed.end(),
        [mode](const Measurement &a, const Measurement &b) {
            return (mode == 0) ? (a.timestamp < b.timestamp) : (a.frequency < b.frequency);
        }
    );

    double minX = (mode == 0) ? minXIt->timestamp : minXIt->frequency;
    double maxX = (mode == 0) ? maxXIt->timestamp : maxXIt->frequency;
    double xPadding = (maxX == minX) ? 1.0 : (maxX - minX) * 0.05;

    auto impX = impChart->axes(Qt::Horizontal);
    auto phaseX = phaseChart->axes(Qt::Horizontal);
    if (!impX.isEmpty()) impX.first()->setRange(minX - xPadding, maxX + xPadding);
    if (!phaseX.isEmpty()) phaseX.first()->setRange(minX - xPadding, maxX + xPadding);

    auto [minIIt, maxIIt] = std::minmax_element(
        measurementsDisplayed.begin(), measurementsDisplayed.end(),
        [](const Measurement &a, const Measurement &b) { return a.impedance < b.impedance; }
    );
    double minI = minIIt->impedance, maxI = maxIIt->impedance;
    double yMarginI = (maxI == minI) ? 10.0 : (maxI - minI) * 0.1;
    
    auto impY = impChart->axes(Qt::Vertical);
    if (!impY.isEmpty()) impY.first()->setRange(minI - yMarginI, maxI + yMarginI);

    auto [minPIt, maxPIt] = std::minmax_element(
        measurementsDisplayed.begin(), measurementsDisplayed.end(),
        [](const Measurement &a, const Measurement &b) { return a.phase < b.phase; }
    );
    double minP = minPIt->phase, maxP = maxPIt->phase;
    double yMarginP = (maxP == minP) ? 5.0 : (maxP - minP) * 0.1;

    auto phaseY = phaseChart->axes(Qt::Vertical);
    if (!phaseY.isEmpty()) phaseY.first()->setRange(minP - yMarginP, maxP + yMarginP);
}