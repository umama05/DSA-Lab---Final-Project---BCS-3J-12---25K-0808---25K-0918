#include "mainwindow.h"

#include <QAbstractButton>
#include <QAction>
#include <QCloseEvent>
#include <QDir>
#include <QFile>
#include <QKeySequence>
#include <QListWidgetItem>
#include <QLocale>
#include <QMenu>
#include <QEvent>
#include <QMessageBox>
#include <QSaveFile>
#include <QScrollBar>
#include <QShortcut>
#include <QSignalBlocker>
#include <QStandardPaths>
#include <QTimer>

#include "addbookdialog.h"
#include "bookdelegate.h"
#include "bookdetailpanel.h"
#include "barchart.h"
#include "coverstore.h"
#include "guiutil.h"
#include "hintlistwidget.h"
#include "recommender.h"
#include "statistics.h"
#include "storage.h"
#include "ui_mainwindow.h"

namespace {

     std::string readFile(const QString& path) {
          QFile f(path);
          if (!f.open(QIODevice::ReadOnly)) return std::string();
          const QByteArray data = f.readAll();
          return std::string(data.constData(), static_cast<std::size_t>(data.size()));
     }

     bool writeFile(const QString& path, const std::string& text) {
          QSaveFile f(path);
          if (!f.open(QIODevice::WriteOnly)) return false;
          f.write(text.data(), static_cast<qint64>(text.size()));
          return f.commit();
     }

}  // namespace

MainWindow::MainWindow(const QString& dataDir, QWidget* parent) : QMainWindow(parent), ui(new Ui::MainWindow) {
     ui->setupUi(this);
     repolish(this);

     dataDir_ = dataDir;
     if (dataDir_.isEmpty()) dataDir_ = qEnvironmentVariable("BOOKVERSE_DATA_DIR");
     if (dataDir_.isEmpty()) dataDir_ = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
     if (dataDir_.isEmpty()) dataDir_ = QDir::homePath() + QStringLiteral("/BookVerseData");
     CoverStore::setFolder(QDir(dataDir_).filePath(QStringLiteral("covers")));  // optional cover pictures
     catalogPath_ = QDir(dataDir_).filePath(QStringLiteral("catalog.csv"));
     userPath_ = QDir(dataDir_).filePath(QStringLiteral("userdata.csv"));

     ui->detailPanel->setLibrary(&library_);
     setupControls();
     setupConnections();
     setupShortcuts();

     loadData();
     rebuildGenreCombo();
     updateOrderButton();
     refreshAll();
     setPage(0);

     resize(1400, 860);
     showStatus(tr("Loaded %1 books from %2").arg(library_.bookCount()).arg(QDir::toNativeSeparators(dataDir_)));
}

MainWindow::~MainWindow() { delete ui; }

void MainWindow::setupControls() {
     ui->sortCombo->addItem(tr("Title"), static_cast<int>(SortKey::Title));
     ui->sortCombo->addItem(tr("Author"), static_cast<int>(SortKey::Author));
     ui->sortCombo->addItem(tr("Rating"), static_cast<int>(SortKey::Rating));
     ui->sortCombo->addItem(tr("Year"), static_cast<int>(SortKey::Year));

     ui->algoCombo->addItem(tr("Merge sort"), static_cast<int>(SortAlgorithm::MergeSort));
     ui->algoCombo->addItem(tr("Insertion sort"), static_cast<int>(SortAlgorithm::InsertionSort));

     auto* normal = new BookDelegate(&library_, false, this);
     auto* tall = new BookDelegate(&library_, true, this);
     for (HintListWidget* l : { ui->catalogList, ui->wantList, ui->readingList, ui->readList }) l->setItemDelegate(normal);
     for (HintListWidget* l : { ui->reviewsList, ui->recoList }) l->setItemDelegate(tall);

     ui->shelfTabs->setDocumentMode(true);
     ui->loadDemoButton->setVisible(false);
     ui->searchEdit->setClearButtonEnabled(true);
}

