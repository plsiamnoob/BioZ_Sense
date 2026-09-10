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
#include <QPlainTextEdit>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>

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
    void startSweep();
    void readData();
    void updateLinearMaxLabel();
    void clearGraphs(); 
    void onStartSweepClicked();

private:
    void stopSweep();
    void updatePlots();

    QPlainTextEdit *consoleOutput;
    QTimer *serialPollTimer;

    QSerialPort *serialPort;
    QElapsedTimer timer;
    double accumulatedTime = 0.0; 
    qint64 lastTime = 0;
    bool isSweepActive = false;

    // Data Containers
    QList<Measurement> measurementsPending;
    QList<Measurement> measurementsDisplayed;
    QQueue<double> sweepQueue;

    // Main Controls
    QComboBox *portComboBox;
    QPushButton *refreshButton;
    QPushButton *sendButton;
    QPushButton *clearButton; 

    // Sweep Configuration UI
    QComboBox *sweepModeBox;
    QStackedWidget *sweepParamsWidget;
    
    // Single Frequency Mode
    QDoubleSpinBox *freqInput;       // Fixed: Added missing definition
    QSpinBox *singlePointsBox;       // Fixed: Corrected type from QDoubleSpinBox to QSpinBox

    // Linear Sweep Mode
    QDoubleSpinBox *linStart;
    QDoubleSpinBox *linStep;
    QSpinBox *linSamples;
    QLabel *linMaxLabel;

    // Logarithmic Sweep Mode
    QDoubleSpinBox *logStart;
    QDoubleSpinBox *logEnd;
    QSpinBox *logSamples;

    // Charts
    QScatterSeries *impSeries;
    QScatterSeries *phaseSeries;
    QChart *impChart;
    QChart *phaseChart;
};