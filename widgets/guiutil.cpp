#include "guiutil.h"
#include "coverstore.h"

#include <QStyle>
#include <QWidget>

#include <QFont>
#include <QLinearGradient>
#include <QPainter>
#include <QPainterPath>
#include <QPolygonF>
#include <QStringList>
#include <QtMath>

namespace Theme {
    QColor shelfColor(Shelf s) {
        switch (s) {
        case Shelf::WantToRead: return QColor("#7C8BC4");
        case Shelf::CurrentlyReading: return Theme::warm();
        case Shelf::Read: return QColor("#5E8C6A");
        default: return Theme::muted();
        }
    }
}  // namespace Theme

QColor genreColor(const QString& genre) {
    static const struct {
        const char* name;
        const char* color;
    } kKnown[] = {
        { "Fantasy", "#7B4B94" },{ "Science Fiction", "#2F6690" },{ "Dystopian", "#5C6B7A" },
        { "Mystery", "#3D6B6E" },{ "Thriller", "#8C2F39" },{ "Romance", "#C45B78" },
        { "Classics", "#8A5A2B" },{ "Historical Fiction", "#9A6A2F" },{ "Non-Fiction", "#3E7C59" },
        { "Self-Help", "#C27C1A" },{ "Horror", "#3B3B4F" },{ "Young Adult", "#2A9D8F" },
    };
    for (const auto& k : kKnown)
        if (genre.compare(QLatin1String(k.name), Qt::CaseInsensitive) == 0) return QColor(k.color);

    uint h = 5381;
    for (QChar c : genre) h = h * 33u + c.unicode();
    return QColor::fromHsl(static_cast<int>(h % 360u), 110, 100);
}

namespace {

    QString initialsOf(const QString& title) {
        static const QStringList kSkip = { "the", "a", "an", "of" };
        QStringList words = title.split(' ', Qt::SkipEmptyParts);
        QString out;
        for (const QString& w : words) {
            if (out.isEmpty() && kSkip.contains(w.toLower()) && words.size() > 1) continue;
            QChar first = QChar();
            for (QChar c : w)
                if (c.isLetterOrNumber()) {
                    first = c;
                    break;
                }
            if (first.isNull()) continue;
            out += first.toUpper();
            if (out.size() == 2) break;
        }
        return out.isEmpty() ? QStringLiteral("?") : out;
    }

    QPolygonF starPolygon(const QPointF& c, qreal outer) {
        static const qreal kPi = 3.14159265358979323846;
        QPolygonF poly;
        const qreal inner = outer * 0.46;
        for (int i = 0; i < 10; ++i) {
            const qreal r = (i % 2 == 0) ? outer : inner;
            const qreal a = -kPi / 2 + i * kPi / 5;
            poly << QPointF(c.x() + r * qCos(a), c.y() + r * qSin(a));
        }
        return poly;
    }

}  // namespace

QPixmap makeCover(const Book& book, const QSize& size, qreal dpr) {
    const QPixmap picture = CoverStore::coverPixmap(book.id, size, dpr);
    if (!picture.isNull()) return picture;

    QPixmap pm(QSize(qRound(size.width() * dpr), qRound(size.height() * dpr)));
    pm.setDevicePixelRatio(dpr);
    pm.fill(Qt::transparent);

    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing);
    p.setRenderHint(QPainter::TextAntialiasing);

    const QRectF r(0, 0, size.width(), size.height());
    const QColor base = genreColor(qstr(book.genre));
    const qreal radius = qMin(r.width(), r.height()) * 0.09;

    QPainterPath shape;
    shape.addRoundedRect(r, radius, radius);

    QLinearGradient grad(r.topLeft(), r.bottomRight());
    grad.setColorAt(0.0, base.lighter(122));
    grad.setColorAt(1.0, base.darker(130));
    p.fillPath(shape, grad);

    p.setClipPath(shape);
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(255, 255, 255, 30));
    p.drawEllipse(QPointF(r.width() * 0.72, r.height() * 0.26), r.width() * 0.42, r.width() * 0.42);
    p.fillRect(QRectF(0, 0, r.width() * 0.10, r.height()), QColor(0, 0, 0, 48));
    p.fillRect(QRectF(r.width() * 0.10, 0, 1.2, r.height()), QColor(255, 255, 255, 45));

    const bool big = size.height() >= 110;

    QFont f(QStringLiteral("Georgia"));
    f.setBold(true);
    f.setPixelSize(qMax(10, qRound(r.height() * (big ? 0.25 : 0.30))));
    p.setFont(f);
    p.setPen(QColor(255, 255, 255, 240));
    p.drawText(QRectF(r.width() * 0.10, r.height() * (big ? 0.10 : 0.0), r.width() * 0.90,
        r.height() * (big ? 0.46 : 1.0)),
        Qt::AlignCenter, initialsOf(qstr(book.title)));

    if (big) {
        QFont tf(QStringLiteral("Georgia"));
        tf.setPixelSize(qMax(8, qRound(r.height() * 0.085)));
        p.setFont(tf);
        p.setPen(QColor(255, 255, 255, 215));
        p.drawText(QRectF(r.width() * 0.17, r.height() * 0.58, r.width() * 0.75, r.height() * 0.36),
            Qt::AlignHCenter | Qt::AlignTop | Qt::TextWordWrap, qstr(book.title));
    }
    p.setClipping(false);

    p.setBrush(Qt::NoBrush);
    p.setPen(QPen(QColor(0, 0, 0, 40), 1));
    p.drawPath(shape);
    return pm;
}

void paintStars(QPainter* p, const QPointF& topLeft, int filled, int count, qreal starSize, qreal gap,
    const QColor& on, const QColor& off) {
    p->save();
    p->setRenderHint(QPainter::Antialiasing);
    p->setPen(Qt::NoPen);
    for (int i = 0; i < count; ++i) {
        const QPointF c(topLeft.x() + starSize / 2 + i * (starSize + gap), topLeft.y() + starSize / 2);
        p->setBrush(i < filled ? on : off);
        p->drawPolygon(starPolygon(c, starSize / 2));
    }
    p->restore();
}

void repolish(QWidget* root) {
    const auto children = root->findChildren<QWidget*>();
    for (QWidget* w : children) {
        w->style()->unpolish(w);
        w->style()->polish(w);
    }
}