void MainWindow::setupConnections() {
     connect(ui->navCatalog, &QPushButton::clicked, this, [this] { setPage(0); });
     connect(ui->navShelves, &QPushButton::clicked, this, [this] { setPage(1); });
     connect(ui->navForYou, &QPushButton::clicked, this, [this] { setPage(2); });
     connect(ui->navStats, &QPushButton::clicked, this, [this] { setPage(3); });
     connect(ui->undoButton, &QPushButton::clicked, this, &MainWindow::onUndo);

     connect(ui->searchEdit, &QLineEdit::textChanged, this, [this] { refreshCatalog(false); });
     connect(ui->genreCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
          [this] { refreshCatalog(false); });
     connect(ui->sortCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this] {
          updateOrderButton();
          refreshCatalog(false);
          });
     connect(ui->orderButton, &QToolButton::toggled, this, [this] {
          updateOrderButton();
          refreshCatalog(false);
          });
     connect(ui->algoCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
          [this] { refreshCatalog(false); });
     connect(ui->addBookButton, &QPushButton::clicked, this, &MainWindow::onAddBook);
     connect(ui->loadDemoButton, &QPushButton::clicked, this, &MainWindow::onLoadDemo);

     connect(ui->detailPanel, &BookDetailPanel::libraryChanged, this, &MainWindow::onLibraryChanged);
     connect(ui->detailPanel, &BookDetailPanel::coverChanged, this, [this](int) {
          refreshCovers();
          showStatus(tr("Cover updated"));
          });
     connect(ui->detailPanel, &BookDetailPanel::bookRemoved, this,
          [this](int) { onLibraryChanged(tr("Book removed from the catalog")); });

     for (HintListWidget* list : allLists()) {
          connect(list, &QListWidget::currentItemChanged, this, [this](QListWidgetItem* current, QListWidgetItem*) {
               if (!current) return;
               const int id = current->data(BookDelegate::BookIdRole).toInt();
               QMetaObject::invokeMethod(this, [this, id] { openBook(id); }, Qt::QueuedConnection);
               });
          connect(list, &QWidget::customContextMenuRequested, this,
               [this, list](const QPoint& pos) { onListContextMenu(list, pos); });
     }
}

void MainWindow::setupShortcuts() {
     auto shortcut = [this](const QKeySequence& keys, std::function<void()> fn) {
          auto* s = new QShortcut(keys, this);
          connect(s, &QShortcut::activated, this, std::move(fn));
          };
     shortcut(QKeySequence(QStringLiteral("Ctrl+1")), [this] { setPage(0); });
     shortcut(QKeySequence(QStringLiteral("Ctrl+2")), [this] { setPage(1); });
     shortcut(QKeySequence(QStringLiteral("Ctrl+3")), [this] { setPage(2); });
     shortcut(QKeySequence(QStringLiteral("Ctrl+4")), [this] { setPage(3); });
     shortcut(QKeySequence(QStringLiteral("Ctrl+N")), [this] { onAddBook(); });
     shortcut(QKeySequence(QStringLiteral("Ctrl+F")), [this] {
          setPage(0);
          ui->searchEdit->setFocus();
          ui->searchEdit->selectAll();
          });
     shortcut(QKeySequence(QStringLiteral("Ctrl+Z")), [this] { onUndo(); });
}


void MainWindow::loadData() {
     QDir().mkpath(dataDir_);

     bool seeded = false;
     LoadReport report;
     if (QFile::exists(catalogPath_)) report = loadCatalogText(readFile(catalogPath_), library_);

     if (library_.bookCount() == 0) {
          QFile damaged(catalogPath_);
          if (damaged.exists() && damaged.size() > 0) QFile::copy(catalogPath_, catalogPath_ + QStringLiteral(".bak"));
          QFile seed(QStringLiteral(":/data/books.csv"));
          if (seed.open(QIODevice::ReadOnly)) {
               const QByteArray data = seed.readAll();
               loadCatalogText(std::string(data.constData(), static_cast<std::size_t>(data.size())), library_);
          }
          seeded = true;
     }
     else if (report.skipped > 0) {
          QMessageBox::warning(this, tr("Some books could not be loaded"),
               tr("%1 row(s) in catalog.csv were skipped because they were incomplete or invalid.")
               .arg(report.skipped));
     }

     if (QFile::exists(userPath_)) loadUserDataText(readFile(userPath_), library_);
     if (seeded || !QFile::exists(userPath_)) saveAll();
}

