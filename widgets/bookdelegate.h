//  bookdelegate.h  -  Paints each book as a rounded "card" inside a list.

#pragma once

#include <QStyledItemDelegate>

#include "library.h"

class BookDelegate : public QStyledItemDelegate {
    Q_OBJECT
public:
    enum Role {
        BookIdRole = Qt::UserRole,
        NoteRole = Qt::UserRole + 1,
        MatchRole = Qt::UserRole + 2
    };

    BookDelegate(const Library* library, bool tall, QObject* parent = nullptr);

    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;

private:
    const Library* library_;
    bool tall_;
};


