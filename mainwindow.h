//  mainwindow.h  -  application window: connects the Qt screens to the library 

#pragma once

#include <QMainWindow>
#include <QString>
#include <QVector>

#include "library.h"

class HintListWidget;
class QCloseEvent;
class QListWidget;
class QPoint;

QT_BEGIN_NAMESPACE
namespace Ui {
     class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
     Q_OBJECT
public:
     explicit MainWindow(const QString& dataDir = QString(), QWidget* parent = nullptr);
     ~MainWindow() override;

     QString dataDir() const { return dataDir_; }
     void setPage(int index);

protected:
     void closeEvent(QCloseEvent* event) override;
     void changeEvent(QEvent* event) override;

private slots:
     void refreshCovers();
     void onAddBook();
     void onUndo();
     void onLoadDemo();
     void onLibraryChanged(const QString& message);

private:
     struct Row {
          const Book* book = nullptr;
          QString note;
          int match = -1;
     };

     void setupControls();
     void setupConnections();
     void setupShortcuts();
     void loadData();

     bool saveAll();

     void refreshAll();
     void rebuildGenreCombo();
     void refreshCatalog(bool keepScroll);
     void refreshShelves();
     void refreshRecommendations();
     void refreshStats();
     void updateUndoButton();
     void updateOrderButton();
     void fill(HintListWidget* list, const QVector<Row>& rows, bool keepScroll = true);

     QVector<HintListWidget*> allLists() const;
     void openBook(int id);
     bool confirmLeaveBook();
     void syncSelection();
     void revealInCatalog(int id);
     void onListContextMenu(QListWidget* list, const QPoint& pos);

     void showStatus(const QString& message);

     Ui::MainWindow* ui;
     Library library_;
     QString dataDir_;
     QString catalogPath_;
     QString userPath_;
     bool saveWarned_ = false;
};


