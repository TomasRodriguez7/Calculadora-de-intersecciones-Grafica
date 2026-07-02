#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QString>
#include <QLineEdit>
#include <QtDataVisualization>

QT_USE_NAMESPACE

struct Recta3D {
    double x0, y0, z0;
    double a, b, c;
    bool ingresada = false;
};

struct Plano3D {
    double A, B, C, D;
    bool ingresado = false;
};

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void calcularInterseccion(const Recta3D& recta, const Plano3D& plano);

protected:
    void changeEvent(QEvent* event) override;

private:
    Ui::MainWindow *ui;
    double resX = 0, resY = 0, resZ = 0;
    double resAngulo = 0;
    QString resDesarrollo;
    bool hayInterseccion = false;
    bool sonCoincidentes = false;
    bool sonParalelos = false;

    void configurarValidadores();
    bool validarCampos();

    // Gráfica 3D interactiva
    Q3DScatter* grafica3D = nullptr;
    QWidget* contenedorGraficaWidget = nullptr;
    Recta3D ultimaRecta;
    Plano3D ultimoPlano;
    void construirGrafica3D();
    void actualizarGrafica3D();
    void destruirGrafica3D();
    void irAGrafica();

};

#endif // MAINWINDOW_H
