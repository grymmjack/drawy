/*
 * SPDX-FileCopyrightText: 2026 Rick Christy <grymmjack@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#pragma once

#include <QObject>

class HandwritingRecognitionTest : public QObject
{
    Q_OBJECT
public:
    explicit HandwritingRecognitionTest(QObject *parent = nullptr);
    ~HandwritingRecognitionTest() override = default;

private Q_SLOTS:
    void shouldDecodeCtc();
    void shouldRenderNothingForNoStrokes();
    void shouldScaleLettersToAReadableSize();
    void shouldUseWhereMovedStrokesAre();
    void shouldSplitLinesOfWriting();
    void shouldReportAMissingModel();
    void shouldRecognizePrintedHandwriting_data();
    void shouldRecognizePrintedHandwriting();
    void shouldKeepLinesOfWriting();
    void shouldRecognizeNothingOnABlankImage();
    void shouldReadRealPrintedHandwriting();
};
