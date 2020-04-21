#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtSerialPort/QSerialPort>
#include <QtSerialPort/QSerialPortInfo>
#include <QListWidgetItem>
#include <QThread>
#include <QMessageBox>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void PopulateSerialPorts ();

private slots:
    void on_program_selector_currentIndexChanged(int index);

    void on_com_selector_currentIndexChanged(int index);

    void on_baud_selector_currentIndexChanged(int index);

    void on_function_selector_currentIndexChanged(int index);

    void on_button_connect_serial_clicked();

    void on_button_write_program_clicked();

    void on_button_add_command_clicked();

    void on_program_list_itemDoubleClicked(QListWidgetItem *item);

    void on_program_list_itemSelectionChanged();

    void on_button_add_command_above_clicked();

    void on_com_selector_activated(int index);

    void on_button_clear_list_clicked();

    void on_com_selector_highlighted(int index);

private:
    Ui::MainWindow *ui;

    QList<QSerialPortInfo> available_ports;
    QSerialPort autosampler_port;
};
#endif // MAINWINDOW_H
