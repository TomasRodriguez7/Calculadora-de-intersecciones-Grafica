#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <cmath>
#include <QApplication>
#include <QMessageBox>
#include <QDoubleValidator>
#include <QLocale>
#include <QVBoxLayout>
#include <QWidget>
#include <Q3DScatter>
#include <QScatter3DSeries>
#include <QScatterDataProxy>
#include <Q3DCamera>
#include <Q3DTheme>
#include <QTimer>
#include <QEvent>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // 1. Iniciar en la página principal
    ui->stackedWidget->setCurrentIndex(0);

    // Validadores numéricos: bloquean letras/símbolos mientras se escribe
    configurarValidadores();

    connect(ui->botonVolverDesdeGrafica, &QPushButton::clicked, [=]() {
        destruirGrafica3D();
        ui->stackedWidget->setCurrentIndex(2); // Volver a Resultados
    });

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
        if (!validarCampos()) {
            return; // Se muestra el mensaje de error dentro de validarCampos()
        }

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
        ultimaRecta = recta;
        ultimoPlano = plano;
        ui->stackedWidget->setCurrentIndex(2); // Página Resultados
    });

    // --- Conexión: Ver Desarrollo ---
    connect(ui->botonVerDesarrollo, &QPushButton::clicked, [=]() {
        ui->stackedWidget->setCurrentIndex(3); // Página Desarrollo
        ui->textEditDesarrollo->setText(resDesarrollo);
    });

    // --- Conexión: Graficar (desde Resultados) ---
    connect(ui->botonGraficar, &QPushButton::clicked, [=]() {
        irAGrafica();
    });

    // --- Conexión: Graficar (desde Paso a Paso / Desarrollo) ---
    connect(ui->botonGraficar_2, &QPushButton::clicked, [=]() {
        irAGrafica();
    });

    // --- Conexión: Abrir Menú Opciones (desde Resultados) ---
    connect(ui->botonMenuOpciones, &QPushButton::clicked, [=]() {
        ui->stackedWidget->setCurrentIndex(5);
    });
}

MainWindow::~MainWindow() { delete ui; }

void MainWindow::changeEvent(QEvent* event) {
    // Red de seguridad: si la ventana se minimiza mientras la gráfica 3D
    // está activa (widget OpenGL embebido), la destruimos de inmediato
    // para evitar que pierda su contexto gráfico y crashee la aplicación.
    // El resto del tiempo (sin gráfica activa) minimizar es seguro.
    if (event->type() == QEvent::WindowStateChange && grafica3D) {
        if (isMinimized()) {
            destruirGrafica3D();
            QTimer::singleShot(0, this, [this]() {
                setWindowState(Qt::WindowNoState);
                show();
                ui->stackedWidget->setCurrentIndex(2); // Volvemos a Resultados, ya que la grafica ya no existe
            });
        }
    }
    QMainWindow::changeEvent(event);
}

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

void MainWindow::configurarValidadores() {
    // Acepta numeros decimales, con signo negativo opcional, sin limite de rango.
    // Se usa locale "C" para que el separador decimal sea siempre el punto (.)
    auto* validadorDouble = new QDoubleValidator(this);
    validadorDouble->setNotation(QDoubleValidator::StandardNotation);
    validadorDouble->setLocale(QLocale::c());

    QList<QLineEdit*> camposNumericos = {
        ui->inputX0, ui->inputY0, ui->inputZ0,
        ui->inputA, ui->inputB, ui->inputC,
        ui->inputAp, ui->inputBp, ui->inputCp, ui->inputDp
    };

    for (QLineEdit* campo : camposNumericos) {
        campo->setValidator(validadorDouble);
        campo->setPlaceholderText("0");
    }
}

bool MainWindow::validarCampos() {
    struct CampoInfo { QLineEdit* campo; QString nombre; };

    QList<CampoInfo> campos = {
        {ui->inputX0, "X0"}, {ui->inputY0, "Y0"}, {ui->inputZ0, "Z0"},
        {ui->inputA, "A (vector director)"}, {ui->inputB, "B (vector director)"}, {ui->inputC, "C (vector director)"},
        {ui->inputAp, "A (plano)"}, {ui->inputBp, "B (plano)"}, {ui->inputCp, "C (plano)"}, {ui->inputDp, "D (plano)"}
    };

    QStringList camposInvalidos;

    for (const CampoInfo& info : campos) {
        QString texto = info.campo->text().trimmed();
        bool ok = false;
        texto.toDouble(&ok);
        if (texto.isEmpty() || !ok) {
            camposInvalidos << info.nombre;
        }
    }

    if (!camposInvalidos.isEmpty()) {
        QMessageBox::critical(this, "Error de entrada",
            "Los siguientes campos deben contener solo numeros validos:\n\n"
            + camposInvalidos.join(", ")
            + "\n\nPor favor revisa los datos antes de calcular.");
        return false;
    }

    return true;
}

