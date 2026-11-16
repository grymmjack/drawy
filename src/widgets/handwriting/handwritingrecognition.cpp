/*
 * SPDX-FileCopyrightText: 2026 Rick Christy <grymmjack@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#include "handwritingrecognition.hpp"
#include "common/constants.hpp"
#include "config-drawy.hpp"
#include "item/freeform.hpp"
#include <QPainter>
#include <QStandardPaths>
#include <algorithm>
#include <array>
#include <cmath>
#include <vector>

#if HAVE_ONNXRUNTIME
// the headers are in an onnxruntime folder, which some packages put on the include path
#if __has_include(<onnxruntime_cxx_api.h>)
#include <onnxruntime_cxx_api.h>
#else
#include <onnxruntime/onnxruntime_cxx_api.h>
#endif
#endif

using namespace Qt::Literals::StringLiterals;

namespace
{
// a stroke counts as part of a letter's body (not a dot, dash or crossbar) when at least this much of a letter tall
constexpr qreal letterBodyFraction{0.5};

// Strokes in world coordinates, so rotated or scaled items render as they look.
QList<QPolygonF> worldStrokes(const QList<std::shared_ptr<FreeformItem>> &strokes)
{
    QList<QPolygonF> result;
    for (const auto &stroke : strokes) {
        if (stroke->points().isEmpty()) {
            continue;
        }
        result.append(stroke->transformObj().map(QPolygonF{stroke->points()}));
    }
    return result;
}

// A rough letter height: printed letters are mostly one or two strokes each and
// a cursive word is one stroke as tall as its letters, so the median stroke
// height is close to the letter height. Dots and dashes pull it down a little.
qreal estimatedLetterHeight(const QList<QPolygonF> &strokes)
{
    QList<qreal> heights;
    for (const auto &stroke : strokes) {
        heights.append(stroke.boundingRect().height());
    }
    const auto middle{heights.begin() + (heights.size() / 2)};
    std::nth_element(heights.begin(), middle, heights.end());
    return std::max(*middle, 1.0);
}

// Splits strokes into lines of writing, top to bottom. The letters' bodies
// define the lines: taken from the top, a stroke whose middle is above the
// bottom of the line so far belongs to it, otherwise it starts a new line.
// Dots, dashes and crossbars then go to the line nearest to them.
QList<QList<QPolygonF>> splitIntoLines(const QList<QPolygonF> &strokes, qreal letterHeight)
{
    struct Line {
        qreal top;
        qreal bottom;
        QList<QPolygonF> strokes;
    };

    QList<QPolygonF> bodies;
    QList<QPolygonF> marks;
    for (const auto &stroke : strokes) {
        (stroke.boundingRect().height() >= letterBodyFraction * letterHeight ? bodies : marks).append(stroke);
    }
    std::ranges::sort(bodies, {}, [](const QPolygonF &stroke) {
        return stroke.boundingRect().center().y();
    });

    QList<Line> lines;
    for (const auto &stroke : std::as_const(bodies)) {
        const QRectF bounds{stroke.boundingRect()};
        if (!lines.isEmpty() && bounds.center().y() <= lines.last().bottom) {
            Line &line{lines.last()};
            line.top = std::min(line.top, bounds.top());
            line.bottom = std::max(line.bottom, bounds.bottom());
            line.strokes.append(stroke);
        } else {
            lines.append({bounds.top(), bounds.bottom(), {stroke}});
        }
    }

    for (const auto &mark : std::as_const(marks)) {
        const qreal middle{mark.boundingRect().center().y()};
        const auto distance{[middle](const Line &line) {
            return std::max({line.top - middle, middle - line.bottom, 0.0});
        }};
        if (lines.isEmpty()) {
            lines.append({mark.boundingRect().top(), mark.boundingRect().bottom(), {}});
        }
        std::ranges::min_element(lines, {}, distance)->strokes.append(mark);
    }

    QList<QList<QPolygonF>> result;
    for (const auto &line : std::as_const(lines)) {
        result.append(line.strokes);
    }
    return result;
}

// Draws the strokes black on white, `scale` image pixels per world pixel, with
// an even pen regardless of pressure or stroke width: OCR reads shapes, not ink.
QImage renderLine(const QList<QPolygonF> &strokes, qreal scale)
{
    QRectF bounds;
    for (const auto &stroke : strokes) {
        bounds |= stroke.boundingRect();
    }

    // keep huge writing to a sane image size
    const qreal largestSide{std::max(bounds.width(), bounds.height()) * scale};
    const qreal maxContentSide{static_cast<qreal>(Common::handwritingMaxImageSide - (2 * Common::handwritingImagePadding))};
    if (largestSide > maxContentSide) {
        scale *= maxContentSide / largestSide;
    }

    const QSize imageSize{static_cast<int>(std::ceil(bounds.width() * scale)) + (2 * Common::handwritingImagePadding),
                          static_cast<int>(std::ceil(bounds.height() * scale)) + (2 * Common::handwritingImagePadding)};
    QImage image{imageSize, QImage::Format_Grayscale8};
    image.fill(Qt::white);

    QPainter painter{&image};
    painter.setRenderHint(QPainter::Antialiasing);
    painter.translate(Common::handwritingImagePadding, Common::handwritingImagePadding);
    painter.scale(scale, scale);
    painter.translate(-bounds.topLeft());
    // the pen width is wanted in image pixels, so it is divided by the scale
    painter.setPen(QPen{Qt::black, Common::handwritingPenWidth / scale, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin});

    for (const auto &stroke : strokes) {
        if (stroke.size() == 1) {
            painter.drawPoint(stroke.constFirst());
        } else {
            painter.drawPolyline(stroke);
        }
    }
    painter.end();
    return image;
}

#if HAVE_ONNXRUNTIME
// The model's input for a line image: scaled to the model's height, three equal
// channels with values from -1 (black) to 1 (white), padded on the right to the
// minimum width with zeros, as PaddleOCR prepares it.
std::vector<float> modelInput(const QImage &line, int64_t &width)
{
    const int height{Common::handwritingModelInputHeight};
    const int scaledWidth{std::max(1, static_cast<int>(std::ceil(static_cast<qreal>(height) * line.width() / line.height())))};
    const QImage scaled{line.convertToFormat(QImage::Format_Grayscale8).scaled(scaledWidth, height, Qt::IgnoreAspectRatio, Qt::SmoothTransformation)};

    width = std::max(scaledWidth, Common::handwritingModelMinInputWidth);
    constexpr int channels{3};
    const auto planeSize{static_cast<size_t>(height) * static_cast<size_t>(width)};
    std::vector<float> input(channels * planeSize, 0.0F);
    for (int y{0}; y < height; ++y) {
        const uchar *row{scaled.constScanLine(y)};
        for (int x{0}; x < scaledWidth; ++x) {
            const float value{(static_cast<float>(row[x]) / 255.0F - 0.5F) / 0.5F};
            const size_t offset{(static_cast<size_t>(y) * static_cast<size_t>(width)) + static_cast<size_t>(x)};
            for (int channel{0}; channel < channels; ++channel) {
                input[(channel * planeSize) + offset] = value;
            }
        }
    }
    return input;
}

Ort::Env &onnxEnvironment()
{
    static Ort::Env environment{ORT_LOGGING_LEVEL_WARNING, "drawy"};
    return environment;
}
#endif
} // namespace

namespace HandwritingRecognition
{
bool isSupported()
{
    return HAVE_ONNXRUNTIME;
}

const Model &model()
{
    // PaddleOCR's PP-OCRv6 small text recognition model (Apache-2.0), as
    // converted to ONNX and published by the RapidOCR project
    static const Model model{
        u"PP-OCRv6_rec_small.onnx"_s,
        QUrl{u"https://www.modelscope.cn/models/RapidAI/RapidOCR/resolve/v3.10.0/onnx/PP-OCRv6/rec/PP-OCRv6_rec_small.onnx"_s},
        "6f327246b50388f3c176ae304bd95767ea6dc0c9ae92153ef8cbe210b3c14884",
        21234383,
    };
    return model;
}

QString installedModelPath()
{
    return QStandardPaths::locate(QStandardPaths::GenericDataLocation, u"drawy/handwriting/"_s + model().fileName);
}

QString modelDownloadPath()
{
    return QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + u"/drawy/handwriting/"_s + model().fileName;
}

StrokeLines renderLines(const QList<std::shared_ptr<FreeformItem>> &strokes)
{
    const QList<QPolygonF> polygons{worldStrokes(strokes)};
    if (polygons.isEmpty()) {
        return {};
    }

    StrokeLines result;
    for (const auto &polygon : polygons) {
        result.worldBounds |= polygon.boundingRect();
    }

    const qreal letterHeight{estimatedLetterHeight(polygons)};
    const qreal scale{std::clamp(Common::handwritingLetterHeight / letterHeight, Common::handwritingMinScale, Common::handwritingMaxScale)};
    for (const auto &line : splitIntoLines(polygons, letterHeight)) {
        result.lines.append(renderLine(line, scale));
    }
    return result;
}

Recognition recognize(const QList<QImage> &lines, const QString &modelPath)
{
    Recognition result;
#if HAVE_ONNXRUNTIME
    try {
        const Ort::SessionOptions options;
        Ort::Session session{onnxEnvironment(), modelPath.toStdString().c_str(), options};
        Ort::AllocatorWithDefaultOptions allocator;

        // the characters the model knows are stored in the model itself; PaddleOCR
        // puts the CTC blank before them and a space after them
        const Ort::ModelMetadata metadata{session.GetModelMetadata()};
        const Ort::AllocatedStringPtr dictionary{metadata.LookupCustomMetadataMapAllocated("character", allocator)};
        if (!dictionary) {
            result.error = u"the model has no character list"_s;
            return result;
        }
        QStringList characters{QString::fromUtf8(dictionary.get()).split(u'\n')};
        characters.prepend(QString{});
        characters.append(u" "_s);

        const Ort::AllocatedStringPtr inputName{session.GetInputNameAllocated(0, allocator)};
        const Ort::AllocatedStringPtr outputName{session.GetOutputNameAllocated(0, allocator)};
        const std::array<const char *, 1> inputNames{inputName.get()};
        const std::array<const char *, 1> outputNames{outputName.get()};
        const Ort::MemoryInfo memory{Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault)};

        QStringList texts;
        for (const auto &line : lines) {
            int64_t width{0};
            std::vector<float> input{modelInput(line, width)};
            const std::array<int64_t, 4> shape{1, 3, Common::handwritingModelInputHeight, width};
            const Ort::Value tensor{Ort::Value::CreateTensor<float>(memory, input.data(), input.size(), shape.data(), shape.size())};

            const auto outputs{session.Run(Ort::RunOptions{nullptr}, inputNames.data(), &tensor, 1, outputNames.data(), 1)};
            // the output is [batch, time steps, classes]
            const std::vector<int64_t> outputShape{outputs.front().GetTensorTypeAndShapeInfo().GetShape()};
            const auto classes{static_cast<qsizetype>(outputShape.back())};
            if (outputShape.size() != 3 || classes != characters.size()) {
                result.error = u"the model's output does not match its character list"_s;
                return result;
            }
            const auto count{static_cast<size_t>(outputShape[1] * outputShape[2])};
            const QString text{decodeCtc({outputs.front().GetTensorData<float>(), count}, classes, characters).trimmed()};
            if (!text.isEmpty()) {
                texts.append(text);
            }
        }
        result.text = texts.join(u'\n');
    } catch (const Ort::Exception &exception) {
        result.error = QString::fromUtf8(exception.what());
    }
#else
    Q_UNUSED(lines)
    Q_UNUSED(modelPath)
    result.error = u"Drawy was built without ONNX Runtime"_s;
#endif
    return result;
}

QString decodeCtc(std::span<const float> scores, qsizetype classes, const QStringList &characters)
{
    QString text;
    qsizetype previous{0};
    for (size_t step{0}; (step + 1) * static_cast<size_t>(classes) <= scores.size(); ++step) {
        const auto row{scores.subspan(step * static_cast<size_t>(classes), static_cast<size_t>(classes))};
        const auto best{static_cast<qsizetype>(std::ranges::max_element(row) - row.begin())};
        if (best != 0 && best != previous && best < characters.size()) {
            text += characters.at(best);
        }
        previous = best;
    }
    return text;
}
} // namespace HandwritingRecognition
