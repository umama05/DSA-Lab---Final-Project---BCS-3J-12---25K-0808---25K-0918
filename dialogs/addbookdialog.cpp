#include "addbookdialog.h"

#include <QFileDialog>
#include <QIntValidator>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QStandardPaths>
#include <QVBoxLayout>

#include "coverstore.h"
#include "guiutil.h"
#include "ui_addbookdialog.h"

AddBookDialog::AddBookDialog(const QStringList& existingGenres, QWidget* parent)
    : QDialog(parent), ui(new Ui::AddBookDialog) {
    ui->setupUi(this);
    repolish(this);

    ui->genreCombo->addItems(existingGenres);
    ui->genreCombo->setCurrentIndex(-1);
    ui->genreCombo->lineEdit()->setPlaceholderText(tr("Pick one or type a new genre"));

    auto* coverRow = new QWidget(this);
    auto* rowLayout = new QHBoxLayout(coverRow);
    rowLayout->setContentsMargins(0, 0, 0, 0);
    rowLayout->setSpacing(12);
    coverPreview_ = new QLabel(coverRow);
    coverPreview_->setFixedSize(46, 66);
    rowLayout->addWidget(coverPreview_);
    auto* column = new QVBoxLayout();
    column->setSpacing(4);
    chooseCoverButton_ = new QPushButton(tr("Choose picture"), coverRow);
    chooseCoverButton_->setProperty("secondary", true);
    chooseCoverButton_->setCursor(Qt::PointingHandCursor);
    clearCoverButton_ = new QPushButton(tr("Use generated cover"), coverRow);
    clearCoverButton_->setProperty("link", true);
    clearCoverButton_->setCursor(Qt::PointingHandCursor);
    clearCoverButton_->setVisible(false);
    auto* hint = new QLabel(tr("Optional. Without a picture, BookVerse draws a cover for you."), coverRow);
    hint->setProperty("role", QStringLiteral("hint"));
    hint->setWordWrap(true);
    column->addWidget(chooseCoverButton_, 0, Qt::AlignLeft);
    column->addWidget(clearCoverButton_, 0, Qt::AlignLeft);
    column->addWidget(hint);
    rowLayout->addLayout(column, 1);
    int descRow = ui->formLayout->rowCount();
    QFormLayout::ItemRole role;
    ui->formLayout->getWidgetPosition(ui->descriptionEdit, &descRow, &role);
    ui->formLayout->insertRow(descRow, tr("Cover"), coverRow);
    repolish(coverRow);

    connect(chooseCoverButton_, &QPushButton::clicked, this, &AddBookDialog::onChooseCover);
    connect(clearCoverButton_, &QPushButton::clicked, this, [this] {
        coverSource_.clear();
        updateCoverPreview();
    });

    connect(ui->titleEdit, &QLineEdit::textChanged, this, &AddBookDialog::updateCoverPreview);
    connect(ui->genreCombo, &QComboBox::currentTextChanged, this, &AddBookDialog::updateCoverPreview);
    updateCoverPreview();

    QPushButton* save = ui->buttonBox->button(QDialogButtonBox::Save);
    save->setText(tr("Add to catalog"));
    save->setProperty("primary", true);
    save->setCursor(Qt::PointingHandCursor);
    QPushButton* cancel = ui->buttonBox->button(QDialogButtonBox::Cancel);
    cancel->setProperty("secondary", true);
    cancel->setCursor(Qt::PointingHandCursor);

    ui->yearEdit->setValidator(new QIntValidator(0, 2100, this));
    ui->pagesEdit->setValidator(new QIntValidator(0, 99999, this));

    ui->titleEdit->setFocus();
}

AddBookDialog::~AddBookDialog() { delete ui; }

Book AddBookDialog::book() const {
    Book b;
    b.title = stdstr(ui->titleEdit->text());
    b.author = stdstr(ui->authorEdit->text());
    b.genre = stdstr(ui->genreCombo->currentText());
    b.year = ui->yearEdit->text().trimmed().toInt();    
    b.pages = ui->pagesEdit->text().trimmed().toInt();
    b.description = stdstr(ui->descriptionEdit->toPlainText());
    return b;
}

void AddBookDialog::onChooseCover() {
    const QString file = QFileDialog::getOpenFileName(this, tr("Choose a cover picture"),
                                                      QStandardPaths::writableLocation(QStandardPaths::PicturesLocation),
                                                      CoverStore::fileDialogFilter());
    if (file.isEmpty()) return;
    QString error;
    CoverStore::previewFromFile(file, coverPreview_->size(), 1.0, &error);  
    if (!error.isEmpty()) {
        QMessageBox::warning(this, tr("Cover picture"), error);
        return;
    }
    coverSource_ = file;
    updateCoverPreview();
}

void AddBookDialog::updateCoverPreview() {
    const qreal dpr = devicePixelRatioF();
    QPixmap pm;
    if (!coverSource_.isEmpty()) pm = CoverStore::previewFromFile(coverSource_, coverPreview_->size(), dpr);
    if (pm.isNull()) pm = makeCover(book(), coverPreview_->size(), dpr); 
    coverPreview_->setPixmap(pm);
    chooseCoverButton_->setText(coverSource_.isEmpty() ? tr("Choose picture") : tr("Choose a different picture"));
    clearCoverButton_->setVisible(!coverSource_.isEmpty());
}

void AddBookDialog::setError(const QString& message) {
    ui->errorLabel->setText(message);
    ui->errorLabel->setVisible(!message.isEmpty());
}
