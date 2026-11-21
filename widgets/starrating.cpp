#include "starrating.h"

#include <QMouseEvent>
#include <QPainter>

#include "guiutil.h"

StarRating::StarRating(QWidget* parent) : QWidget(parent) {
    setMouseTracking(true);
    setCursor(Qt::PointingHandCursor);
    setToolTip(tr("Click a star to rate. Click the same star again to clear."));
}

void StarRating::setRating(int rating) {
    rating = qBound(0, rating, 5);
    if (rating == rating_) return;
    rating_ = rating;
    update();
    emit ratingChanged(rating_);
}

QSize StarRating::sizeHint() const { return QSize(5 * kStar + 4 * kGap, kStar + 4); }

int StarRating::starAt(const QPoint& pos) const {
    if (pos.y() < 0 || pos.y() > height()) return 0;
    const int idx = pos.x() / (kStar + kGap);
    return (idx >= 0 && idx < 5 && pos.x() >= 0) ? idx + 1 : 0;
}

void StarRating::paintEvent(QPaintEvent*) {
    QPainter p(this);
    const int shown = hover_ > 0 ? hover_ : rating_;
    QColor on = Theme::gold();
    if (hover_ > 0 && hover_ != rating_) on = on.lighter(112);
    paintStars(&p, QPointF(0, 2), shown, 5, kStar, kGap, on, Theme::starOff());
}

void StarRating::mouseMoveEvent(QMouseEvent* e) {
    const int s = starAt(e->pos());
    if (s != hover_) {
        hover_ = s;
        update();
    }
}

void StarRating::mousePressEvent(QMouseEvent* e) {
    if (e->button() != Qt::LeftButton) return;
    const int s = starAt(e->pos());
    if (s == 0) return;
    setRating(s == rating_ ? 0 : s);
}

void StarRating::leaveEvent(QEvent*) {
    hover_ = 0;
    update();
}
