//  coverstore.h  -  optional real cover pictures for books.

#pragma once

#include <QPixmap>
#include <QSize>
#include <QString>

namespace CoverStore {

	void setFolder(const QString& folder);
	QString folder();

	bool rescan();

	bool hasCover(int bookId);

	bool importCover(int bookId, const QString& sourceFile, QString* error = nullptr);

	void removeCover(int bookId);

	QPixmap coverPixmap(int bookId, const QSize& size, qreal dpr = 1.0);

	QPixmap previewFromFile(const QString& file, const QSize& size, qreal dpr = 1.0, QString* error = nullptr);

	QString fileDialogFilter();

}  // namespace CoverStore



