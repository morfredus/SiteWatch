/*
 * SiteWatch
 * Copyright (C) 2026 morfredus
 * SPDX-License-Identifier: GPL-3.0-only
 */

#include "ui/Icons.h"

#include <QFontDatabase>
#include <QFont>
#include <QPixmap>
#include <QIcon>
#include <QPainter>
#include <QColor>
#include <QGuiApplication>

namespace icons {
namespace {

// Codepoints Font Awesome 6 Free Solid des glyphes embarqués (cf. le
// sous-ensemble resources/fonts/SiteWatchIcons.ttf).
char32_t codepoint(Glyph g) {
    switch (g) {
        case Glyph::Chart:       return 0xe0e3;  // chart-column
        case Glyph::User:        return 0xf007;  // user
        case Glyph::Robot:       return 0xf544;  // robot
        case Glyph::Warn:        return 0xf071;  // triangle-exclamation
        case Glyph::Shield:      return 0xf3ed;  // shield-halved
        case Glyph::Ban:         return 0xf05e;  // ban
        case Glyph::Trophy:      return 0xf091;  // trophy
        case Glyph::CrossCircle: return 0xf057;  // circle-xmark
        case Glyph::TrendUp:     return 0xe098;  // arrow-trend-up
        case Glyph::Heart:       return 0xf004;  // heart
        case Glyph::Check:       return 0xf00c;  // check
        case Glyph::Cross:       return 0xf00d;  // xmark
        case Glyph::CircleCheck: return 0xf058;  // circle-check
        case Glyph::CircleInfo:  return 0xf05a;  // circle-info
        case Glyph::Eye:         return 0xf06e;  // eye
        case Glyph::Dot:         return 0xf111;  // circle
    }
    return 0xf111;
}

QString g_family;

} // namespace

QString family() {
    if (g_family.isEmpty()) {
        const int id = QFontDatabase::addApplicationFont(":/fonts/SiteWatchIcons.ttf");
        if (id >= 0) {
            const QStringList fams = QFontDatabase::applicationFontFamilies(id);
            if (!fams.isEmpty()) g_family = fams.first();
        }
        if (g_family.isEmpty()) g_family = QStringLiteral("SiteWatch Icons");
    }
    return g_family;
}

QFont font(int px) {
    QFont f(family());
    f.setPixelSize(px);
    return f;
}

QChar ch(Glyph g) {
    // Tous les glyphes utilisés sont dans le plan de base (PUA U+E000–U+F8FF).
    return QChar(static_cast<char16_t>(codepoint(g)));
}

QString span(Glyph g, const QString& colorHex, int px) {
    return QString("<span style=\"font-family:'%1';font-size:%2px;color:%3\">%4</span>")
        .arg(family())
        .arg(px)
        .arg(colorHex)
        .arg(QString(ch(g)));
}

QPixmap pixmap(Glyph g, const QColor& color, int px) {
    const qreal dpr = qApp ? qApp->devicePixelRatio() : 1.0;
    QPixmap pm(QSize(px, px) * dpr);
    pm.setDevicePixelRatio(dpr);
    pm.fill(Qt::transparent);

    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setRenderHint(QPainter::TextAntialiasing, true);
    QFont f = font(static_cast<int>(px * 0.92));
    p.setFont(f);
    p.setPen(color);
    p.drawText(QRectF(0, 0, px, px), Qt::AlignCenter, QString(ch(g)));
    p.end();
    return pm;
}

QIcon icon(Glyph g, const QColor& color, int px) {
    return QIcon(pixmap(g, color, px));
}

} // namespace icons
