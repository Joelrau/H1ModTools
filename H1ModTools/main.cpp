#include "H1ModTools.h"

#include <QApplication>
#include <QStyleFactory>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // Set the style to Fusion
    QApplication::setStyle(QStyleFactory::create("Fusion"));

    QString style = R"(
    QWidget {
        background-color: #1e1e1e;
        color: #e0e0e0;
        font-family: "Segoe UI", "Noto Sans", sans-serif;
        font-size: 9.0pt;
    }

    /* QPushButton */
    QPushButton {
        background-color: #2d2d2d;
        border: 1px solid #444;
        padding: 6px 12px;
        border-radius: 4px;
    }
    QPushButton:hover {
        background-color: #ff8800;
        color: #000;
    }
    QPushButton:pressed {
        background-color: #cc6e00;
    }
    QPushButton:disabled {
        background-color: #3a3a3a;
        color: #777;
        border: 1px solid #333;
    }

    /* QLineEdit */
    QLineEdit {
        background-color: #2b2b2b;
        border: 1px solid #555;
        padding: 4px;
        border-radius: 3px;
        color: #e0e0e0;
    }
    QLineEdit:focus {
        border: 1px solid #ff8800;
    }
    QLineEdit:disabled {
        background-color: #3a3a3a;
        color: #777;
        border: 1px solid #333;
    }

    /* QListWidget */
    QListWidget {
        background-color: #2b2b2b;
        border: 1px solid #555;
        padding: 4px;
        color: #e0e0e0;
    }
    QListWidget:disabled {
        background-color: #3a3a3a;
        color: #777;
    }
    QListWidget::item:hover {
        background-color: #ff8800;
        color: #000;
    }
    QListWidget::item:selected {
        background-color: #ff8800;
        color: #000;
    }

    /* QTabWidget */
    QTabWidget::pane {
        border: 1px solid #444;
        top: -1px;
    }
    QTabBar::tab {
        background: #2d2d2d;
        border: 1px solid #444;
        padding: 6px;
        margin-right: 2px;
        border-top-left-radius: 4px;
        border-top-right-radius: 4px;
        color: #e0e0e0;
    }
    QTabBar::tab:selected {
        background: #1e1e1e;
        border-bottom: 1px solid #1e1e1e;
    }
    QTabBar::tab:hover {
        background: #ff8800;
        color: #000;
    }
    QTabBar::tab:disabled {
        background-color: #3a3a3a;
        color: #777;
    }

    /* QCheckBox */
    QCheckBox {
    color: #e0e0e0;
    spacing: 6px;
    }

    /* Disabled state */
    QCheckBox:disabled {
        color: #777;
        opacity: 0.7;
    }

    /* Ensure indicator inherits disabled state */
    QCheckBox::indicator:disabled {
        opacity: inherit;
    }

    /* QLabel */
    QLabel:disabled {
        color: #777;
    }

    /* QGroupBox */
    QGroupBox {
        border: 1px solid #444;
        border-radius: 5px;
        margin-top: 6px;
    }
    QGroupBox::title {
        subcontrol-origin: margin;
        subcontrol-position: top left;
        padding: 0 4px;
        color: #ff8800;
    }
    QGroupBox:disabled {
        color: #777;
        border-color: #333;
    }

    /* QDialogButtonBox QPushButton */
    QDialogButtonBox QPushButton {
        min-width: 80px;
    }
)";
    qApp->setStyleSheet(style);

    H1ModTools w;
    w.show();
    return a.exec();
}
