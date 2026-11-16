/*
 * SPDX-FileCopyrightText: 2026 Rick Christy <grymmjack@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#pragma once
#include "libdrawywidgets_private_export.h"
#include <QByteArray>
#include <QImage>
#include <QList>
#include <QRectF>
#include <QString>
#include <QStringList>
#include <QUrl>
#include <memory>
#include <span>

class FreeformItem;

/**
 * Turning hand-drawn strokes into text.
 *
 * The strokes are split into lines of writing, each line is rendered into a
 * clean image (black on white, even line weight) and read by PaddleOCR's text
 * recognition model, run with ONNX Runtime. The model is not shipped with
 * Drawy; it is downloaded on first use (see HandwritingModelDownload) or
 * installed by a package.
 *
 * renderLines() runs on the GUI thread; recognize() is slower and is meant to
 * run on a worker thread.
 */
namespace HandwritingRecognition
{
// false when Drawy was built without ONNX Runtime
LIBDRAWYWIDGETS_TESTS_EXPORT bool isSupported();

struct Model {
    QString fileName;
    QUrl url;
    // SHA-256 of the file, hex encoded
    QByteArray sha256;
    qint64 size;
};

// the recognition model Drawy uses
LIBDRAWYWIDGETS_TESTS_EXPORT const Model &model();

// The installed model: downloaded for this user, or installed system wide by a
// package (in <data dir>/drawy/handwriting/). Empty if there is none.
LIBDRAWYWIDGETS_TESTS_EXPORT QString installedModelPath();

// where a downloaded model is saved
LIBDRAWYWIDGETS_TESTS_EXPORT QString modelDownloadPath();

struct StrokeLines {
    // one image per line of writing, top to bottom
    QList<QImage> lines;
    // the world rectangle all the strokes cover (outline only, no stroke width)
    QRectF worldBounds;
};

// renders the strokes for recognition; no lines if there is nothing to render
LIBDRAWYWIDGETS_TESTS_EXPORT StrokeLines renderLines(const QList<std::shared_ptr<FreeformItem>> &strokes);

struct Recognition {
    // one line per line of writing; empty when nothing was recognized
    QString text;
    // why recognition failed (e.g. the model could not be loaded); empty if it ran
    QString error;
};

// reads the text in the line images with the model at modelPath
LIBDRAWYWIDGETS_TESTS_EXPORT Recognition recognize(const QList<QImage> &lines, const QString &modelPath);

// Greedy CTC decoding of the model's output: `scores` holds one row of
// `classes` scores per time step. Class 0 is the CTC blank and class i is
// characters[i]. Repeats of a class are merged unless a blank separates them.
LIBDRAWYWIDGETS_TESTS_EXPORT QString decodeCtc(std::span<const float> scores, qsizetype classes, const QStringList &characters);
} // namespace HandwritingRecognition
