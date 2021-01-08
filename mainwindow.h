#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLabel>
#include <QTimer>
#include <QIcon>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_pushButton_clicked();
    void on_Conectar_Bt_clicked();
    void on_Bt_Iniciar_clicked();
    void on_Bt_Abortar_clicked();
    void on_Bt_Capturar_clicked();
    void on_Bt_Exportar_clicked();

    void help();

    void makePlot();
    void onTimeout();
    void realtimeData();
    void graficarValores();
    void refrescarValores(double x[10], double y[10]);
    void limpiarGraficos();
    void inicializarGraficos();
    void terminoMedicion();

private:
    Ui::MainWindow *ui;
};
#endif // MAINWINDOW_H
