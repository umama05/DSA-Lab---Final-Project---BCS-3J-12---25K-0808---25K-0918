//  barchart.h  -  a tiny custom painted horizontal bar chart 

#pragma once

#include <QColor>
#include <QString>
#include <QVector>
#include <QWidget>

struct BarItem {
    QString label;
    double value = 0;
    QColor color;
};

class BarChart : public QWidget {
    Q_OBJECT
public:
    explicit BarChart(QWidget* parent = nullptr);

    void setItems(const QVector<BarItem>& items);
    void setEmptyText(const QString& text);

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    QVector<BarItem> items_;
    QString emptyText_;
};
