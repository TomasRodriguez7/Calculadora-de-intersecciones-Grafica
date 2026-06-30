#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QString>

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

private:
    Ui::MainWindow *ui;
    double resX = 0, resY = 0, resZ = 0;
    double resAngulo = 0;
    QString resDesarrollo;
    bool hayInterseccion = false;
    bool sonCoincidentes = false;
    bool sonParalelos = false;

};

#endif // MAINWINDOW_H
