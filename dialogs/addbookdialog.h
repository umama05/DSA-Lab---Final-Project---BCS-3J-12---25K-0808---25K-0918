//  addbookdialog.h  -  small form for adding a new book to the catalog.

#pragma once

#include <QDialog>
#include <QStringList>

#include "book.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class AddBookDialog;
}
class QLabel;
class QPushButton;
QT_END_NAMESPACE

class AddBookDialog : public QDialog {
    Q_OBJECT
public:
    explicit AddBookDialog(const QStringList& existingGenres, QWidget* parent = nullptr);
    ~AddBookDialog() override;

    Book book() const; 
    void setError(const QString& message);
    QString coverSource() const { return coverSource_; }

private slots:
    void onChooseCover();
    void updateCoverPreview();

private:
    Ui::AddBookDialog* ui;
    QString coverSource_;
    QLabel* coverPreview_ = nullptr;
    QPushButton* chooseCoverButton_ = nullptr;
    QPushButton* clearCoverButton_ = nullptr;
};
