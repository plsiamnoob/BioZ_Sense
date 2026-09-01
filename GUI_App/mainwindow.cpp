#include "mainwindow.h"
#include <QSerialPort>
#include <QSerialPortInfo> // <-- Ensure this is included
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDebug>
#include <algorithm>
#include <iostream>
MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {

    std::cout << "C++ language standard version: " << __cplusplus << std::endl;
    serialPort = new QSerialPort(this);
    
    // UI Elements
    freqInput = new QDoubleSpinBox(this);
    freqInput->setRange(1.0, 200000.0);
    sendButton = new QPushButton("Send Frequency", this);
    
    // Plotting setup
    impSeries = new QScatterSeries();
    phaseSeries = new QScatterSeries();
    impChart = new QChart();
    phaseChart = new QChart();
    
    impChart->addSeries(impSeries);
    phaseChart->addSeries(phaseSeries);
    
    QChartView *impView = new QChartView(impChart);
    QChartView *phaseView = new QChartView(phaseChart);

    // Layout
    QWidget *centralWidget = new QWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);
    QHBoxLayout *inputLayout = new QHBoxLayout();
    
    inputLayout->addWidget(new QLabel("Frequency (Hz):"));
    inputLayout->addWidget(freqInput);
    inputLayout->addWidget(sendButton);
    
    mainLayout->addLayout(inputLayout);
    mainLayout->addWidget(impView);
    mainLayout->addWidget(phaseView);
    setCentralWidget(centralWidget);

    // Connections
    connect(sendButton, &QPushButton::clicked, this, &MainWindow::sendFrequency);
    connect(serialPort, &QSerialPort::readyRead, this, &MainWindow::readData);

    findAndConnectPort();
}

void MainWindow::findAndConnectPort() {
    QList<QSerialPortInfo> ports = QSerialPortInfo::availablePorts();
    for (const QSerialPortInfo &port : ports) {
        if (port.description().contains("CMSIS-DAP") || 
            port.description().contains("DAPLink") || 
            port.description().contains("Serial")) {
            serialPort->setPort(port);
            break;
        }
    }
    
    if (serialPort->portName().isEmpty() && !ports.isEmpty()) {
        serialPort->setPort(ports.first());
    }

    serialPort->setBaudRate(230400);
    if (serialPort->open(QIODevice::ReadWrite)) {
        qDebug() << "Connected to board:" << serialPort->portName();
    }
}

void MainWindow::sendFrequency() {
    if (!serialPort->isOpen()) return;
    
    currentFreq = freqInput->value();
    QString payload = QString::number(currentFreq) + "\n";
    serialPort->write(payload.toUtf8());
}

void MainWindow::readData() {
    while (serialPort->canReadLine()) {
        QString data = QString::fromUtf8(serialPort->readLine()).trimmed();
        QStringList parts = data.split(", ");
        
        if (parts.size() == 2) {
            double impedance = parts[0].toDouble();
            double phase = parts[1].toDouble();
            
            frequencies.append(currentFreq);
            impedances.append(impedance);
            phases.append(phase);
            
            updatePlots();
        }
    }
}

void MainWindow::updatePlots() {
    impSeries->append(currentFreq, impedances.last());
    phaseSeries->append(currentFreq, phases.last());
    
    // IQR Calculation and Axis Scaling
    QList<double> sortedImp = impedances;
    std::sort(sortedImp.begin(), sortedImp.end());
    
    if(sortedImp.size() > 3) {
        double q25 = sortedImp[sortedImp.size() * 0.25];
        double q75 = sortedImp[sortedImp.size() * 0.75];
        double iqr = q75 - q25;
        
        impChart->axes(Qt::Vertical).first()->setRange(q25 - (iqr * 1.5), q75 + (iqr * 1.5));
    }
    
    // Repeat IQR for phase ...
}