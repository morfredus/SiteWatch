/*
 * SiteWatch
 * Copyright (C) 2026 morfredus
 * SPDX-License-Identifier: GPL-3.0-only
 */

#include <QApplication>
#include <QIcon>
#include "ui/MainWindow.h"

// -----------------------------------------------------------------------------
// Feuille de style globale : aspect sobre facon Windows 11.
// Centralisee ici pour que toute l'interface partage la meme identite visuelle.
// -----------------------------------------------------------------------------
static const char* kStyle = R"(
QMainWindow, QWidget#central { background: #f5f6f8; }
QWidget { font-family: 'Segoe UI'; font-size: 12px; color: #1f2328; }

QFrame#card, QFrame#panel {
    background: #ffffff; border: 1px solid #e6e8eb; border-radius: 10px;
}
QLabel#cardTitle { color: #6b7280; font-size: 11px; }
QLabel#cardValue { font-size: 20px; font-weight: 700; color: #111827; }

QPushButton {
    background: #0f6cbd; color: #ffffff; border: none;
    border-radius: 6px; padding: 6px 16px; font-weight: 600;
}
QPushButton:hover  { background: #1479cf; }
QPushButton:pressed { background: #0c5aa0; }

QComboBox, QDateEdit {
    background: #ffffff; border: 1px solid #d0d5dd;
    border-radius: 6px; padding: 4px 8px; min-height: 20px;
}

QTabWidget::pane {
    border: 1px solid #e6e8eb; border-radius: 10px; background: #ffffff; top: -1px;
}
QTabBar::tab {
    background: transparent; padding: 8px 16px; margin-right: 4px;
    color: #6b7280; border: none;
}
QTabBar::tab:selected { color: #0f6cbd; border-bottom: 2px solid #0f6cbd; font-weight: 600; }
QTabBar::tab:hover { color: #0f6cbd; }

QTreeWidget, QTableWidget { background: #ffffff; border: none; outline: none; }
QTreeWidget::item, QTableWidget::item { padding: 4px; }
QTreeWidget::item:selected, QTableWidget::item:selected { background: #e8f1fb; color: #111827; }
QHeaderView::section {
    background: #ffffff; border: none; border-bottom: 1px solid #e6e8eb;
    padding: 6px; color: #6b7280; font-weight: 600;
}

QStatusBar { background: #ffffff; border-top: 1px solid #e6e8eb; color: #6b7280; }

/* Popup du calendrier (QDateEdit) : bandeau bleu lisible, grille claire. */
QCalendarWidget QWidget { background: #ffffff; }
QCalendarWidget QWidget#qt_calendar_navigationbar {
    background: #0f6cbd; min-height: 30px;
}
QCalendarWidget QToolButton {
    color: #ffffff; background: transparent; border: none;
    font-weight: 600; padding: 4px 8px; margin: 2px;
}
QCalendarWidget QToolButton:hover { background: #1479cf; border-radius: 4px; }
QCalendarWidget QToolButton::menu-indicator { image: none; }
QCalendarWidget QMenu { background: #ffffff; color: #1f2328; }
QCalendarWidget QSpinBox { background: #ffffff; color: #1f2328; }
QCalendarWidget QAbstractItemView:enabled {
    color: #1f2328; background: #ffffff; outline: none;
    selection-background-color: #0f6cbd; selection-color: #ffffff;
}
QCalendarWidget QAbstractItemView:disabled { color: #c0c4cc; }
)";

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("SiteWatch");
    app.setWindowIcon(QIcon(":/app.ico"));
    app.setStyleSheet(kStyle);

    MainWindow window;
    window.show();
    return app.exec();
}
