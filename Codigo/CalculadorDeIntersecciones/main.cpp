#include "mainwindow.h"

#include <QApplication>
#include <QLocale>
#include <QTranslator>
#include <QScreen>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QTranslator translator;
    const QStringList uiLanguages = QLocale::system().uiLanguages();
    for (const QString &locale : uiLanguages) {
        const QString baseName = "CalculadorDeIntersecciones_" + QLocale(locale).name();
        if (translator.load(":/i18n/" + baseName)) {
            a.installTranslator(&translator);
            break;
        }
    }
    MainWindow w;

    // El diseño interno (mainwindow.ui) ya está escalado a 1280x697,
    // un tamaño que entra en pantallas desde 1366x768 hacia arriba.
    // Quitamos los botones de minimizar y maximizar: la gráfica 3D usa
    // una ventana OpenGL embebida (Q3DScatter) que pierde su contexto
    // gráfico y crashea la app si la ventana cambia de estado (minimizada/
    // maximizada). Como el tamaño ya es fijo, solo dejamos el botón de cerrar.
    w.setWindowFlags((w.windowFlags() | Qt::CustomizeWindowHint)
                      & ~Qt::WindowMaximizeButtonHint
                      & ~Qt::WindowMinimizeButtonHint);
    w.setMinimumSize(w.size());
    w.setMaximumSize(w.size());

    QRect pantallaDisponible = QGuiApplication::primaryScreen()->availableGeometry();
    int x = pantallaDisponible.x() + (pantallaDisponible.width() - w.width()) / 2;
    int y = pantallaDisponible.y() + (pantallaDisponible.height() - w.height()) / 2;
    w.move(qMax(x, 0), qMax(y, 0));

    w.show();
    return a.exec();
}