bool MainWindow::saveAll() {
     QDir().mkpath(dataDir_);
     const bool ok = writeFile(catalogPath_, catalogToText(library_)) && writeFile(userPath_, userDataToText(library_));
     if (!ok && !saveWarned_) {
          saveWarned_ = true;
          QMessageBox::warning(this, tr("Could not save"),
               tr("BookVerse could not write to:\n%1\n\nYour changes are still visible but will be lost "
                    "when you close the app. Check that the folder is writable.")
               .arg(QDir::toNativeSeparators(dataDir_)));
     }
     return ok;
}


void MainWindow::setPage(int index) {
     index = qBound(0, index, ui->pages->count() - 1);
     ui->pages->setCurrentIndex(index);
     QAbstractButton* nav[] = { ui->navCatalog, ui->navShelves, ui->navForYou, ui->navStats };
     nav[index]->setChecked(true);
}

void MainWindow::refreshAll() {
     rebuildGenreCombo();
     refreshCatalog(true);
     refreshShelves();
     refreshRecommendations();
     refreshStats();
     updateUndoButton();
     ui->detailPanel->showBook(ui->detailPanel->bookId());
}

void MainWindow::rebuildGenreCombo() {
     const QString previous = ui->genreCombo->currentData().toString();
     QSignalBlocker blocker(ui->genreCombo);
     ui->genreCombo->clear();
     ui->genreCombo->addItem(tr("All genres"), QString());
     int select = 0;
     for (const std::string& g : library_.genres()) {
          ui->genreCombo->addItem(qstr(g), qstr(g));
          if (qstr(g) == previous) select = ui->genreCombo->count() - 1;
     }
     ui->genreCombo->setCurrentIndex(select);
}

void MainWindow::updateOrderButton() {
     const bool desc = ui->orderButton->isChecked();
     QString asc, des;
     switch (static_cast<SortKey>(ui->sortCombo->currentData().toInt())) {
     case SortKey::Rating:
          asc = tr("Low → High");
          des = tr("High → Low");
          break;
     case SortKey::Year:
          asc = tr("Old → New");
          des = tr("New → Old");
          break;
     default:
          asc = tr("A → Z");
          des = tr("Z → A");
          break;
     }
     ui->orderButton->setText(desc ? des : asc);
}

void MainWindow::updateUndoButton() {
     const bool can = library_.canUndo();
     ui->undoButton->setEnabled(can);
     ui->undoButton->setToolTip(can ? tr("Undo: %1 (Ctrl+Z)").arg(qstr(library_.undoLabel()))
          : tr("Nothing to undo yet"));
}

void MainWindow::fill(HintListWidget* list, const QVector<Row>& rows, bool keepScroll) {
     QSignalBlocker blocker(list);

     bool same = list->count() == rows.size();
     for (int i = 0; same && i < rows.size(); ++i)
          same = list->item(i)->data(BookDelegate::BookIdRole).toInt() == rows[i].book->id;

     if (!same) {
          const int scroll = list->verticalScrollBar()->value();
          list->clear();
          for (const Row& r : rows) {
               auto* item = new QListWidgetItem(list);
               item->setData(BookDelegate::BookIdRole, r.book->id);
          }
          if (keepScroll && scroll > 0)
               QTimer::singleShot(0, list, [list, scroll] { list->verticalScrollBar()->setValue(scroll); });
     }

     const int selectedId = ui->detailPanel->bookId();
     QListWidgetItem* selected = nullptr;
     for (int i = 0; i < rows.size(); ++i) {
          QListWidgetItem* item = list->item(i);
          item->setData(BookDelegate::NoteRole, rows[i].note);
          item->setData(BookDelegate::MatchRole, rows[i].match >= 0 ? QVariant(rows[i].match) : QVariant());
          item->setToolTip(qstr(rows[i].book->title) + QStringLiteral("  —  ") + qstr(rows[i].book->author));
          item->setData(Qt::AccessibleTextRole, qstr(rows[i].book->title));
          if (rows[i].book->id == selectedId) selected = item;
     }
     if (selected) list->setCurrentItem(selected);
     else list->setCurrentItem(nullptr);
     list->viewport()->update();
}

