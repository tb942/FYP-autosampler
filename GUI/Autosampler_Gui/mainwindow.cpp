#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    PopulateSerialPorts();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::PopulateSerialPorts()
{
    if(ui->com_selector->count()-1 == QSerialPortInfo::availablePorts().count()){
        return;
    }

    for (int i = 1;i < ui->com_selector->count();i++) {
        ui->com_selector->removeItem(i);
    }

    available_ports.clear();

    Q_FOREACH(QSerialPortInfo port, QSerialPortInfo::availablePorts()) {
            ui->com_selector->addItem(port.portName() + " " + port.description());
            available_ports.push_back(port);
        }
}


void MainWindow::on_program_selector_currentIndexChanged(int index)
{
    if (ui->program_selector->currentIndex() == 0 || ui->button_connect_serial->text() == "Connect") {
        ui->button_write_program->setEnabled(false);
    } else {
        ui->button_write_program->setEnabled(true);
    }
}

void MainWindow::on_com_selector_currentIndexChanged(int index)
{
    if (ui->com_selector->currentIndex() == 0 && ui->baud_selector->currentIndex() == 0) {
        ui->button_connect_serial->setEnabled(false);
    } else {
        ui->button_connect_serial->setEnabled(true);
        autosampler_port.setPort(available_ports[ui->com_selector->currentIndex()-1]);
    }
}

void MainWindow::on_baud_selector_currentIndexChanged(int index)
{
    on_com_selector_currentIndexChanged(index);
    autosampler_port.setBaudRate(ui->baud_selector->itemText(index).toUInt());
}



void MainWindow::on_function_selector_currentIndexChanged(int index)
{
    if (index == 0) {
        ui->button_add_command->setEnabled(false);
        ui->button_add_command_above->setEnabled(false);

        ui->value_selector->setRange(0,0);
    } else {
        ui->button_add_command->setEnabled(true);

        switch (index) {
        case 1:
            ui->value_selector->setRange(1,99);
            break;
        case 2:
            ui->value_selector->setRange(1,8);
            break;
        case 3:
            ui->value_selector->setRange(1,3);
            break;
        case 4:
            ui->value_selector->setRange(1,8);
            break;
        }
    }
}

void MainWindow::on_button_connect_serial_clicked()
{
    ui->serial_status->setText(QString::number(autosampler_port.baudRate()));

    if(autosampler_port.isOpen()==true){
        autosampler_port.close();

        ui->serial_status->setText(QString::number(autosampler_port.baudRate()));
        ui->com_selector->setEnabled(true);
        ui->baud_selector->setEnabled(true);
        ui->button_write_program->setEnabled(false);

        ui->button_connect_serial->setText("Connect");

    } else if(autosampler_port.open(QIODevice::ReadWrite)==true){
        ui->serial_status->setText(QString::number(autosampler_port.baudRate()) + " Open");

        ui->com_selector->setEnabled(false);
        ui->baud_selector->setEnabled(false);

        ui->button_connect_serial->setText("Disconnect");
        on_program_selector_currentIndexChanged(0);
    } else {
        ui->serial_status->setText(QString::number(autosampler_port.baudRate()) + " Connection Failed");
    }
}

void MainWindow::on_button_write_program_clicked()
{
    if(autosampler_port.isOpen()==true){

        autosampler_port.write(QString(QString("W")+QString::number(ui->program_selector->currentIndex())+QString(";")).toUtf8());
        autosampler_port.write(QString(QString("P")+QString::number(ui->program_selector->currentIndex())+QString(";")).toUtf8());

        for (int i = 0;i<ui->program_list->count();i++) {
            autosampler_port.write(ui->program_list->item(i)->text().toUtf8());

            autosampler_port.waitForBytesWritten();
            QThread::msleep(50);
        }

        autosampler_port.write("P0;");
        QMessageBox::information(this, tr("Write Complete"),tr("Commands sent successfully"));
    }
}

void MainWindow::on_button_add_command_clicked()
{
    QString command_buffer;
    command_buffer = QString(ui->function_selector->currentText().at(0) + QString("%1").arg(ui->value_selector->value(), 2, 10, QChar('0')) + ';');

    if(ui->program_list->count()==0){
        ui->button_clear_list->setEnabled(true);
    }

    ui->program_list->addItem(command_buffer);
}

void MainWindow::on_program_list_itemDoubleClicked(QListWidgetItem *item)
{
    ui->program_list->takeItem(ui->program_list->currentRow());

    if (ui->program_list->count()==0){
        ui->button_add_command_above->setEnabled(false);
        ui->button_clear_list->setEnabled(false);
    }
}

void MainWindow::on_program_list_itemSelectionChanged()
{
    ui->button_add_command_above->setEnabled(true);
}

void MainWindow::on_button_add_command_above_clicked()
{
    QString command_buffer;
    command_buffer = QString(ui->function_selector->currentText().at(0) + QString("%1").arg(ui->value_selector->value(), 2, 10, QChar('0')) + ';');

    ui->program_list->insertItem(ui->program_list->currentRow(),command_buffer);
}

void MainWindow::on_com_selector_activated(int index)
{
    PopulateSerialPorts();
}

void MainWindow::on_button_clear_list_clicked()
{
    ui->program_list->clear();
    ui->button_clear_list->setEnabled(false);
}

void MainWindow::on_com_selector_highlighted(int index)
{
    if(index == 0){
        PopulateSerialPorts();
    }
}
