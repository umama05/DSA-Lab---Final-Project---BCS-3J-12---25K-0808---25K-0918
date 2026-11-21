#include "coverstore.h"

#include <QCache>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QHash>
#include <QImage>
#include <QImageReader>
#include <QObject>
#include <QPainter>
#include <QPainterPath>
#include <QStringList>

namespace {

    struct Entry {
        QString path;
        qint64 stamp = 0;
        bool operator==(const Entry& o) const { return path == o.path && stamp == o.stamp; }
    };

    QString g_folder;
    QHash<int, Entry> g_index;
    QCache<QString, QImage> g_cache(256);

    const QStringList& extensions() {
        static const QStringList e = { QStringLiteral("jpg"), QStringLiteral("jpeg"), QStringLiteral("png"),
                                      QStringLiteral("webp"), QStringLiteral("bmp") };
        return e;
    }

    constexpr int kMaxSide = 900;

    QImage render(const QImage& src, const QSize& size, qreal dpr) {
        const QSize px(qMax(1, qRound(size.width() * dpr)), qMax(1, qRound(size.height() * dpr)));
        const QImage scaled = src.scaled(px, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
        const int x = (scaled.width() - px.width()) / 2;
        const int y = (scaled.height() - px.height()) / 2;

        QImage out(px, QImage::Format_ARGB32_Premultiplied);
        out.setDevicePixelRatio(dpr);
        out.fill(Qt::transparent);

        QPainter p(&out);
        p.setRenderHint(QPainter::Antialiasing);
        p.setRenderHint(QPainter::SmoothPixmapTransform);
        const QRectF r(0, 0, size.width(), size.height());
        const qreal radius = qMin(r.width(), r.height()) * 0.09;
        QPainterPath shape;
        shape.addRoundedRect(r, radius, radius);
        p.setClipPath(shape);
        p.drawImage(r, scaled, QRectF(x, y, px.width(), px.height()));
        p.setClipping(false);
        p.setBrush(Qt::NoBrush);
        p.setPen(QPen(QColor(0, 0, 0, 40), 1));
        p.drawPath(shape);
        return out;
    }

    QImage readImage(const QString& file, QString* error) {
        QImageReader reader(file);
        reader.setAutoTransform(true);
        QImage img = reader.read();
        if (img.isNull() && error)
            *error = QObject::tr("That file could not be read as a picture (%1).").arg(reader.errorString());
        return img;
    }

    void removeFilesOf(int bookId) {
        if (g_folder.isEmpty()) return;
        const QDir dir(g_folder);
        const QFileInfoList files = dir.entryInfoList(QDir::Files);
        for (const QFileInfo& fi : files)
            if (fi.completeBaseName() == QString::number(bookId) && extensions().contains(fi.suffix().toLower()))
                QFile::remove(fi.absoluteFilePath());
    }

}  // namespace

namespace CoverStore {

    void setFolder(const QString& folder) {
        g_folder = folder;
        rescan();
    }

    QString folder() { return g_folder; }

    bool rescan() {
        QHash<int, Entry> found;
        if (!g_folder.isEmpty()) {
            const QFileInfoList files = QDir(g_folder).entryInfoList(QDir::Files);
            for (const QFileInfo& fi : files) {
                if (!extensions().contains(fi.suffix().toLower())) continue;
                bool ok = false;
                const int id = fi.completeBaseName().toInt(&ok);
                if (!ok || id <= 0) continue;
                Entry e;
                e.path = fi.absoluteFilePath();
                e.stamp = fi.lastModified().toMSecsSinceEpoch();
                // two files for one book: keep the newest
                if (!found.contains(id) || found.value(id).stamp < e.stamp) found.insert(id, e);
            }
        }
        const bool changed = (found != g_index);
        if (changed) {
            g_index = found;
            g_cache.clear();
        }
        return changed;
    }

    bool hasCover(int bookId) { return g_index.contains(bookId); }

    bool importCover(int bookId, const QString& sourceFile, QString* error) {
        auto fail = [&](const QString& message) {
            if (error) *error = message;
            return false;
            };
        if (bookId <= 0) return fail(QObject::tr("This book has no ID yet."));
        if (g_folder.isEmpty()) return fail(QObject::tr("The covers folder is not set."));

        QString readError;
        QImage img = readImage(sourceFile, &readError);
        if (img.isNull()) return fail(readError);
        if (img.width() < 20 || img.height() < 20) return fail(QObject::tr("That picture is too small to be a cover."));

        if (img.width() > kMaxSide || img.height() > kMaxSide)
            img = img.scaled(kMaxSide, kMaxSide, Qt::KeepAspectRatio, Qt::SmoothTransformation);

        if (img.hasAlphaChannel()) {
            QImage flat(img.size(), QImage::Format_RGB32);
            flat.fill(Qt::white);
            QPainter p(&flat);
            p.drawImage(0, 0, img);
            p.end();
            img = flat;
        }

        if (!QDir().mkpath(g_folder)) return fail(QObject::tr("The covers folder could not be created."));
        removeFilesOf(bookId);

        const QDir dir(g_folder);
        QString dest = dir.filePath(QString::number(bookId) + QStringLiteral(".jpg"));
        if (!img.save(dest, "JPG", 90)) {
            dest = dir.filePath(QString::number(bookId) + QStringLiteral(".png"));
            if (!img.save(dest, "PNG")) return fail(QObject::tr("The picture could not be saved in the covers folder."));
        }
        g_cache.clear();
        rescan();
        return true;
    }

    void removeCover(int bookId) {
        removeFilesOf(bookId);
        g_cache.clear();
        rescan();
    }

    QPixmap coverPixmap(int bookId, const QSize& size, qreal dpr) {
        const auto it = g_index.constFind(bookId);
        if (it == g_index.constEnd() || size.isEmpty()) return QPixmap();

        const QString key = QStringLiteral("%1|%2|%3x%4|%5")
            .arg(bookId)
            .arg(it->stamp)
            .arg(size.width())
            .arg(size.height())
            .arg(dpr);
        if (const QImage* cached = g_cache.object(key)) return QPixmap::fromImage(*cached);

        const QImage src = readImage(it->path, nullptr);
        if (src.isNull()) return QPixmap();
        auto* img = new QImage(render(src, size, dpr));
        const QPixmap result = QPixmap::fromImage(*img);
        g_cache.insert(key, img);
        return result;
    }

    QPixmap previewFromFile(const QString& file, const QSize& size, qreal dpr, QString* error) {
        const QImage src = readImage(file, error);
        if (src.isNull()) return QPixmap();
        return QPixmap::fromImage(render(src, size, dpr));
    }

    QString fileDialogFilter() {
        return QObject::tr("Pictures (*.png *.jpg *.jpeg *.webp *.bmp);;All files (*)");
    }

}  // namespace CoverStore