void MainWindow::refreshCatalog(bool keepScroll) {
     const auto key = static_cast<SortKey>(ui->sortCombo->currentData().toInt());
     const auto algo = static_cast<SortAlgorithm>(ui->algoCombo->currentData().toInt());
     const bool ascending = !ui->orderButton->isChecked();
     const QString genre = ui->genreCombo->currentData().toString();

     SortStats stats;
     const std::vector<Book*> found = library_.query(stdstr(ui->searchEdit->text()), stdstr(genre), key, ascending, algo, &stats);

     QVector<Row> rows;
     rows.reserve(static_cast<int>(found.size()));
     for (const Book* b : found) {
          Row r;
          r.book = b;
          rows.append(r);
     }

     const bool filtered = !ui->searchEdit->text().trimmed().isEmpty() || !genre.isEmpty();
     ui->catalogList->setHint(filtered ? tr("No books match your search.\nTry fewer words or pick another genre.")
          : tr("The catalog is empty.\nClick “Add Book” to add your first one."));
     fill(ui->catalogList, rows, keepScroll);

     ui->resultsLabel->setText(tr("%1 of %2 books  ·  %3  ·  %4 comparisons")
          .arg(found.size())
          .arg(library_.bookCount())
          .arg(ui->algoCombo->currentText())
          .arg(stats.comparisons));
}

void MainWindow::refreshShelves() {
     auto rowsFor = [this](Shelf s) {
          QVector<Row> rows;
          for (const Book* b : library_.shelf(s)) {
               Row r;
               r.book = b;
               rows.append(r);
          }
          return rows;
          };

     const QVector<Row> want = rowsFor(Shelf::WantToRead);
     const QVector<Row> reading = rowsFor(Shelf::CurrentlyReading);
     const QVector<Row> read = rowsFor(Shelf::Read);

     QVector<Row> reviews;
     for (const Book* b : library_.reviewFeed()) {
          Row r;
          r.book = b;
          r.note = b->review.text.empty() ? tr("(rated without a written review)") : qstr(b->review.text);
          reviews.append(r);
     }

     ui->wantList->setHint(tr("Nothing queued up yet.\nOpen a book from the Catalog and set its shelf to “Want to Read”."));
     ui->readingList->setHint(tr("You are not reading anything right now.\nStart a book from the Catalog."));
     ui->readList->setHint(tr("No finished books yet.\nBooks move here when you reach their last page."));
     ui->reviewsList->setHint(tr("No ratings or reviews yet.\nOpen any book and give it some stars."));

     fill(ui->wantList, want);
     fill(ui->readingList, reading);
     fill(ui->readList, read);
     fill(ui->reviewsList, reviews);

     ui->shelfTabs->setTabText(0, tr("Want to Read  (%1)").arg(want.size()));
     ui->shelfTabs->setTabText(1, tr("Currently Reading  (%1)").arg(reading.size()));
     ui->shelfTabs->setTabText(2, tr("Read  (%1)").arg(read.size()));
     ui->shelfTabs->setTabText(3, tr("My Reviews  (%1)").arg(reviews.size()));
}

void MainWindow::refreshRecommendations() {
     const std::vector<Recommendation> recs = recommend(library_, 10);

     QVector<Row> rows;
     for (const Recommendation& rec : recs) {
          Row r;
          r.book = rec.book;
          r.note = qstr(rec.reason);
          r.match = rec.matchPercent;
          rows.append(r);
     }

     const bool anyShelves = library_.shelf(Shelf::WantToRead).size() + library_.shelf(Shelf::CurrentlyReading).size() +
          library_.shelf(Shelf::Read).size() > 0;
     const bool hasTaste = library_.shelf(Shelf::Read).size() > 0 || !library_.reviewFeed().empty();

     ui->recoList->setHint(hasTaste ? tr("No new suggestions right now.\nRate more books to sharpen your taste profile.")
          : tr("Rate a few books you have read — 4 or 5 stars for the ones you loved —\n"
               "and personalised suggestions will appear here."));
     ui->loadDemoButton->setVisible(!anyShelves && library_.reviewFeed().empty());
     fill(ui->recoList, rows);
}

