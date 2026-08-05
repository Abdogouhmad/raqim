#include "calculatorwindow.h"
#include "thememanager.h"

#include <QApplication>
#include <QIcon>

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);

    // Must match Exec= in raqim.desktop and the installed icon name.
    // Lets Wayland compositors like niri associate this running window
    // with the right .desktop entry (icon in overview/switcher, etc.).
    app.setDesktopFileName("raqim");
    app.setWindowIcon(QIcon::fromTheme("raqim"));

    ThemeManager themeManager;
    themeManager.applyToApplication();

    CalculatorWindow window;
    window.setMinimumSize(240, 340);
    window.resize(320, 460);
    window.show();

    return app.exec();
}
