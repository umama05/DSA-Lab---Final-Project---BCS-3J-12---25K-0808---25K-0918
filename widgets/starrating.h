//  starrating.h  -  clickable 5-star rating widget

#pragma once

#include <QWidget>

class StarRating : public QWidget {
    Q_OBJECT
public:
    explicit StarRating(QWidget* parent = nullptr);

    int rating() const { return rating_; }
    void setRating(int rating);  

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override { return sizeHint(); }

signals:
    void ratingChanged(int rating);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void leaveEvent(QEvent* event) override;

private:
    int starAt(const QPoint& pos) const;  // 1..5, or 0 if outside

    static constexpr int kStar = 26;
    static constexpr int kGap = 5;

    int rating_ = 0;
    int hover_ = 0;
};
