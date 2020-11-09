#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QtGui>
#include "qmessagebox.h"

#include "globales.h"
#include "libusb.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    setWindowTitle("Potenciostato");

    //asocio la accion triggered del ui->actionAyuda a la funcion help() (dentro de slots el mainwindow.h)
    connect(ui->actionAyuda, SIGNAL(triggered()), this, SLOT(help()));
    MainWindow::makePlot();

    auto timer = new QTimer(this);
    timer->setSingleShot(false);
    connect(timer, &QTimer::timeout, this, &MainWindow::onTimeout);
    timer->start(5000);
    //el timer se usara para refrescar el grafico en base a la medicion obtenida por USB del potenciostato

}

MainWindow::~MainWindow() //al cerrar la ventana se llama a este metodo
{
    delete ui;

    if(connected > 0)
    {
        libusb_free_device_list(devs,1);
        libusb_exit(ctx);
    }
}

void MainWindow::onTimeout(){
    qDebug() << "timeout";
}

void MainWindow::makePlot()
{
    // generate some data:
    QVector<double> x(101), y(101); // initialize with entries 0..100
    for (int i=0; i<101; ++i)
    {
        x[i] = i/50.0 - 1; // x goes from -1 to 1
        y[i] = x[i]*x[i]; // let's plot a quadratic function
    }
    // create graph and assign data to it:
    ui->customPlot->addGraph();
    ui->customPlot->graph(0)->setData(x, y);
    // give the axes some labels:
    ui->customPlot->xAxis->setLabel("x");
    ui->customPlot->yAxis->setLabel("y");
    // set axes ranges, so we see all data:
    ui->customPlot->xAxis->setRange(-1, 1);
    ui->customPlot->yAxis->setRange(0, 1);
    ui->customPlot->replot();
}

void MainWindow::on_pushButton_clicked()
{
    qDebug() << "hola";
}

