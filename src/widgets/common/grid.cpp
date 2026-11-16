// SPDX-FileCopyrightText: 2026 Rick Christy <grymmjack@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "grid.hpp"

#include "canvas/canvas.hpp"
#include "common/constants.hpp"
#include "common/utils/snap.hpp"
#include "context/applicationcontext.hpp"
#include "context/coordinatetransformer.hpp"
#include "context/renderingcontext.hpp"
#include "context/spatialcontext.hpp"
#include "context/uicontext.hpp"
#include "drawyglobalconfig.h"
#include "event/event.hpp"

#include <QLineF>
#include <QPainter>
#include <QtMath>

namespace
{
// Grid lines are at multiples of `step`; this returns the indices of the
// first and last line inside [from, to].
std::pair<qint64, qint64> visibleLineRange(qreal from, qreal to, qreal step)
{
    return {static_cast<qint64>(std::ceil(from / step)), static_cast<qint64>(std::floor(to / step))};
}

bool isMajor(qint64 index, int majorEvery)
{
    return majorEvery > 0 && index % majorEvery == 0;
}
} // namespace

namespace Common::Grid
{
qreal size()
{
    return DrawyGlobalConfig::self()->gridSize();
}

bool isSnapActive(ApplicationContext *context)
{
    const bool invert{context->uiContext()->appEvent()->modifiers().testFlag(Qt::ControlModifier)};
    return DrawyGlobalConfig::self()->snapToGrid() != invert;
}

QPointF snapWorldPoint(ApplicationContext *context, QPointF worldPoint)
{
    if (!isSnapActive(context)) {
        return worldPoint;
    }

    return Common::Utils::Snap::snapPoint(worldPoint, size());
}

void render(ApplicationContext *context)
{
    const auto *config{DrawyGlobalConfig::self()};
    if (!config->showGrid()) {
        return;
    }

    auto *rendering{context->renderingContext()};
    auto *canvas{rendering->canvas()};
    auto &transformer{context->spatialContext()->coordinateTransformer()};

    const bool dots{config->gridStyle() == DrawyGlobalConfig::EnumGridStyle::Dots};
    const qreal zoom{rendering->zoomFactor()};
    const qreal deviceScale{canvas->scale()};
    const int majorEvery{config->gridMajorLineEvery()};

    // When zoomed out far enough that lines would blur together, draw only
    // every Nth line (the major lines) instead, repeating as needed.
    const qreal minSpacing{(dots ? Common::gridMinDotSpacing : Common::gridMinLineSpacing) * deviceScale};
    const int coarsenFactor{majorEvery > 1 ? majorEvery : 2};
    qreal step{size()};
    int coarsened{0};
    while (step * zoom < minSpacing) {
        if (++coarsened > Common::gridMaxCoarsening) {
            return;
        }
        step *= coarsenFactor;
    }

    const QRectF viewport{QPointF{0, 0}, QSizeF{canvas->dimensions()}};
    const QRectF world{transformer.viewToWorld(viewport)};
    const auto [firstColumn, lastColumn]{visibleLineRange(world.left(), world.right(), step)};
    const auto [firstRow, lastRow]{visibleLineRange(world.top(), world.bottom(), step)};

    const bool darkBackground{canvas->canvasBg().lightnessF() < Common::gridDarkBackgroundLightness};
    const QColor baseColor{darkBackground ? Qt::white : Qt::black};
    const auto colorWithAlpha{[&baseColor](int alpha) {
        QColor color{baseColor};
        color.setAlpha(alpha);
        return color;
    }};

    // world line index -> view coordinate, rounded to the pixel centre so 1px lines stay crisp
    const auto toViewX{[&transformer, step](qint64 index) {
        return std::floor(transformer.worldToView(QPointF{index * step, 0}).x()) + 0.5;
    }};
    const auto toViewY{[&transformer, step](qint64 index) {
        return std::floor(transformer.worldToView(QPointF{0, index * step}).y()) + 0.5;
    }};

    canvas->paintCanvas([&](QPainter &painter) {
        painter.setRenderHint(QPainter::Antialiasing, dots);

        if (dots) {
            QList<QPointF> minorDots;
            QList<QPointF> majorDots;
            for (qint64 column{firstColumn}; column <= lastColumn; ++column) {
                for (qint64 row{firstRow}; row <= lastRow; ++row) {
                    // even-sized dots are crisp when centred on a pixel corner rather than a pixel centre
                    const QPointF point{toViewX(column) - 0.5, toViewY(row) - 0.5};
                    (isMajor(column, majorEvery) && isMajor(row, majorEvery) ? majorDots : minorDots).append(point);
                }
            }

            QPen pen{colorWithAlpha(Common::gridMinorDotAlpha), Common::gridDotSize * deviceScale};
            pen.setCapStyle(Qt::RoundCap);
            painter.setPen(pen);
            painter.drawPoints(minorDots.constData(), static_cast<int>(minorDots.size()));

            pen.setColor(colorWithAlpha(Common::gridMajorDotAlpha));
            pen.setWidthF(Common::gridMajorDotSize * deviceScale);
            painter.setPen(pen);
            painter.drawPoints(majorDots.constData(), static_cast<int>(majorDots.size()));
            return;
        }

        QList<QLineF> minorLines;
        QList<QLineF> majorLines;
        for (qint64 column{firstColumn}; column <= lastColumn; ++column) {
            const qreal x{toViewX(column)};
            (isMajor(column, majorEvery) ? majorLines : minorLines).append(QLineF{x, viewport.top(), x, viewport.bottom()});
        }
        for (qint64 row{firstRow}; row <= lastRow; ++row) {
            const qreal y{toViewY(row)};
            (isMajor(row, majorEvery) ? majorLines : minorLines).append(QLineF{viewport.left(), y, viewport.right(), y});
        }

        // a cosmetic pen is always one device pixel wide
        QPen pen{colorWithAlpha(Common::gridMinorLineAlpha), 0};
        painter.setPen(pen);
        painter.drawLines(minorLines);

        pen.setColor(colorWithAlpha(Common::gridMajorLineAlpha));
        painter.setPen(pen);
        painter.drawLines(majorLines);
    });
}
} // namespace Common::Grid
