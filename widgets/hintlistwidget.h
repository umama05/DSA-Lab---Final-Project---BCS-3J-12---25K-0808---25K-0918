//  hintlistwidget.h  -  A QListWidget that shows a friendly centered message when it has no items 

#pragma once

#include <QListWidget>

class HintListWidget : public QListWidget {
    Q_OBJECT
public:
    explicit HintListWidget(QWidget* parent = nullptr);

    void setHint(const QString& hint);
    QString hint() const { return hint_; }

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    QString hint_;
};