void MainWindow::refreshStats() {
     const LibraryStats s = computeStats(library_);
     const QLocale loc;

     ui->valRead->setText(QString::number(s.readCount));
     ui->valReading->setText(QString::number(s.readingCount));
     ui->valWant->setText(QString::number(s.wantCount));
     ui->valPages->setText(loc.toString(static_cast<qlonglong>(s.pagesRead)));
     ui->valAvg->setText(s.ratedCount > 0 ? QString::number(s.averageRating, 'f', 1) + QStringLiteral(" / 5") : QStringLiteral("—"));
     ui->valFav->setWordWrap(true);
     ui->valFav->setText(s.favoriteGenre.empty() ? QStringLiteral("—") : qstr(s.favoriteGenre));

     QVector<BarItem> genreItems;
     for (std::size_t i = 0; i < s.genreCounts.size() && i < 6; ++i) {
          BarItem it;
          it.label = qstr(s.genreCounts[i].first);
          it.value = s.genreCounts[i].second;
          it.color = genreColor(it.label);
          genreItems.append(it);
     }
     ui->genreCardTitle->setText(s.readCount > 0 ? tr("Books read by genre") : tr("Rated books by genre"));
     ui->genreChart->setEmptyText(tr("Finish or rate a few books to see your favourite genres"));
     ui->genreChart->setItems(genreItems);

     QVector<BarItem> ratingItems;
     for (int stars = 5; stars >= 1; --stars) {
          BarItem it;
          it.label = stars == 1 ? tr("1 star") : tr("%1 stars").arg(stars);
          it.value = s.ratingHistogram[stars];
          it.color = Theme::gold();
          ratingItems.append(it);
     }
     ui->ratingChart->setEmptyText(tr("Rate a book to see how you score things"));
     ui->ratingChart->setItems(s.ratedCount > 0 ? ratingItems : QVector<BarItem>());
}

void MainWindow::showStatus(const QString& message) { statusBar()->showMessage(message, 6000); }


QVector<HintListWidget*> MainWindow::allLists() const {
     return { ui->catalogList, ui->wantList, ui->readingList, ui->readList, ui->reviewsList, ui->recoList };
}

