#pragma once
#include <QMainWindow>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QComboBox>
#include <QPushButton>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QStackedWidget>
#include <QQueue>
#include <QElapsedTimer>
#include <QtCharts/QChartView>
#include <QtCharts/QScatterSeries>
#include <QTimer>
#include <QToolTip>
#include <QCursor>
#include <cmath>
struct Measurement {
    double frequency = 0.0;
    double impedance = 0.0;
    double phase     = 0.0;
    double timestamp = 0.0;
};

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

private slots:
    void populateSerialPorts();
    void onPortSelected(int index);
    void startSweep();          // Replaces sendFrequency
    void readData();
    void sendNextSweepPoint();  // Handles the queue
    void updateLinearMaxLabel(); // New slot
    void clearGraphs(); 
private:
    void updatePlots();
    QTimer *serialPollTimer; // Replaces readyRead

    QSerialPort *serialPort;
    QElapsedTimer timer;
double accumulatedTime = 0.0; 
    qint64 lastTime = 0;         // <-- Use qint64 for exact timer matching
    bool isSweepActive = false;  // <-- Simple flag to control the clock
    // ...

    // Data Containers
    QList<Measurement> measurementsPending;
    QList<Measurement> measurementsDisplayed;
    QQueue<double> sweepQueue; // Holds generated frequencies to be sent

    // UI Elements
    QComboBox *portComboBox;
    QPushButton *refreshButton;
    QPushButton *sendButton;
QPushButton *clearButton; 
    // Sweep Configuration UI
    QComboBox *sweepModeBox;
    QStackedWidget *sweepParamsWidget;
    
    QDoubleSpinBox *freqInput;       // One-Shot
    QDoubleSpinBox *linStart, *linStep; // Linear
    QDoubleSpinBox *logStart, *logEnd; QSpinBox *logSamples; // Logarithmic
    QSpinBox *linSamples; // Added back!
    QLabel *linMaxLabel;
    // Charts
    QScatterSeries *impSeries;
    QScatterSeries *phaseSeries;
    QChart *impChart;
    QChart *phaseChart;
};