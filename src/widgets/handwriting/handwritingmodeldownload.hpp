/*
 * SPDX-FileCopyrightText: 2026 Rick Christy <grymmjack@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#pragma once
#include "libdrawywidgets_private_export.h"
#include <QCryptographicHash>
#include <QObject>
#include <QString>
#include <memory>

class QNetworkAccessManager;
class QNetworkReply;
class QSaveFile;

/**
 * Downloads the handwriting recognition model to
 * HandwritingRecognition::modelDownloadPath(). The file is checked against its
 * known SHA-256 and only kept if it matches.
 */
class LIBDRAWYWIDGETS_TESTS_EXPORT HandwritingModelDownload : public QObject
{
    Q_OBJECT
public:
    explicit HandwritingModelDownload(QObject *parent = nullptr);
    ~HandwritingModelDownload() override;

    void start();
    void cancel();

Q_SIGNALS:
    void progress(qint64 received, qint64 total);
    // error is empty on success, and when the download was cancelled
    void finished(bool success, const QString &error);

private:
    void writeReceivedData();
    void finish();
    void fail(const QString &error);

    QNetworkAccessManager *const m_network;
    QNetworkReply *m_reply{nullptr};
    std::unique_ptr<QSaveFile> m_file;
    QCryptographicHash m_hash{QCryptographicHash::Sha256};
    bool m_cancelled{false};
};
