#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QtGui>
#include "qmessagebox.h"

#include "globales.h"
#include "libusb.h"

#define CANT_VALORES 51

QVector<double> valoresX(CANT_VALORES), valoresY(CANT_VALORES); // declara vectores con 10 posiciones (0..9)

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    setWindowTitle("Potenciostato");

    //asocio la accion triggered del ui->actionAyuda a la funcion help() (dentro de slots el mainwindow.h)
    connect(ui->actionAyuda, SIGNAL(triggered()), this, SLOT(help()));
    //MainWindow::makePlot();
    MainWindow::graficarValores();

    // Configuracion timer
    auto timer = new QTimer(this);
    timer->setSingleShot(false);
    connect(timer, &QTimer::timeout, this, &MainWindow::onTimeout);
    //timer->start(5000);
    //el timer se usara para refrescar el grafico en base a la medicion obtenida por USB del potenciostato

    /*
    //Configuracion real time plot
    ui->customPlot->addGraph(); // blue line
    ui->customPlot->graph(0)->setPen(QPen(QColor(40, 110, 255)));
    ui->customPlot->addGraph(); // red line
    ui->customPlot->graph(1)->setPen(QPen(QColor(255, 110, 40)));

    QSharedPointer<QCPAxisTickerTime> timeTicker(new QCPAxisTickerTime);
    timeTicker->setTimeFormat("%h:%m:%s");
    ui->customPlot->xAxis->setTicker(timeTicker);
    ui->customPlot->axisRect()->setupFullAxesBox();
    ui->customPlot->yAxis->setRange(-1.2, 1.2);

    // make left and bottom axes transfer their ranges to right and top axes:
    connect(ui->customPlot->xAxis, SIGNAL(rangeChanged(QCPRange)), ui->customPlot->xAxis2, SLOT(setRange(QCPRange)));
    connect(ui->customPlot->yAxis, SIGNAL(rangeChanged(QCPRange)), ui->customPlot->yAxis2, SLOT(setRange(QCPRange)));

    // setup a timer that repeatedly calls MainWindow::realtimeData:
    connect(timer, &QTimer::timeout, this, &MainWindow::realtimeData);
    timer->start(0); // Interval 0 means to refresh as fast as possible
    */
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

void MainWindow::graficarValores(){
    double muestraX[CANT_VALORES] = {
        0.50, 0.51, 0.52, 0.53, 0.54, 0.55, 0.56, 0.57, 0.58, 0.59,
        0.60, 0.61, 0.62, 0.63, 0.64, 0.65, 0.66, 0.67, 0.68, 0.69,
        0.70, 0.71, 0.72, 0.73, 0.74, 0.75, 0.76, 0.77, 0.78, 0.79,
        0.80, 0.81, 0.82, 0.83, 0.84, 0.85, 0.86, 0.87, 0.88, 0.89,
        0.90, 0.91, 0.92, 0.93, 0.94, 0.95, 0.96, 0.97, 0.98, 0.99,
        1.00
    };
    double muestraY[CANT_VALORES] = {
        0.010, 0.010, 0.010, 0.010, 0.010, 0.010, 0.010, 0.010, 0.010, 0.010,
        0.011, 0.020, 0.011, 0.015, 0.011, 0.018, 0.023, 0.011, 0.036, 0.050,
        0.111, 0.332, 0.552, 0.773, 0.994, 1.210, 1.430, 1.660, 1.880, 2.100,
        2.320, 2.540, 2.760, 2.980, 3.200, 3.420, 3.500, 3.550, 3.580, 3.570,
        3.520, 3.460, 3.410, 3.370, 3.320, 3.250, 3.190, 3.140, 3.090, 3.040,
        3.000
    };
    int i;
    double multiplicadorX, multiplicadorY;

    // aplicamos escalamiento por multiplicadores de unidades

    multiplicadorX = 1.00;
    for (i=0; i<CANT_VALORES; ++i)
    {
        muestraX[i] = muestraX[i]*multiplicadorX;
    }

    multiplicadorY = 10.00;
    for (i=0; i<CANT_VALORES; ++i)
    {
        muestraY[i] = muestraY[i]*multiplicadorY;
    }

    // generate some data:
    for (i=0; i<CANT_VALORES; ++i)
    {
        valoresX[i] = muestraX[i];
        valoresY[i] = muestraY[i];
    }
    // create graph
    ui->customPlot->addGraph();
    // give the axes some labels:
    ui->customPlot->xAxis->setLabel("Tension [V]");
    ui->customPlot->yAxis->setLabel("Corriente [uA]");
    // se puede especificar el rango fijo
    // set axes ranges, so we see all data:
    ui->customPlot->xAxis->setRange(0.5, 1.0);
    ui->customPlot->yAxis->setRange(0, 40);

    // hace que los ejes escalen automaticamente
    ui->customPlot->graph(0)->rescaleAxes();
    connect(ui->customPlot->xAxis, SIGNAL(rangeChanged(QCPRange)), ui->customPlot->xAxis2, SLOT(setRange(QCPRange)));
    connect(ui->customPlot->yAxis, SIGNAL(rangeChanged(QCPRange)), ui->customPlot->yAxis2, SLOT(setRange(QCPRange)));

    // poner datos en el grafico
    ui->customPlot->graph(0)->setPen(QPen(Qt::red));
    ui->customPlot->graph(0)->setBrush(QBrush(QColor(255, 0, 0, 20)));
    ui->customPlot->graph(0)->setData(valoresX, valoresY);
    ui->customPlot->replot();

    // hace interactivo al grafico para que se pueda arrastrar, hacer zoom y seleccionar las curvas
    ui->customPlot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom | QCP::iSelectPlottables);
}

