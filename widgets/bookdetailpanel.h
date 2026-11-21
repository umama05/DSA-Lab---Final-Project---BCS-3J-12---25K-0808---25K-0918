//  bookdetailpanel.h  -  panel that shows one book and lets the 
//  user shelve it, track progress, rate it and write a review.

#pragma once

#include <QWidget>

#include "book.h"

class Library;

QT_BEGIN_NAMESPACE
namespace Ui {
	class BookDetailPanel;
}
QT_END_NAMESPACE

class BookDetailPanel : public QWidget {
	Q_OBJECT
public:
	explicit BookDetailPanel(QWidget* parent = nullptr);
	~BookDetailPanel() override;

	void setLibrary(Library* library);

	void showBook(int bookId);
	void reload();
	void updateCover();

	int bookId() const { return bookId_; }
	bool isDirty() const;
	bool commit();

signals:
	void libraryChanged(const QString& message);
	void bookRemoved(int bookId);
	void coverChanged(int bookId);

private slots:
	void onShelfChanged();
	void onSliderMoved(int value);
	void onSpinChanged(int value);
	void onReviewTextChanged();
	void updateProgressText();
	void updateSaveState();
	void onRemove();
	void onChangeCover();
	void onRemoveCover();

private:
	const Book* book() const;
	Shelf selectedShelf() const;

	Ui::BookDetailPanel* ui;
	Library* library_ = nullptr;
	int bookId_ = 0;
	bool loading_ = false;

	static constexpr int kMaxReviewChars = 600;
};
