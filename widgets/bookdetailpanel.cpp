#include "bookdetailpanel.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QSignalBlocker>
#include <QStandardPaths>
#include <QTextCursor>

#include "coverstore.h"
#include "guiutil.h"
#include "library.h"
#include "textutil.h"
#include "ui_bookdetailpanel.h"

BookDetailPanel::BookDetailPanel(QWidget* parent) : QWidget(parent), ui(new Ui::BookDetailPanel) {
     ui->setupUi(this);
     repolish(this);

     ui->shelfCombo->addItem(tr("Not on a shelf"), static_cast<int>(Shelf::None));
     ui->shelfCombo->addItem(tr("Want to Read"), static_cast<int>(Shelf::WantToRead));
     ui->shelfCombo->addItem(tr("Currently Reading"), static_cast<int>(Shelf::CurrentlyReading));
     ui->shelfCombo->addItem(tr("Read"), static_cast<int>(Shelf::Read));

     ui->saveButton->setShortcut(QKeySequence::Save);
     ui->saveButton->setToolTip(tr("Save (Ctrl+S)"));

     connect(ui->shelfCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &BookDetailPanel::onShelfChanged);
     connect(ui->pageSlider, &QSlider::valueChanged, this, &BookDetailPanel::onSliderMoved);
     connect(ui->pageSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &BookDetailPanel::onSpinChanged);
     connect(ui->starRating, &StarRating::ratingChanged, this, &BookDetailPanel::updateSaveState);
     connect(ui->reviewEdit, &QPlainTextEdit::textChanged, this, &BookDetailPanel::onReviewTextChanged);
     connect(ui->saveButton, &QPushButton::clicked, this, [this] { commit(); });
     connect(ui->removeButton, &QPushButton::clicked, this, &BookDetailPanel::onRemove);
     connect(ui->changeCoverButton, &QPushButton::clicked, this, &BookDetailPanel::onChangeCover);
     connect(ui->removeCoverButton, &QPushButton::clicked, this, &BookDetailPanel::onRemoveCover);

     ui->titleBlock->setAlignment(ui->genreChip, Qt::AlignLeft);
     showBook(0);
}

BookDetailPanel::~BookDetailPanel() { delete ui; }

void BookDetailPanel::setLibrary(Library* library) { library_ = library; }

const Book* BookDetailPanel::book() const {
     return (library_ && bookId_ > 0) ? library_->findBook(bookId_) : nullptr;
}

Shelf BookDetailPanel::selectedShelf() const {
     return shelfFromInt(ui->shelfCombo->currentData().toInt());
}

void BookDetailPanel::showBook(int bookId) {
     if (bookId == bookId_ && bookId_ != 0 && isDirty()) return;
     reload();
}

void BookDetailPanel::reload() {
     const Book* b = book();
     if (!b) {
          bookId_ = 0;
          ui->detailStack->setCurrentIndex(0);
          return;
     }

     loading_ = true;
     {
          const QSignalBlocker b1(ui->shelfCombo), b2(ui->pageSlider), b3(ui->pageSpin), b4(ui->starRating),
               b5(ui->reviewEdit);

          updateCover();
          ui->titleLabel->setText(qstr(b->title));
          ui->authorLabel->setText(tr("by %1").arg(qstr(b->author)));
          const QColor gc = genreColor(qstr(b->genre));
          ui->genreChip->setText(qstr(b->genre));
          ui->genreChip->setStyleSheet(QStringLiteral("QLabel { color: %1; background: rgba(%2,%3,%4,38); }")
               .arg(gc.name())
               .arg(gc.red())
               .arg(gc.green())
               .arg(gc.blue()));
          ui->metaLabel->setText(tr("%1  ·  %2 pages").arg(b->year).arg(b->pages));
          ui->descriptionView->setPlainText(b->description.empty() ? tr("No description yet.") : qstr(b->description));

          ui->shelfCombo->setCurrentIndex(ui->shelfCombo->findData(static_cast<int>(b->shelf)));
          ui->pageSlider->setRange(0, b->pages);
          ui->pageSpin->setRange(0, b->pages);
          ui->pageSlider->setValue(b->currentPage);
          ui->pageSpin->setValue(b->currentPage);

          ui->starRating->setRating(b->review.rating);
          ui->reviewEdit->setPlainText(qstr(b->review.text));
     }
     loading_ = false;

     ui->progressBox->setVisible(selectedShelf() == Shelf::CurrentlyReading);
     updateProgressText();
     onReviewTextChanged();
     updateSaveState();
     ui->detailStack->setCurrentIndex(1);
}