void MainWindow::refrescarValores(double x[CANT_VALORES], double y[CANT_VALORES]){
    // opcionalmente, para hallar el largo del vector x
    //size_t n = (&x)[1] - x;
    //int largo = (int) n;

    for (int i=0; i < CANT_VALORES; ++i)
    {
        valoresX[i] = x[i];
        valoresY[i] = y[i];
    }
    // poner datos en el grafico
    //ui->customPlot->addGraph();
    ui->customPlot->graph(0)->setData(valoresX, valoresY);
    ui->customPlot->replot();
}

void MainWindow::realtimeData(){
    static QTime time(QTime::currentTime());
    // calculate two new data points:
    double key = time.elapsed()/1000.0; // time elapsed since start of demo, in seconds
    static double lastPointKey = 0;
    if (key-lastPointKey > 0.002) // at most add point every 2 ms
    {
        // add data to lines:
        ui->customPlot->graph(0)->addData(key, qSin(key)+qrand()/(double)RAND_MAX*1*qSin(key/0.3843));
        ui->customPlot->graph(1)->addData(key, qCos(key)+qrand()/(double)RAND_MAX*0.5*qSin(key/0.4364));
        // rescale value (vertical) axis to fit the current data:
        //ui->customPlot->graph(0)->rescaleValueAxis();
        //ui->customPlot->graph(1)->rescaleValueAxis(true);
        lastPointKey = key;
    }
    // make key axis range scroll with the data (at a constant range size of 8):
    ui->customPlot->xAxis->setRange(key, 8, Qt::AlignRight);
    ui->customPlot->replot();

    // calculate frames per second:
    static double lastFpsKey;
    static int frameCount;
    ++frameCount;
    if (key-lastFpsKey > 2) // average fps over 2 seconds
    {
        /*ui->statusBar->showMessage(
            QString("%1 FPS, Total Data points: %2")
                .arg(frameCount/(key-lastFpsKey), 0, 'f', 0)
                .arg(ui->customPlot->graph(0)->data()->size()+ui->customPlot->graph(1)->data()->size())
                , 0);*/
        qDebug() << QString("%1 FPS, Total Data points: %2")
                        .arg(frameCount/(key-lastFpsKey), 0, 'f', 0)
                        .arg(ui->customPlot->graph(0)->data()->size()+ui->customPlot->graph(1)->data()->size());
        lastFpsKey = key;
        frameCount = 0;
    }
}

void MainWindow::onTimeout(){
    double muestraX[CANT_VALORES] = {0};
    double muestraY[CANT_VALORES] = {0};


    for (int i=0; i < CANT_VALORES; ++i)
    {
        muestraX[i] = valoresX[i];
        if (i%2 > 0)
            muestraY[i] = valoresY[i]+0.3;
        else
            muestraY[i] = valoresY[i]+0.25;
    }

    qDebug() << "Envio de refresco al grafico";
    MainWindow::refrescarValores(muestraX, muestraY);

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