bool MainWindow::confirmLeaveBook() {
     if (!ui->detailPanel->isDirty()) return true;
     const Book* b = library_.findBook(ui->detailPanel->bookId());
     QMessageBox box(this);
     box.setIcon(QMessageBox::Question);
     box.setWindowTitle(tr("Unsaved changes"));
     box.setText(tr("You have unsaved changes to “%1”.").arg(b ? qstr(b->title) : QString()));
     box.setInformativeText(tr("Do you want to save them first?"));
     box.setStandardButtons(QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
     box.setDefaultButton(QMessageBox::Save);
     switch (box.exec()) {
     case QMessageBox::Save: return ui->detailPanel->commit();
     case QMessageBox::Discard: return true;
     default: return false;
     }
}

void MainWindow::openBook(int id) {
     if (id != ui->detailPanel->bookId()) {
          if (!confirmLeaveBook()) {
               syncSelection();
               return;
          }
          ui->detailPanel->showBook(id);
     }
     syncSelection();
}

void MainWindow::syncSelection() {
     const int id = ui->detailPanel->bookId();
     for (HintListWidget* list : allLists()) {
          QSignalBlocker blocker(list);
          QListWidgetItem* found = nullptr;
          for (int r = 0; r < list->count() && !found; ++r)
               if (list->item(r)->data(BookDelegate::BookIdRole).toInt() == id) found = list->item(r);
          if (found) {
               list->setCurrentItem(found);
          }
          else {
               list->clearSelection();
               list->setCurrentItem(nullptr);
          }
     }
}

void MainWindow::revealInCatalog(int id) {
     for (int r = 0; r < ui->catalogList->count(); ++r) {
          QListWidgetItem* item = ui->catalogList->item(r);
          if (item->data(BookDelegate::BookIdRole).toInt() == id) {
               ui->catalogList->scrollToItem(item, QAbstractItemView::PositionAtCenter);
               return;
          }
     }
}

void MainWindow::onListContextMenu(QListWidget* list, const QPoint& pos) {
     QListWidgetItem* item = list->itemAt(pos);
     if (!item) return;
     const int id = item->data(BookDelegate::BookIdRole).toInt();
     const Book* b = library_.findBook(id);
     if (!b) return;

     QMenu menu(this);
     QAction* title = menu.addAction(qstr(b->title));
     title->setEnabled(false);
     menu.addSeparator();

     const struct {
          Shelf shelf;
          QString text;
     } choices[] = { {Shelf::WantToRead, tr("Move to Want to Read")},
                    {Shelf::CurrentlyReading, tr("Move to Currently Reading")},
                    {Shelf::Read, tr("Mark as Read")} };
     for (const auto& c : choices) {
          QAction* a = menu.addAction(c.text);
          a->setData(static_cast<int>(c.shelf));
          a->setCheckable(true);
          a->setChecked(b->shelf == c.shelf);
     }
     menu.addSeparator();
     QAction* remove = menu.addAction(tr("Remove from my shelves"));
     remove->setData(static_cast<int>(Shelf::None));
     remove->setEnabled(b->shelf != Shelf::None);

     QAction* chosen = menu.exec(list->viewport()->mapToGlobal(pos));
     if (!chosen || !chosen->data().isValid()) return;
     const Shelf target = shelfFromInt(chosen->data().toInt());
     if (target == b->shelf) return;

     if (id == ui->detailPanel->bookId() && !confirmLeaveBook()) return;

     std::string label;
     if (library_.moveToShelf(id, target, &label)) {
          if (id == ui->detailPanel->bookId()) ui->detailPanel->reload();
          onLibraryChanged(qstr(label));
     }
}

void MainWindow::onLibraryChanged(const QString& message) {
     const bool ok = saveAll();
     refreshAll();
     showStatus(ok ? tr("%1  —  saved").arg(message) : message);
}

void MainWindow::onAddBook() {
     QStringList genres;
     for (const std::string& g : library_.genres()) genres << qstr(g);

     AddBookDialog dialog(genres, this);
     while (dialog.exec() == QDialog::Accepted) {
          std::string error;
          const Book entered = dialog.book();
          const int id = library_.addBook(entered, &error);
          if (id == 0) {
               dialog.setError(qstr(error));
               continue;
          }

          QString coverError;
          const bool coverFailed =
               !dialog.coverSource().isEmpty() && !CoverStore::importCover(id, dialog.coverSource(), &coverError);
          {
               QSignalBlocker a(ui->searchEdit), b(ui->genreCombo);
               ui->searchEdit->clear();
               ui->genreCombo->setCurrentIndex(0);
          }
          setPage(0);
          const bool ok = saveAll();
          refreshAll();
          openBook(id);
          revealInCatalog(id);
          showStatus(ok ? tr("Added “%1” to the catalog  —  saved").arg(qstr(entered.title))
               : tr("Added “%1” to the catalog").arg(qstr(entered.title)));
          return;
     }
}

void MainWindow::refreshCovers() {
     for (HintListWidget* list : allLists()) list->viewport()->update();
}

void MainWindow::changeEvent(QEvent* event) {
     QMainWindow::changeEvent(event);
     if (event->type() == QEvent::ActivationChange && isActiveWindow() && CoverStore::rescan()) {
          refreshCovers();
          ui->detailPanel->updateCover();
     }
}

void MainWindow::onUndo() {
     if (!library_.canUndo()) {
          showStatus(tr("Nothing to undo yet"));
          return;
     }
     if (!confirmLeaveBook()) return;
     std::string message;
     if (library_.undo(&message)) {
          ui->detailPanel->reload();
          onLibraryChanged(qstr(message));
     }
}

void MainWindow::onLoadDemo() {
     if (!confirmLeaveBook()) return;
     QFile f(QStringLiteral(":/data/demo_userdata.csv"));
     if (!f.open(QIODevice::ReadOnly)) return;
     const QByteArray data = f.readAll();

     library_.resetUserData();
     loadUserDataText(std::string(data.constData(), static_cast<std::size_t>(data.size())), library_);
     ui->detailPanel->reload();
     onLibraryChanged(tr("Loaded a sample reading history"));
}

void MainWindow::closeEvent(QCloseEvent* event) {
     if (!confirmLeaveBook()) {
          event->ignore();
          return;
     }
     saveAll();
     event->accept();
}



