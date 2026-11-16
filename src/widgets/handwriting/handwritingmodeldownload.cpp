/*
 * SPDX-FileCopyrightText: 2026 Rick Christy <grymmjack@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#include "handwritingmodeldownload.hpp"
#include "handwritingrecognition.hpp"
#include <KLocalizedString>
#include <QDir>
#include <QFileInfo>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSaveFile>

HandwritingModelDownload::HandwritingModelDownload(QObject *parent)
    : QObject{parent}
    , m_network{new QNetworkAccessManager(this)}
{
}

HandwritingModelDownload::~HandwritingModelDownload() = default;

void HandwritingModelDownload::start()
{
    const QString path{HandwritingRecognition::modelDownloadPath()};
    if (!QDir().mkpath(QFileInfo{path}.absolutePath())) {
        fail(i18n("Could not create the folder %1.", QFileInfo{path}.absolutePath()));
        return;
    }
    // written next to the final file and only moved into place once complete and verified
    m_file = std::make_unique<QSaveFile>(path);
    if (!m_file->open(QIODevice::WriteOnly)) {
        fail(m_file->errorString());
        return;
    }

    m_reply = m_network->get(QNetworkRequest{HandwritingRecognition::model().url});
    connect(m_reply, &QNetworkReply::readyRead, this, &HandwritingModelDownload::writeReceivedData);
    connect(m_reply, &QNetworkReply::downloadProgress, this, [this](qint64 received, qint64 total) {
        Q_EMIT progress(received, total > 0 ? total : HandwritingRecognition::model().size);
    });
    connect(m_reply, &QNetworkReply::finished, this, &HandwritingModelDownload::finish);
}

void HandwritingModelDownload::cancel()
{
    m_cancelled = true;
    if (m_reply) {
        m_reply->abort();
    }
}

void HandwritingModelDownload::writeReceivedData()
{
    const QByteArray data{m_reply->readAll()};
    m_hash.addData(data);
    if (m_file->write(data) != data.size()) {
        m_reply->abort();
    }
}

void HandwritingModelDownload::finish()
{
    m_reply->deleteLater();
    if (m_cancelled) {
        m_file->cancelWriting();
        Q_EMIT finished(false, {});
        return;
    }
    if (m_reply->error() != QNetworkReply::NoError) {
        fail(m_reply->errorString());
        return;
    }
    writeReceivedData();
    if (m_hash.result().toHex() != HandwritingRecognition::model().sha256) {
        fail(i18n("The downloaded file is not the expected model (its checksum does not match)."));
        return;
    }
    if (!m_file->commit()) {
        fail(m_file->errorString());
        return;
    }
    Q_EMIT finished(true, {});
}

void HandwritingModelDownload::fail(const QString &error)
{
    if (m_file) {
        m_file->cancelWriting();
    }
    Q_EMIT finished(false, error);
}

#include "moc_handwritingmodeldownload.cpp"
