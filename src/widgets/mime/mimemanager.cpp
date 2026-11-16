// SPDX-FileCopyrightText: 2026 Nikolay Kochulin <basiqueevangelist@yandex.ru>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "mime/mimemanager.hpp"
#include "common/constants.hpp"
#include "context/applicationcontext.hpp"
#include "item/item.hpp"
#include "mime/drawymimehandler.hpp"
#include "mime/imagemimehandler.hpp"
#include "mime/rendermimehandler.hpp"
#include "mime/svgmimehandler.hpp"
#include "mime/textmimehandler.hpp"
#include <QCryptographicHash>
#include <QImage>
#include <QMimeData>

namespace
{
// A hash of the image's pixels, independent of the pixel format it was decoded
// into: a clipboard manager re-encodes the image, so its bytes differ, but the
// pixels of a lossless round trip do not.
QByteArray imageFingerprint(const QImage &image)
{
    const QImage normalized{image.convertToFormat(QImage::Format_ARGB32)};
    const qsizetype rowBytes{static_cast<qsizetype>(normalized.width()) * static_cast<qsizetype>(sizeof(QRgb))};

    QCryptographicHash hash{QCryptographicHash::Sha256};
    for (int y{0}; y < normalized.height(); ++y) {
        hash.addData(QByteArrayView{reinterpret_cast<const char *>(normalized.constScanLine(y)), rowBytes});
    }
    return hash.result();
}
} // namespace

MimeManager::MimeManager(ApplicationContext *parent)
    : QObject(parent)
{
    registerHandler(std::make_unique<DrawyMimeHandler>());
    registerHandler(std::make_unique<ImageMimeHandler>(parent));
    registerHandler(std::make_unique<TextMimeHandler>());
    registerHandler(std::make_unique<SvgMimeHandler>());
    registerHandler(std::make_unique<RenderMimeHandler>());
}

MimeManager::~MimeManager() = default;

void MimeManager::registerHandler(std::unique_ptr<MimeHandler> &&handler)
{
    m_handlers.push_back(std::move(handler));
}

QList<std::shared_ptr<Item>> MimeManager::readData(const QMimeData &mimeData) const
{
    if (isLastWrittenImage(mimeData)) {
        QMimeData nativeData;
        nativeData.setData(Common::drawyClipboardMime, m_lastWrittenItems);
        return readData(nativeData);
    }

    for (auto &handler : m_handlers) {
        const QList<std::shared_ptr<Item>> items{handler->tryReadData(mimeData)};

        if (!items.isEmpty()) {
            return items;
        }
    }

    return {};
}

void MimeManager::writeData(QMimeData &mimeData, const QList<std::shared_ptr<Item>> &selectedItems)
{
    for (auto &handler : m_handlers) {
        handler->contributeData(mimeData, selectedItems);
    }

    m_lastWrittenItems.clear();
    m_lastWrittenImageSize = {};
    m_lastWrittenImageFingerprint.clear();

    if (mimeData.hasFormat(Common::drawyClipboardMime) && mimeData.hasImage()) {
        const QImage image{qvariant_cast<QImage>(mimeData.imageData())};
        m_lastWrittenItems = mimeData.data(Common::drawyClipboardMime);
        m_lastWrittenImageSize = image.size();
        m_lastWrittenImageFingerprint = imageFingerprint(image);
    }
}

bool MimeManager::isLastWrittenImage(const QMimeData &mimeData) const
{
    if (m_lastWrittenItems.isEmpty() || mimeData.hasFormat(Common::drawyClipboardMime) || !mimeData.hasImage()) {
        return false;
    }

    const QImage image{qvariant_cast<QImage>(mimeData.imageData())};
    // compare the size first so unrelated images are rejected without hashing them
    return image.size() == m_lastWrittenImageSize && imageFingerprint(image) == m_lastWrittenImageFingerprint;
}

#include "moc_mimemanager.cpp"
