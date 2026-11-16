/*
 * SPDX-FileCopyrightText: 2026 Rick Christy <grymmjack@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#include "handwritingrecognitiontest.hpp"
#include "common/constants.hpp"
#include "handwriting/handwritingrecognition.hpp"
#include "item/freeform.hpp"
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTest>
#include <cmath>
#include <memory>
#include <numbers>
QTEST_MAIN(HandwritingRecognitionTest)

using namespace Qt::Literals::StringLiterals;

namespace
{
// Block capitals drawn the way a person prints them: each letter is a few pen
// strokes in a box 0.6 wide and 1 tall (y grows downwards).
using Glyph = QList<QPolygonF>;

QPolygonF ellipse(qreal centerX, qreal centerY, qreal radiusX, qreal radiusY)
{
    constexpr int segments{40};
    QPolygonF polygon;
    for (int i{0}; i <= segments; ++i) {
        const qreal angle{2.0 * std::numbers::pi * i / segments};
        polygon.append({centerX + (radiusX * std::cos(angle)), centerY + (radiusY * std::sin(angle))});
    }
    return polygon;
}

Glyph glyph(QChar letter)
{
    switch (letter.unicode()) {
    case 'H':
        return {QPolygonF{{0, 0}, {0, 1}}, QPolygonF{{0.6, 0}, {0.6, 1}}, QPolygonF{{0, 0.5}, {0.6, 0.5}}};
    case 'E':
        return {QPolygonF{{0.6, 0}, {0, 0}, {0, 1}, {0.6, 1}}, QPolygonF{{0, 0.5}, {0.45, 0.5}}};
    case 'L':
        return {QPolygonF{{0, 0}, {0, 1}, {0.6, 1}}};
    case 'O':
        return {ellipse(0.3, 0.5, 0.3, 0.5)};
    case 'W':
        return {QPolygonF{{0, 0}, {0.15, 1}, {0.3, 0.4}, {0.45, 1}, {0.6, 0}}};
    case 'R':
        return {QPolygonF{{0, 1}, {0, 0}, {0.45, 0}, {0.6, 0.12}, {0.6, 0.38}, {0.45, 0.5}, {0, 0.5}}, QPolygonF{{0.3, 0.5}, {0.6, 1}}};
    case 'D':
        return {QPolygonF{{0, 0}, {0, 1}, {0.35, 1}, {0.6, 0.75}, {0.6, 0.25}, {0.35, 0}, {0, 0}}};
    default:
        return {};
    }
}

// a freeform stroke through the points, with a point every couple of world pixels like a real pen
std::shared_ptr<FreeformItem> penStroke(const QPolygonF &points)
{
    constexpr qreal step{2.0};
    auto stroke{std::make_shared<FreeformItem>()};
    for (qsizetype i{0}; i + 1 < points.size(); ++i) {
        const QLineF segment{points[i], points[i + 1]};
        const int steps{std::max(1, static_cast<int>(segment.length() / step))};
        for (int s{0}; s < steps; ++s) {
            stroke->addPoint(segment.pointAt(static_cast<qreal>(s) / steps), 1.0);
        }
    }
    stroke->addPoint(points.constLast(), 1.0);
    stroke->finalizeStroke();
    return stroke;
}

// How untidy the writing is: a forward slant (x shift per unit of height) and
// a wobble (amplitude as a fraction of the letter height).
struct Hand {
    qreal slant{0};
    qreal wobble{0};
};

// the text written in strokes with letters `height` world pixels tall, starting at origin
QList<std::shared_ptr<FreeformItem>> handwrite(const QString &text, QPointF origin, qreal height, Hand hand = {})
{
    // the wobble's wavelength along the stroke, as a fraction of the letter height
    constexpr qreal wobbleWavelength{0.35};
    QList<std::shared_ptr<FreeformItem>> strokes;
    qreal x{origin.x()};
    for (const QChar letter : text) {
        if (letter == u' ') {
            x += height;
            continue;
        }
        for (const auto &line : glyph(letter)) {
            QPolygonF scaled;
            for (const QPointF &point : line) {
                const qreal slantShift{hand.slant * (1.0 - point.y())};
                scaled.append({x + ((point.x() + slantShift) * height), origin.y() + (point.y() * height)});
            }
            QPolygonF wobbly;
            // sample densely so the wobble shows between the corners
            const auto dense{penStroke(scaled)->points()};
            for (const QPointF &point : dense) {
                const qreal phase{2.0 * std::numbers::pi * (point.x() + point.y()) / (wobbleWavelength * height)};
                wobbly.append(point + (QPointF{std::sin(phase), std::cos(phase)} * (hand.wobble * height)));
            }
            strokes.append(penStroke(wobbly));
        }
        x += 0.9 * height;
    }
    return strokes;
}
// the number of single-character edits that turn one string into the other
qsizetype editDistance(const QString &from, const QString &to)
{
    QList<qsizetype> previous(to.size() + 1);
    for (qsizetype j{0}; j <= to.size(); ++j) {
        previous[j] = j;
    }
    for (qsizetype i{1}; i <= from.size(); ++i) {
        QList<qsizetype> current(to.size() + 1);
        current[0] = i;
        for (qsizetype j{1}; j <= to.size(); ++j) {
            const qsizetype substitution{previous[j - 1] + (from[i - 1] == to[j - 1] ? 0 : 1)};
            current[j] = std::min({previous[j] + 1, current[j - 1] + 1, substitution});
        }
        previous = current;
    }
    return previous[to.size()];
}
// The recognition model: DRAWY_HANDWRITING_MODEL if set, else the installed one.
// Recognition tests are skipped without it.
QString modelPath()
{
    if (!HandwritingRecognition::isSupported()) {
        return {};
    }
    const QString fromEnvironment{qEnvironmentVariable("DRAWY_HANDWRITING_MODEL")};
    return fromEnvironment.isEmpty() ? HandwritingRecognition::installedModelPath() : fromEnvironment;
}

constexpr const char *noModelMessage{"no recognition model (built without ONNX Runtime, or set DRAWY_HANDWRITING_MODEL)"};
} // namespace

HandwritingRecognitionTest::HandwritingRecognitionTest(QObject *parent)
    : QObject{parent}
{
}

void HandwritingRecognitionTest::shouldDecodeCtc()
{
    // class 0 is the blank; one row of scores per time step
    const QStringList characters{QString{}, u"a"_s, u"b"_s, u" "_s};
    const QList<int> best{1, 1, 0, 1, 2, 2, 3, 0, 0};
    QList<float> scores;
    for (const int index : best) {
        for (int c{0}; c < characters.size(); ++c) {
            scores.append(c == index ? 0.9F : 0.05F);
        }
    }
    // repeats merge unless a blank separates them
    QCOMPARE(HandwritingRecognition::decodeCtc(scores, characters.size(), characters), u"aab "_s);
    QCOMPARE(HandwritingRecognition::decodeCtc({}, characters.size(), characters), QString{});
}

void HandwritingRecognitionTest::shouldRenderNothingForNoStrokes()
{
    QVERIFY(HandwritingRecognition::renderLines({}).lines.isEmpty());
}

void HandwritingRecognitionTest::shouldScaleLettersToAReadableSize()
{
    // tiny writing (10 world pixels) and huge writing (300) both come out at the same letter size
    for (const qreal height : {10.0, 300.0}) {
        const auto rendered{HandwritingRecognition::renderLines(handwrite(u"HELLO"_s, {100, 50}, height))};
        QCOMPARE(rendered.lines.size(), 1);
        QCOMPARE(rendered.worldBounds.top(), 50.0);
        QCOMPARE(rendered.worldBounds.height(), height);

        const int letterPixels{rendered.lines.constFirst().height() - (2 * Common::handwritingImagePadding)};
        QVERIFY2(std::abs(letterPixels - Common::handwritingLetterHeight) <= 2.0, qPrintable(QString::number(letterPixels)));
    }
}

void HandwritingRecognitionTest::shouldUseWhereMovedStrokesAre()
{
    // strokes moved after drawing keep their points and gain a transform
    auto strokes{handwrite(u"HELLO"_s, {0, 0}, 60)};
    for (const auto &stroke : std::as_const(strokes)) {
        stroke->setTransform(QTransform::fromTranslate(500, 200));
    }
    const auto rendered{HandwritingRecognition::renderLines(strokes)};
    QCOMPARE(rendered.worldBounds.topLeft(), QPointF(500, 200));
    QCOMPARE(rendered.lines.size(), 1);
}

void HandwritingRecognitionTest::shouldSplitLinesOfWriting()
{
    // two lines, the second a line and a half below the first
    auto strokes{handwrite(u"HELLO"_s, {0, 0}, 60)};
    strokes.append(handwrite(u"WORLD"_s, {0, 90}, 60));
    // a dash between the lines, nearer the second: it must not start a line of its own
    strokes.append(penStroke(QPolygonF{{10, 80}, {20, 80}}));

    const auto rendered{HandwritingRecognition::renderLines(strokes)};
    QCOMPARE(rendered.lines.size(), 2);
    // the dash went to the second line, making it taller than the first
    QVERIFY(rendered.lines.at(1).height() > rendered.lines.at(0).height());
    QCOMPARE(rendered.worldBounds.height(), 150.0);
}

void HandwritingRecognitionTest::shouldReportAMissingModel()
{
    QImage blank{400, 100, QImage::Format_Grayscale8};
    blank.fill(Qt::white);
    QVERIFY(!HandwritingRecognition::recognize({blank}, u"/nonexistent/model.onnx"_s).error.isEmpty());
}

void HandwritingRecognitionTest::shouldRecognizePrintedHandwriting_data()
{
    QTest::addColumn<qreal>("slant");
    QTest::addColumn<qreal>("wobble");

    QTest::newRow("neat") << 0.0 << 0.0;
    QTest::newRow("slanted") << 0.25 << 0.0;
    QTest::newRow("shaky") << 0.0 << 0.03;
    QTest::newRow("slanted and shaky") << 0.25 << 0.03;
}

void HandwritingRecognitionTest::shouldRecognizePrintedHandwriting()
{
    const QString model{modelPath()};
    if (model.isEmpty()) {
        QSKIP(noModelMessage);
    }

    QFETCH(qreal, slant);
    QFETCH(qreal, wobble);

    const auto rendered{HandwritingRecognition::renderLines(handwrite(u"HELLO WORLD"_s, {0, 0}, 60, {slant, wobble}))};
    const auto result{HandwritingRecognition::recognize(rendered.lines, model)};
    QCOMPARE(result.error, QString{});
    QCOMPARE(result.text, u"HELLO WORLD"_s);
}

void HandwritingRecognitionTest::shouldKeepLinesOfWriting()
{
    const QString model{modelPath()};
    if (model.isEmpty()) {
        QSKIP(noModelMessage);
    }

    auto strokes{handwrite(u"HELLO"_s, {0, 0}, 60)};
    strokes.append(handwrite(u"WORLD"_s, {0, 90}, 60));

    const auto result{HandwritingRecognition::recognize(HandwritingRecognition::renderLines(strokes).lines, model)};
    QCOMPARE(result.text, u"HELLO\nWORLD"_s);
}

void HandwritingRecognitionTest::shouldRecognizeNothingOnABlankImage()
{
    const QString model{modelPath()};
    if (model.isEmpty()) {
        QSKIP(noModelMessage);
    }

    QImage blank{400, 100, QImage::Format_Grayscale8};
    blank.fill(Qt::white);
    const auto result{HandwritingRecognition::recognize({blank}, model)};
    QCOMPARE(result.error, QString{});
    QCOMPARE(result.text, QString{});
}

void HandwritingRecognitionTest::shouldReadRealPrintedHandwriting()
{
    const QString model{modelPath()};
    if (model.isEmpty()) {
        QSKIP(noModelMessage);
    }

    // labels printed with a pen on a tablet, as drawn in Drawy (points only)
    QFile file{QLatin1StringView(DRAWY_DATA_DIR) + u"/handwriting/printed-labels.json"_s};
    QVERIFY(file.open(QIODevice::ReadOnly));
    const QJsonArray samples = QJsonDocument::fromJson(file.readAll()).object().value(u"samples"_s).toArray();
    QVERIFY(!samples.isEmpty());

    qsizetype totalCharacters{0};
    qsizetype totalErrors{0};
    for (const auto &sampleValue : samples) {
        const QJsonObject sample{sampleValue.toObject()};
        const QString expected{sample.value(u"text"_s).toString()};

        QList<std::shared_ptr<FreeformItem>> strokes;
        for (const auto &strokeValue : sample.value(u"strokes"_s).toArray()) {
            const QJsonArray coordinates = strokeValue.toArray();
            auto stroke{std::make_shared<FreeformItem>()};
            for (qsizetype i{0}; i + 1 < coordinates.size(); i += 2) {
                stroke->addPoint({coordinates[i].toDouble(), coordinates[i + 1].toDouble()}, 1.0);
            }
            stroke->finalizeStroke();
            strokes.append(stroke);
        }

        const auto result{HandwritingRecognition::recognize(HandwritingRecognition::renderLines(strokes).lines, model)};
        QCOMPARE(result.error, QString{});
        const qsizetype errors{editDistance(result.text, expected)};
        qInfo().noquote() << u"%1  %2 -> %3"_s.arg(errors).arg(expected, result.text);
        totalCharacters += expected.size();
        totalErrors += errors;
    }

    // PaddleOCR reads these at about 97%; Tesseract managed 40%
    const double accuracy{1.0 - (static_cast<double>(totalErrors) / static_cast<double>(totalCharacters))};
    qInfo().noquote() << u"character accuracy %1%"_s.arg(accuracy * 100.0, 0, 'f', 1);
    QVERIFY2(accuracy >= 0.9, qPrintable(u"character accuracy %1"_s.arg(accuracy)));
}

#include "moc_handwritingrecognitiontest.cpp"
