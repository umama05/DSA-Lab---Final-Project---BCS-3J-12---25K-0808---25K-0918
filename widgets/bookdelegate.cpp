#include "bookdelegate.h"

#include <QFontMetrics>
#include <QPainter>
#include <QPainterPath>

#include "guiutil.h"

namespace {
    constexpr int kRowHeight = 100;
    constexpr int kTallRowHeight = 128;

    void drawPill(QPainter* p, const QRectF& rect, const QString& text, const QColor& color, const QFont& font,
        bool filled) {
        p->save();
        p->setRenderHint(QPainter::Antialiasing);
        p->setPen(Qt::NoPen);
        p->setBrush(filled ? color : QColor(color.red(), color.green(), color.blue(), 30));
        p->drawRoundedRect(rect, rect.height() / 2, rect.height() / 2);
        p->setFont(font);
        p->setPen(filled ? QColor("#FFFFFF") : color);
        p->drawText(rect, Qt::AlignCenter, text);
        p->restore();
    }
}  // namespace

BookDelegate::BookDelegate(const Library* library, bool tall, QObject* parent)
    : QStyledItemDelegate(parent), library_(library), tall_(tall) {
}

QSize BookDelegate::sizeHint(const QStyleOptionViewItem&, const QModelIndex&) const {
    return QSize(100, tall_ ? kTallRowHeight : kRowHeight);
}