void MainWindow::on_Conectar_Bt_clicked()
{
    signed int r;
    unsigned int cnt;
    unsigned char i;
    signed int j, k, p;

    connected = 1;

    r = libusb_init(&ctx);

    //libusb_set_debug(ctx, 3); //set verbosity level to 3, as suggested in the documentation

    cnt = libusb_get_device_list(ctx,&devs);

    qDebug() << "Hay" << cnt << "dispositivos conectados";

    for (i = 0; i < cnt; i++)
    {
        r = libusb_get_device_descriptor(devs[i],&desc);

        if(r < 0)
        {
            qDebug() << "Error con el device descriptor del dispositivo";
        }

        if(desc.idProduct == MY_PRODUCT && desc.idVendor == MY_VENDOR)
        {
            //ui->Enviar_Bt->setEnabled(true);

            Dispositivo = i;

            qDebug() << "Mi dispositivo es el" << i;
            qDebug() << "Tiene " << desc.bNumConfigurations << " cantidad de configuraciones";
            qDebug() << "Vendor ID: " << desc.idVendor;
            qDebug() << "Product ID: " << desc.idProduct;
            qDebug() << "Device Class: " << desc.bDeviceClass;
            qDebug() << "Serial Number: " << desc.iSerialNumber;


            libusb_get_config_descriptor(devs[Dispositivo], 0, &config);


            qDebug() <<"Interfaces: "<<(uint8_t)config->bNumInterfaces;

            for(j=0; j<(uint8_t )(config->bNumInterfaces); j++)
            {
                inter = &config->interface[j];

                qDebug()<<"Cantidad de conf. alternativas: "<<(uint8_t) (inter->num_altsetting);

                for(k=0; k < (uint8_t) (inter->num_altsetting); k++)
                {
                    interdesc = &inter->altsetting[k];

                    qDebug() << "Cant. de Interfaces: "<<(uint8_t )interdesc->bInterfaceNumber;

                    qDebug() << "Cant. de Endpoints: "<<(uint8_t )interdesc->bNumEndpoints;

                    for(p= 0 ; p<(uint8_t )interdesc->bNumEndpoints; p++)
                    {
                        epdesc = &interdesc->endpoint[p];

                        qDebug() << "Tipo de Descriptor: "<<(uint8_t )epdesc->bDescriptorType;

                        qDebug() << "Dirección de EP: "<<(uint8_t )epdesc->bEndpointAddress;

                    }

                }
            }

            libusb_free_config_descriptor(config);


            r = libusb_open(devs[Dispositivo],&dev_handle);

            if(r<0)
            {
                qDebug() << "Error" << r << "abriendo dispositivo";
                return;
            }
        }
    }

    //QString Buff;

    unsigned int Datos;
    unsigned int Potencia;

    //unsigned char TxData [4];
    unsigned char TxData[4];
    //unsigned char RxData [4];
    unsigned char RxData[4];

    int Enviados=0;

    int actual_length;

    //Buff = ui->TBox->toPlainText();

    //Datos = Buff.toInt();


    /*for (i=0 ; i < 4; i++)
    {
        TxData[3-i] = Datos % 10;
        Datos = Datos / 10;
    }

    qDebug() << "-----Inicio Bytes Enviados-----";
    qDebug() << TxData[0];
    qDebug() << TxData[1];
    qDebug() << TxData[2];
    qDebug() << TxData[3];
    qDebug() << "-----Fin Bytes Enviados-----";
    */

    //if(data1 > 80)
    //{
    //    QMessageBox::warning(this,"Error","El valor ingresado es inválido",QMessageBox::Retry);

    //    return;
    //}

    libusb_claim_interface(dev_handle, 0);

    unsigned char buffer[64] = {0x0};
    unsigned char recv_data[64] = {0x0};
    buffer[0] = 0xA;
    qDebug() << "antes de recibir buffer" << buffer[0];
    qDebug() << "antes de recibir: recv_data" << recv_data[0];
    int len;
    int send_ret, recv_ret;

    send_ret = libusb_interrupt_transfer(dev_handle, 0x02, buffer, (sizeof(buffer)) * 1, &len, 1000);
    recv_ret = libusb_interrupt_transfer(dev_handle, 0x82, recv_data, (sizeof(recv_data)) * 1, &len, 1000);
    //int
    //int recv_ret = libusb_interrupt_transfer(dev_handle, 0x81, recv_data, (sizeof(recv_data)) * 64, &len, 1000);

    qDebug() << "codigo envio" << send_ret;
    qDebug() << "dato enviado" << buffer[0];
    qDebug() << "codigo recepcion" << recv_ret;
    qDebug() << "dato recibido" << recv_data[0];

    Enviados = libusb_interrupt_transfer(dev_handle , 0x02 , TxData , (sizeof (TxData)) * 4, &actual_length , 1000);
    libusb_interrupt_transfer(dev_handle , 0x82 , RxData , (sizeof (RxData)) * 4, &actual_length , 1000);

    //Datos = 0;

    for (i=0 ; i < 4; i++)
    {
        Potencia = (unsigned int) pow((double) 10 , (double) i);
        //Datos += RxData[3-i] * Potencia;
    }

    //Buff=QString::number(Datos);

    //ui->TBox_2->setText(Buff);

    qDebug() << "-----Inicio Bytes Recibidos-----";
    qDebug() << RxData[0];
    qDebug() << RxData[1];
    qDebug() << RxData[2];
    qDebug() << RxData[3];
    qDebug() << "-----Fin Bytes Recibidos-----";
    qDebug() << Dispositivo;
    qDebug() << Enviados;
}

void MainWindow::help()
{
    qDebug() << "Ayuda";
    //link a documentacion https://doc.qt.io/qt-5/qmessagebox.html#about
    QMessageBox::about(this,"Sobre Potenciostato","Potenciostato versión 1.0");
}