void MainWindow::irAGrafica() {
    if (!hayInterseccion) {
        QMessageBox::warning(this, "Aviso", "No hay punto de intersección válido para graficar.");
        return;
    }
    construirGrafica3D();
    actualizarGrafica3D();
    ui->stackedWidget->setCurrentIndex(4); // Página Gráfica

    // Forzamos el foco al widget 3D para que reciba los eventos
    // de mouse necesarios para poder rotar la vista arrastrando.
    QTimer::singleShot(50, this, [this]() {
        if (contenedorGraficaWidget) {
            contenedorGraficaWidget->setFocus(Qt::OtherFocusReason);
        }
    });
}

void MainWindow::destruirGrafica3D() {
    // Destruimos el widget contenedor y la ventana Q3DScatter al salir
    // de la página de gráfica. Esto evita que la ventana OpenGL nativa
    // quede expuesta el resto del tiempo que el usuario use la app,
    // que es lo que causaba el crash al minimizar/maximizar.
    if (contenedorGraficaWidget) {
        ui->contenedorGrafica3D->layout()->removeWidget(contenedorGraficaWidget);
        delete contenedorGraficaWidget; // esto también elimina grafica3D internamente
        contenedorGraficaWidget = nullptr;
        grafica3D = nullptr;
    }
    delete ui->contenedorGrafica3D->layout();
}

void MainWindow::construirGrafica3D() {
    if (grafica3D) {
        return; // ya existe, no la recreamos
    }

    grafica3D = new Q3DScatter();

    // Tema oscuro para que combine con el fondo de la pantalla
    grafica3D->activeTheme()->setType(Q3DTheme::ThemeQt);
    grafica3D->activeTheme()->setBackgroundEnabled(false);
    grafica3D->activeTheme()->setGridEnabled(true);
    grafica3D->scene()->activeCamera()->setCameraPreset(Q3DCamera::CameraPresetIsometricLeft);

    grafica3D->axisX()->setTitle("X");
    grafica3D->axisY()->setTitle("Z"); // En Qt Scatter, Y es la altura visual; usamos Z matemático ahí
    grafica3D->axisZ()->setTitle("Y");
    grafica3D->axisX()->setTitleVisible(true);
    grafica3D->axisY()->setTitleVisible(true);
    grafica3D->axisZ()->setTitleVisible(true);

    // Serie del plano: malla de puntos semitransparente
    QScatter3DSeries* seriePlano = new QScatter3DSeries();
    seriePlano->setName("Plano");
    seriePlano->setItemSize(0.08f);
    seriePlano->setBaseColor(QColor(52, 152, 219)); // azul
    seriePlano->setMesh(QAbstract3DSeries::MeshPoint);

    // Serie de la recta: puntos a lo largo del vector director
    QScatter3DSeries* serieRecta = new QScatter3DSeries();
    serieRecta->setName("Recta");
    serieRecta->setItemSize(0.12f);
    serieRecta->setBaseColor(QColor(243, 156, 18)); // naranjo
    serieRecta->setMesh(QAbstract3DSeries::MeshSphere);

    // Serie del punto de intersección: un único punto destacado
    QScatter3DSeries* seriePunto = new QScatter3DSeries();
    seriePunto->setName("Intersección");
    seriePunto->setItemSize(0.25f);
    seriePunto->setBaseColor(QColor(231, 76, 60)); // rojo
    seriePunto->setMesh(QAbstract3DSeries::MeshSphere);

    grafica3D->addSeries(seriePlano);
    grafica3D->addSeries(serieRecta);
    grafica3D->addSeries(seriePunto);

    // Embebemos la ventana Q3DScatter dentro del QWidget contenedor del .ui
    contenedorGraficaWidget = QWidget::createWindowContainer(grafica3D, ui->contenedorGrafica3D);
    contenedorGraficaWidget->setMinimumSize(100, 100);
    contenedorGraficaWidget->setFocusPolicy(Qt::StrongFocus);
    contenedorGraficaWidget->setMouseTracking(true);

    QVBoxLayout* layout = new QVBoxLayout(ui->contenedorGrafica3D);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(contenedorGraficaWidget);
}

