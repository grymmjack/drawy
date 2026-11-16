// SPDX-FileCopyrightText: 2026 Nikolay Kochulin <basiqueevangelist@yandex.ru>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "libdrawywidgets_private_export.h"
#include "mime/mimehandler.hpp"
#include <QByteArray>
#include <QObject>
#include <QSize>
#include <memory>
#include <vector>

class Item;
class QMimeData;
class ApplicationContext;

class LIBDRAWYWIDGETS_TESTS_EXPORT MimeManager : public QObject
{
    Q_OBJECT
public:
    explicit MimeManager(ApplicationContext *parent = nullptr);
    ~MimeManager() override;

    void registerHandler(std::unique_ptr<MimeHandler> &&handler);

    [[nodiscard]] QList<std::shared_ptr<Item>> readData(const QMimeData &mimeData) const;
    void writeData(QMimeData &mimeData, const QList<std::shared_ptr<Item>> &selectedItems);

private:
    [[nodiscard]] bool isLastWrittenImage(const QMimeData &mimeData) const;

    std::vector<std::unique_ptr<MimeHandler>> m_handlers;

    // What the last writeData() put on the clipboard. Clipboard managers such as
    // Klipper may take the clipboard over and serve back only the rendered image,
    // dropping the native format; when the image we get back is the one we
    // rendered, the native items are pasted instead of a picture of them.
    QByteArray m_lastWrittenItems;
    QSize m_lastWrittenImageSize;
    QByteArray m_lastWrittenImageFingerprint;
};