void BookDelegate::paint(QPainter* p, const QStyleOptionViewItem& opt, const QModelIndex& index) const {
    const Book* b = library_->findBook(index.data(BookIdRole).toInt());
    if (!b) return;

    p->save();
    p->setRenderHint(QPainter::Antialiasing);
    p->setRenderHint(QPainter::TextAntialiasing);

    const bool selected = opt.state & QStyle::State_Selected;
    const bool hover = opt.state & QStyle::State_MouseOver;

    const QRectF card = QRectF(opt.rect).adjusted(2, 4, -2, -4);
    p->setPen(QPen(selected ? Theme::accent() : Theme::border(), selected ? 1.6 : 1.0));
    p->setBrush(selected ? Theme::accentSoft() : (hover ? QColor("#FFFCF7") : Theme::card()));
    p->drawRoundedRect(card, 14, 14);

    const QSize coverSize(54, 78);
    const QPointF coverPos(card.left() + 14, card.top() + 7);
    p->drawPixmap(coverPos, makeCover(*b, coverSize, p->device()->devicePixelRatioF()));

    const qreal rightEdge = card.right() - 16;
    QFont pillFont = opt.font;
    pillFont.setPointSizeF(8.5);
    pillFont.setBold(true);
    const QFontMetrics pillFm(pillFont);

    qreal rightColumnW = 0;
    const int match = index.data(MatchRole).isValid() ? index.data(MatchRole).toInt() : -1;

    if (match >= 0) {
        const QString t = tr("%1% match").arg(match);
        const qreal w = pillFm.horizontalAdvance(t) + 22;
        drawPill(p, QRectF(rightEdge - w, card.top() + 14, w, 22), t, Theme::accent(), pillFont, true);
        rightColumnW = w;
    }
    else {
        if (b->rating() > 0) {
            const qreal starSize = 14, gap = 2;
            const qreal w = 5 * starSize + 4 * gap;
            paintStars(p, QPointF(rightEdge - w, card.top() + 14), b->rating(), 5, starSize, gap, Theme::gold(),
                Theme::starOff());
            rightColumnW = w;
        }
        if (b->shelf != Shelf::None) {
            const QString t = b->shelf == Shelf::WantToRead ? tr("Want to Read")
                : b->shelf == Shelf::CurrentlyReading ? tr("Reading")
                : tr("Read");
            const qreal w = pillFm.horizontalAdvance(t) + 20;
            drawPill(p, QRectF(rightEdge - w, card.top() + 38, w, 20), t, Theme::shelfColor(b->shelf), pillFont,
                false);
            rightColumnW = qMax(rightColumnW, w);
        }
    }

    const qreal textLeft = coverPos.x() + coverSize.width() + 16;
    const qreal textRight = rightEdge - (rightColumnW > 0 ? rightColumnW + 14 : 0);
    const qreal textW = qMax<qreal>(40, textRight - textLeft);

    QFont titleFont(QStringLiteral("Georgia"));
    titleFont.setPointSizeF(11.5);
    titleFont.setBold(true);
    p->setFont(titleFont);
    p->setPen(Theme::ink());
    p->drawText(QRectF(textLeft, card.top() + 8, textW, 24), Qt::AlignLeft | Qt::AlignVCenter,
        QFontMetrics(titleFont).elidedText(qstr(b->title), Qt::ElideRight, qRound(textW)));

    QFont authorFont = opt.font;
    authorFont.setPointSizeF(9.5);
    p->setFont(authorFont);
    p->setPen(Theme::muted());
    p->drawText(QRectF(textLeft, card.top() + 31, textW, 18), Qt::AlignLeft | Qt::AlignVCenter,
        QFontMetrics(authorFont).elidedText(tr("by %1").arg(qstr(b->author)), Qt::ElideRight, qRound(textW)));

    QFont chipFont = opt.font;
    chipFont.setPointSizeF(8.5);
    chipFont.setBold(true);
    const QFontMetrics chipFm(chipFont);
    const QString genre = qstr(b->genre);
    const qreal chipW = qMin<qreal>(textW * 0.6, chipFm.horizontalAdvance(genre) + 20);
    drawPill(p, QRectF(textLeft, card.top() + 53, chipW, 20),
        chipFm.elidedText(genre, Qt::ElideRight, qRound(chipW) - 16), genreColor(genre), chipFont, false);

    p->setFont(authorFont);
    p->setPen(Theme::muted());
    p->drawText(QRectF(textLeft + chipW + 10, card.top() + 53, qMax<qreal>(0, textW - chipW - 10), 20),
        Qt::AlignLeft | Qt::AlignVCenter, tr("%1  ·  %2 pages").arg(b->year).arg(b->pages));

    const QString note = index.data(NoteRole).toString();
    const bool showNote = tall_ && !note.isEmpty();

    // reading progress bar (skipped when a note line takes its place)
    if (b->shelf == Shelf::CurrentlyReading && b->pages > 0 && !showNote) {
        const qreal frac = qBound<qreal>(0, qreal(b->currentPage) / b->pages, 1);
        const qreal barW = qMin<qreal>(textW - 120, 260);
        if (barW > 40) {
            const QRectF track(textLeft, card.top() + 79, barW, 6);
            p->setPen(Qt::NoPen);
            p->setBrush(QColor("#E9DFF1"));
            p->drawRoundedRect(track, 3, 3);
            p->setBrush(Theme::warm());
            p->drawRoundedRect(QRectF(track.left(), track.top(), qMax<qreal>(6, barW * frac), 6), 3, 3);
            QFont small = opt.font;
            small.setPointSizeF(8.5);
            p->setFont(small);
            p->setPen(Theme::muted());
            p->drawText(QRectF(track.right() + 10, card.top() + 73, 120, 18), Qt::AlignLeft | Qt::AlignVCenter,
                tr("%1 / %2  (%3%)").arg(b->currentPage).arg(b->pages).arg(qRound(frac * 100)));
        }
    }

    // optional note line (recommendation reason / review text)
    if (showNote) {
        QFont noteFont = opt.font;
        noteFont.setPointSizeF(9);
        noteFont.setItalic(true);
        p->setFont(noteFont);
        p->setPen(Theme::accentDark());
        const QRectF noteRect(textLeft, card.top() + 77, card.right() - 16 - textLeft, card.height() - 77 - 8);
        p->setClipRect(noteRect);
        p->drawText(noteRect, Qt::AlignLeft | Qt::AlignTop | Qt::TextWordWrap, note);
    }

    p->restore();
}