bool BookDetailPanel::isDirty() const {
     const Book* b = book();
     if (!b) return false;
     const Shelf s = selectedShelf();
     if (s != b->shelf) return true;
     if (s == Shelf::CurrentlyReading && ui->pageSpin->value() != b->currentPage) return true;
     if (ui->starRating->rating() != b->review.rating) return true;
     return text::trim(stdstr(ui->reviewEdit->toPlainText())) != b->review.text;
}

void BookDetailPanel::updateSaveState() {
     if (loading_) return;
     ui->saveButton->setEnabled(isDirty());
}

void BookDetailPanel::onShelfChanged() {
     if (loading_) return;
     const Book* b = book();
     const Shelf s = selectedShelf();
     if (b && s == Shelf::CurrentlyReading && ui->pageSpin->value() >= b->pages) {
          const QSignalBlocker b1(ui->pageSlider), b2(ui->pageSpin);
          ui->pageSlider->setValue(0);
          ui->pageSpin->setValue(0);
     }
     ui->progressBox->setVisible(s == Shelf::CurrentlyReading);
     updateProgressText();
     updateSaveState();
}

void BookDetailPanel::onSliderMoved(int value) {
     if (loading_) return;
     {
          const QSignalBlocker blocker(ui->pageSpin);
          ui->pageSpin->setValue(value);
     }
     updateProgressText();
     updateSaveState();
}

void BookDetailPanel::onSpinChanged(int value) {
     if (loading_) return;
     {
          const QSignalBlocker blocker(ui->pageSlider);
          ui->pageSlider->setValue(value);
     }
     updateProgressText();
     updateSaveState();
}

void BookDetailPanel::updateProgressText() {
     const Book* b = book();
     if (!b || b->pages <= 0) return;
     const int page = ui->pageSpin->value();
     QString text = tr("Page %1 of %2  ·  %3%").arg(page).arg(b->pages).arg(qRound(100.0 * page / b->pages));
     if (page >= b->pages) text += tr("  ·  saving will mark it as Read");
     ui->progressLabel->setText(text);
}

void BookDetailPanel::onReviewTextChanged() {
     if (loading_) return;
     QString t = ui->reviewEdit->toPlainText();
     if (t.size() > kMaxReviewChars) {
          const QSignalBlocker blocker(ui->reviewEdit);
          ui->reviewEdit->setPlainText(t.left(kMaxReviewChars));
          ui->reviewEdit->moveCursor(QTextCursor::End);
          t = ui->reviewEdit->toPlainText();
     }
     ui->reviewCountLabel->setText(QStringLiteral("%1 / %2").arg(t.size()).arg(kMaxReviewChars));
     updateSaveState();
}

bool BookDetailPanel::commit() {
     const Book* b = book();
     if (!b || !isDirty()) return true;
     std::string label;
     const bool changed = library_->updateUserData(bookId_, selectedShelf(), ui->pageSpin->value(),
          ui->starRating->rating(), stdstr(ui->reviewEdit->toPlainText()),
          &label);
     if (changed) emit libraryChanged(qstr(label));
     else reload();
     return true;
}

void BookDetailPanel::updateCover() {
     const Book* b = book();
     if (!b) return;
     ui->coverLabel->setPixmap(makeCover(*b, QSize(96, 138), devicePixelRatioF()));
     const bool hasPicture = CoverStore::hasCover(b->id);
     ui->changeCoverButton->setText(hasPicture ? tr("Change picture") : tr("Add picture"));
     ui->removeCoverButton->setVisible(hasPicture);
}

void BookDetailPanel::onChangeCover() {
     const Book* b = book();
     if (!b) return;
     const QString file = QFileDialog::getOpenFileName(
          this, tr("Choose a cover picture for \"%1\"").arg(qstr(b->title)),
          QStandardPaths::writableLocation(QStandardPaths::PicturesLocation), CoverStore::fileDialogFilter());
     if (file.isEmpty()) return;

     QString error;
     if (!CoverStore::importCover(b->id, file, &error)) {
          QMessageBox::warning(this, tr("Cover picture"), error);
          return;
     }
     updateCover();
     emit coverChanged(b->id);
}

void BookDetailPanel::onRemoveCover() {
     const Book* b = book();
     if (!b) return;
     const int id = b->id;
     CoverStore::removeCover(id);
     updateCover();
     emit coverChanged(id);
}

void BookDetailPanel::onRemove() {
     const Book* b = book();
     if (!b) return;
     const auto answer = QMessageBox::question(
          this, tr("Remove book"),
          tr("Remove \"%1\" from the catalog?\n\nIt will also leave your shelves, and your rating and review for it "
               "will be deleted. Undo history will be cleared.")
          .arg(qstr(b->title)),
          QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel);
     if (answer != QMessageBox::Yes) return;

     const int id = bookId_;
     if (library_->removeBook(id)) {
          CoverStore::removeCover(id);
          bookId_ = 0;
          reload();
          emit bookRemoved(id);
     }
}



