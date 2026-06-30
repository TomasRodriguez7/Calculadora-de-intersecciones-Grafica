#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <cmath>
#include <QApplication>
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // 1. Iniciar en la página principal
    ui->stackedWidget->setCurrentIndex(0);

    // --- Navegación General ---
    connect(ui->botonContinuar, &QPushButton::clicked, [=]() { ui->stackedWidget->setCurrentIndex(6); });
    connect(ui->botonNuevoCalculo, &QPushButton::clicked, [=]() { ui->stackedWidget->setCurrentIndex(1); });
    connect(ui->botonSalir, &QPushButton::clicked, QApplication::quit);

    // --- Conexión: Menú de Opciones (Botones nuevos) ---
    connect(ui->botonNuevoCalculoOpciones, &QPushButton::clicked, [=]() {
        // Limpiamos los inputs antes de ir a nuevo cálculo
        ui->inputX0->clear(); ui->inputY0->clear(); ui->inputZ0->clear();
        ui->inputA->clear(); ui->inputB->clear(); ui->inputC->clear();
        ui->inputAp->clear(); ui->inputBp->clear(); ui->inputCp->clear(); ui->inputDp->clear();
        ui->stackedWidget->setCurrentIndex(1);
    });

    connect(ui->botonVolverMenu, &QPushButton::clicked, [=]() {
        ui->stackedWidget->setCurrentIndex(0); // Vuelve al Menú Principal
    });

    connect(ui->botonSalirOpciones, &QPushButton::clicked, QApplication::quit);

    // --- Conexión: Calcular ---
    connect(ui->botonCalcular, &QPushButton::clicked, [=]() {
        Recta3D recta;
        recta.x0 = ui->inputX0->text().toDouble();
        recta.y0 = ui->inputY0->text().toDouble();
        recta.z0 = ui->inputZ0->text().toDouble();
        recta.a = ui->inputA->text().toDouble();
        recta.b = ui->inputB->text().toDouble();
        recta.c = ui->inputC->text().toDouble();
        recta.ingresada = true;

        Plano3D plano;
        plano.A = ui->inputAp->text().toDouble();
        plano.B = ui->inputBp->text().toDouble();
        plano.C = ui->inputCp->text().toDouble();
        plano.D = ui->inputDp->text().toDouble();
        plano.ingresado = true;

        calcularInterseccion(recta, plano);
        ui->stackedWidget->setCurrentIndex(2); // Página Resultados
    });

    // --- Conexión: Ver Desarrollo ---
    connect(ui->botonVerDesarrollo, &QPushButton::clicked, [=]() {
        ui->stackedWidget->setCurrentIndex(3); // Página Desarrollo
        ui->textEditDesarrollo->setText(resDesarrollo);
    });

    // --- Conexión: Graficar ---
    connect(ui->botonGraficar, &QPushButton::clicked, [=]() {
        if (!hayInterseccion) {
            QMessageBox::warning(this, "Aviso", "No hay punto de intersección válido para graficar.");
        } else {
            ui->stackedWidget->setCurrentIndex(4); // Página Gráfica
        }
    });

    // --- Conexión: Abrir Menú Opciones (desde Resultados) ---
    connect(ui->botonMenuOpciones, &QPushButton::clicked, [=]() {
        ui->stackedWidget->setCurrentIndex(5);
    });
}

MainWindow::~MainWindow() { delete ui; }