void MainWindow::actualizarGrafica3D() {
    if (!grafica3D) return;

    QList<QScatter3DSeries*> series = grafica3D->seriesList();
    if (series.size() < 3) return;

    QScatter3DSeries* seriePlano = series[0];
    QScatter3DSeries* serieRecta = series[1];
    QScatter3DSeries* seriePunto = series[2];

    // --- Generar malla de puntos para representar el plano ---
    // Tomamos como centro el punto de intersección (o P0 si no hay) y
    // construimos un parche del plano alrededor usando dos vectores
    // directores del plano, obtenidos a partir de la normal (A,B,C).
    QScatterDataArray* datosPlano = new QScatterDataArray();
    double A = ultimoPlano.A, B = ultimoPlano.B, C = ultimoPlano.C, D = ultimoPlano.D;
    double magNormal = std::sqrt(A*A + B*B + C*C);
    if (magNormal < 1e-9) magNormal = 1.0;

    // Vector u: cualquier vector no paralelo a la normal, lo cruzamos para obtener una base del plano
    double ux, uy, uz;
    if (std::abs(A) < 0.9) { ux = 1; uy = 0; uz = 0; } else { ux = 0; uy = 1; uz = 0; }
    // u = normal x ref
    double u1 = B*uz - C*uy;
    double u2 = C*ux - A*uz;
    double u3 = A*uy - B*ux;
    double magU = std::sqrt(u1*u1 + u2*u2 + u3*u3);
    if (magU < 1e-9) { u1 = 1; u2 = 0; u3 = 0; magU = 1; }
    u1 /= magU; u2 /= magU; u3 /= magU;

    // v = normal x u  (segunda dirección del plano, perpendicular a u)
    double v1 = (B*u3 - C*u2);
    double v2 = (C*u1 - A*u3);
    double v3 = (A*u2 - B*u1);
    double magV = std::sqrt(v1*v1 + v2*v2 + v3*v3);
    if (magV < 1e-9) magV = 1.0;
    v1 /= magV; v2 /= magV; v3 /= magV;

    // Punto base del plano: el de intersección si existe, si no buscamos uno que cumpla la ecuación
    double centroX, centroY, centroZ;
    if (hayInterseccion) {
        centroX = resX; centroY = resY; centroZ = resZ;
    } else {
        // Punto cualquiera del plano: despejamos sobre el eje con coeficiente mas grande
        centroX = 0; centroY = 0; centroZ = 0;
        if (std::abs(C) > 1e-6) centroZ = -D / C;
        else if (std::abs(B) > 1e-6) centroY = -D / B;
        else if (std::abs(A) > 1e-6) centroX = -D / A;
    }

    const double EXTENSION = 5.0; // que tan grande se ve el parche del plano
    const int PASOS = 12;
    for (int i = -PASOS; i <= PASOS; ++i) {
        for (int j = -PASOS; j <= PASOS; ++j) {
            double s = (EXTENSION / PASOS) * i;
            double t = (EXTENSION / PASOS) * j;
            float px = float(centroX + s * u1 + t * v1);
            float py = float(centroZ + s * u3 + t * v3); // eje Y visual = Z matemático
            float pz = float(centroY + s * u2 + t * v2); // eje Z visual = Y matemático
            *datosPlano << QVector3D(px, py, pz);
        }
    }
    seriePlano->dataProxy()->resetArray(datosPlano);

    // --- Generar puntos de la recta a lo largo del parámetro t ---
    QScatterDataArray* datosRecta = new QScatterDataArray();
    const double RANGO_T = 6.0;
    const int PASOS_RECTA = 60;
    for (int i = 0; i <= PASOS_RECTA; ++i) {
        double t = -RANGO_T + (2 * RANGO_T) * i / PASOS_RECTA;
        float px = float(ultimaRecta.x0 + ultimaRecta.a * t);
        float py = float(ultimaRecta.z0 + ultimaRecta.c * t);
        float pz = float(ultimaRecta.y0 + ultimaRecta.b * t);
        *datosRecta << QVector3D(px, py, pz);
    }
    serieRecta->dataProxy()->resetArray(datosRecta);

    // --- Punto de intersección destacado ---
    QScatterDataArray* datosPunto = new QScatterDataArray();
    if (hayInterseccion) {
        *datosPunto << QVector3D(float(resX), float(resZ), float(resY));
    }
    seriePunto->dataProxy()->resetArray(datosPunto);

    // Ajustamos el rango de los ejes para que todo sea visible
    float rango = float(EXTENSION + 1.0);
    grafica3D->axisX()->setRange(float(centroX) - rango, float(centroX) + rango);
    grafica3D->axisY()->setRange(float(centroZ) - rango, float(centroZ) + rango);
    grafica3D->axisZ()->setRange(float(centroY) - rango, float(centroY) + rango);
}
