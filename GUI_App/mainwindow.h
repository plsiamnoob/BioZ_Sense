#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSerialPort>
#include <QDoubleSpinBox>
#include <QPushButton>
#include <QtCharts/QChartView>
#include <QtCharts/QScatterSeries>
#include <QtCharts/QChart>
#include <QLabel>
#include <QList>

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);

private slots:
    void sendFrequency();
    void readData();
    void updatePlots();
    void findAndConnectPort();

private:
    QSerialPort *serialPort;
    QDoubleSpinBox *freqInput;
    QPushButton *sendButton;
    
    QScatterSeries *impSeries;
    QScatterSeries *phaseSeries;
    QChart *impChart;
    QChart *phaseChart;

    double currentFreq = 0.0;
    QList<double> frequencies;
    QList<double> impedances;
    QList<double> phases;
};

#endif // MAINWINDOW_H