void MainWindow::calcularInterseccion(const Recta3D& recta, const Plano3D& plano) {
    const double EPSILON = 1e-6;
    const double PI = std::acos(-1.0);

    double productoPunto = (plano.A * recta.a) + (plano.B * recta.b) + (plano.C * recta.c);
    double evaluacionPunto = (plano.A * recta.x0) + (plano.B * recta.y0) + (plano.C * recta.z0) + plano.D;

    hayInterseccion = false;
    resDesarrollo = "--- DESARROLLO MATEMATICO DETALLADO ---\n\n";

    resDesarrollo += "1. Definicion de elementos:\n";
    resDesarrollo += "   * P0: Punto inicial de la recta (" + QString::number(recta.x0) + ", " + QString::number(recta.y0) + ", " + QString::number(recta.z0) + ")\n";
    resDesarrollo += "   * v (Vector director): (" + QString::number(recta.a) + ", " + QString::number(recta.b) + ", " + QString::number(recta.c) + ")\n";
    resDesarrollo += "   * n (Vector normal): (" + QString::number(plano.A) + ", " + QString::number(plano.B) + ", " + QString::number(plano.C) + ")\n";
    resDesarrollo += "   * Ecuacion Plano: " + QString::number(plano.A) + "x + " + QString::number(plano.B) + "y + " + QString::number(plano.C) + "z + (" + QString::number(plano.D) + ") = 0\n\n";

    resDesarrollo += "2. Calculo del producto punto (n · v):\n";
    resDesarrollo += "   (Este valor indica si la recta es perpendicular al plano o si son paralelos)\n";
    resDesarrollo += "   n·v = (" + QString::number(plano.A) + "*" + QString::number(recta.a) + ") + (" + QString::number(plano.B) + "*" + QString::number(recta.b) + ") + (" + QString::number(plano.C) + "*" + QString::number(recta.c) + ") = " + QString::number(productoPunto) + "\n\n";

    resDesarrollo += "3. Evaluacion del punto inicial P0 en la ecuacion del plano:\n";
    resDesarrollo += "   (Si el resultado es 0, el punto ya pertenece al plano)\n";
    resDesarrollo += "   Eval = " + QString::number(plano.A) + "(" + QString::number(recta.x0) + ") + " + QString::number(plano.B) + "(" + QString::number(recta.y0) + ") + " + QString::number(plano.C) + "(" + QString::number(recta.z0) + ") + (" + QString::number(plano.D) + ") = " + QString::number(evaluacionPunto) + "\n\n";

    if (std::abs(productoPunto) > EPSILON) {
        double t = -evaluacionPunto / productoPunto;
        resX = recta.x0 + recta.a * t;
        resY = recta.y0 + recta.b * t;
        resZ = recta.z0 + recta.c * t;

        double magRecta = std::sqrt(recta.a * recta.a + recta.b * recta.b + recta.c * recta.c);
        double magPlano = std::sqrt(plano.A * plano.A + plano.B * plano.B + plano.C * plano.C);
        resAngulo = std::asin(std::abs(productoPunto) / (magRecta * magPlano)) * (180.0 / PI);

        hayInterseccion = true;

        resDesarrollo += "4. Resultado:\n";
        resDesarrollo += "   Como n·v != 0, la recta y el plano se cruzan en un punto unico.\n";
        resDesarrollo += "   t: Parametro escalar que indica cuanto avanzar desde P0 en direccion v para tocar el plano.\n";
        resDesarrollo += "   t = -(" + QString::number(evaluacionPunto) + ") / " + QString::number(productoPunto) + " = " + QString::number(t) + "\n";
        resDesarrollo += "   Punto Interseccion = (" + QString::number(resX, 'f', 2) + ", " + QString::number(resY, 'f', 2) + ", " + QString::number(resZ, 'f', 2) + ")\n";
        resDesarrollo += "   Angulo = " + QString::number(resAngulo, 'f', 2) + " grados.";

        QString mensajeFinal = "Se intersectan en 1 punto.\n";
        mensajeFinal += "Punto: (" + QString::number(resX, 'f', 2) + ", " + QString::number(resY, 'f', 2) + ", " + QString::number(resZ, 'f', 2) + ")\n";
        mensajeFinal += "Angulo: " + QString::number(resAngulo, 'f', 2) + "°";
        ui->labelResultado->setText(mensajeFinal);
    }
    else {
        resDesarrollo += "4. Resultado:\n   La recta y el plano son paralelos o coincidentes (n·v = 0).";
        if (std::abs(evaluacionPunto) <= EPSILON) {
            ui->labelResultado->setText("Relación: COINCIDENTES (La recta pertenece al plano).");
        } else {
            ui->labelResultado->setText("Relación: PARALELOS (No se intersectan).");
        }
    }
}
