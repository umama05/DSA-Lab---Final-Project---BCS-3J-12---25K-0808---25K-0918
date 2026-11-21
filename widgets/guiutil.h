//  guiutil.h  -  Small helpers shared by all GUI classes:

#pragma once

#include <QColor>
#include <QPixmap>
#include <QPointF>
#include <QSize>
#include <QString>
#include <string>

#include "book.h"

class QPainter;
class QWidget;

inline QString qstr(const std::string& s) { return QString::fromStdString(s); }
inline std::string stdstr(const QString& s) { return s.toStdString(); }

namespace Theme {
    inline QColor accent() { return QColor("#9A5DB9"); }      // lavender purple
    inline QColor accentDark() { return QColor("#8449A3"); }
    inline QColor accentSoft() { return QColor("#F1E7F8"); }  // pale lavender tint (selected card)
    inline QColor warm() { return QColor("#E8672B"); }        // small warm spark: reading progress
    inline QColor ink() { return QColor("#2E2438"); }
    inline QColor muted() { return QColor("#857A94"); }
    inline QColor border() { return QColor("#E1D6EA"); }
    inline QColor card() { return QColor("#FFFFFF"); }
    inline QColor cream() { return QColor("#FBF7EE"); }
    inline QColor gold() { return QColor("#E5A23B"); }
    inline QColor starOff() { return QColor("#E4DBEA"); }
    QColor shelfColor(Shelf s);
}  // namespace Theme

QColor genreColor(const QString& genre);

QPixmap makeCover(const Book& book, const QSize& size, qreal dpr = 1.0);

void paintStars(QPainter* p, const QPointF& topLeft, int filled, int count, qreal starSize, qreal gap,
    const QColor& on, const QColor& off);


void repolish(QWidget* root);


