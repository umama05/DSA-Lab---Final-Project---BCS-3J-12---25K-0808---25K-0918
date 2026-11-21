#include "hintlistwidget.h"
#include <QPainter>
#include "guiutil.h"

HintListWidget::HintListWidget(QWidget* parent) : QListWidget(parent) {}

void HintListWidget::setHint(const QString& hint) {
    hint_ = hint;
    viewport()->update();
}

void HintListWidget::paintEvent(QPaintEvent* event) {
    QListWidget::paintEvent(event);
    if (count() > 0 || hint_.isEmpty()) return;

    QPainter p(viewport());
    p.setRenderHint(QPainter::TextAntialiasing);
    QFont f = font();
    f.setPointSizeF(11);
    p.setFont(f);
    p.setPen(Theme::muted());
    p.drawText(viewport()->rect().adjusted(48, 0, -48, 0), Qt::AlignCenter | Qt::TextWordWrap, hint_);
}
