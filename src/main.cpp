#include "calculatorwindow.h"
#include "thememanager.h"

#include <QApplication>

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);

    ThemeManager themeManager;
    themeManager.applyToApplication();

    CalculatorWindow window;
    window.setMinimumSize(240, 340);
    window.resize(320, 460);
    window.show();

    return app.exec();
}
