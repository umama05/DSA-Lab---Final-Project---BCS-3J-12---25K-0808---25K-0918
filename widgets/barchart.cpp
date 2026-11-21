#include "barchart.h"
#include "guiutil.h"

#include <QFontMetrics>
#include <QPainter>
#include <QPainterPath>

namespace {
constexpr int kRowHeight = 34;
constexpr int kBarHeight = 16;
constexpr int kPad = 6;
}  // namespace

BarChart::BarChart(QWidget* parent) : QWidget(parent) {
    emptyText_ = tr("Nothing to show yet");
}

void BarChart::setItems(const QVector<BarItem>& items) {
    items_ = items;
    updateGeometry();
    update();
}

void BarChart::setEmptyText(const QString& text) {
    emptyText_ = text;
    update();
}

QSize BarChart::sizeHint() const { return QSize(320, qMax(120, items_.size() * kRowHeight + 2 * kPad)); }
QSize BarChart::minimumSizeHint() const { return QSize(220, qMax(120, items_.size() * kRowHeight + 2 * kPad)); }

void BarChart::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.setRenderHint(QPainter::TextAntialiasing);

    double maxValue = 0;
    for (const BarItem& it : items_) maxValue = qMax(maxValue, it.value);

    if (items_.isEmpty() || maxValue <= 0) {
        p.setPen(Theme::muted());
        p.drawText(rect().adjusted(16, 0, -16, 0), Qt::AlignCenter | Qt::TextWordWrap, emptyText_);
        return;
    }

    QFont labelFont = font();
    labelFont.setPointSizeF(9.5);
    p.setFont(labelFont);
    const QFontMetrics fm(labelFont);

    int labelW = 0;
    for (const BarItem& it : items_) labelW = qMax(labelW, fm.horizontalAdvance(it.label));
    labelW = qMin(labelW, 150) + 12;
    const int valueW = 34;

    const QRectF track(labelW, 0, qMax(20, width() - labelW - valueW), kBarHeight);

    for (int i = 0; i < items_.size(); ++i) {
        const BarItem& it = items_[i];
        const qreal y = kPad + i * kRowHeight;

        p.setPen(Theme::ink());
        p.drawText(QRectF(0, y - 2, labelW - 10, kBarHeight + 4), Qt::AlignRight | Qt::AlignVCenter,
                   fm.elidedText(it.label, Qt::ElideRight, 150));

        QRectF bg = track.translated(0, y);
        QPainterPath bgPath;
        bgPath.addRoundedRect(bg, kBarHeight / 2.0, kBarHeight / 2.0);
        p.fillPath(bgPath, QColor("#EDE5F3"));

        if (it.value > 0) {
            const qreal w = qMax<qreal>(kBarHeight, bg.width() * it.value / maxValue);
            QRectF fill(bg.left(), bg.top(), w, bg.height());
            QPainterPath fillPath;
            fillPath.addRoundedRect(fill, kBarHeight / 2.0, kBarHeight / 2.0);
            p.fillPath(fillPath, it.color.isValid() ? it.color : Theme::accent());
        }

        p.setPen(Theme::muted());
        p.drawText(QRectF(bg.right() + 8, y - 2, valueW, kBarHeight + 4), Qt::AlignLeft | Qt::AlignVCenter,
                   QString::number(it.value, 'f', it.value == qRound(it.value) ? 0 : 1));
    }
}
