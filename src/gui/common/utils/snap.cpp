// SPDX-FileCopyrightText: 2026 Rick Christy <grymmjack@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "snap.hpp"

#include <QtMath>
#include <algorithm>
#include <numeric>

namespace Common::Utils::Snap
{
qreal snapValue(qreal value, qreal gridSize)
{
    if (gridSize <= 0.0) {
        return value;
    }

    return std::round(value / gridSize) * gridSize;
}

QPointF snapPoint(QPointF point, qreal gridSize)
{
    return QPointF{snapValue(point.x(), gridSize), snapValue(point.y(), gridSize)};
}

QPointF snapDelta(QPointF anchor, QPointF rawDelta, qreal gridSize)
{
    return snapPoint(anchor + rawDelta, gridSize) - anchor;
}

QList<QPointF> distributeOffsets(const QList<QRectF> &boxes, Axis axis)
{
    QList<QPointF> offsets(boxes.size(), QPointF{0, 0});

    // with fewer than three boxes there is nothing in between to space out
    constexpr qsizetype minimumBoxes{3};
    if (boxes.size() < minimumBoxes) {
        return offsets;
    }

    const bool horizontal{axis == Axis::Horizontal};
    const auto start{[horizontal](const QRectF &box) {
        return horizontal ? box.left() : box.top();
    }};
    const auto length{[horizontal](const QRectF &box) {
        return horizontal ? box.width() : box.height();
    }};

    // visit boxes in order of their leading edge, keeping the original index
    // so that the offsets can be returned in the caller's order
    QList<qsizetype> order(boxes.size());
    std::iota(order.begin(), order.end(), 0);
    std::ranges::stable_sort(order, [&boxes, &start](qsizetype a, qsizetype b) {
        return start(boxes.at(a)) < start(boxes.at(b));
    });

    const QRectF &first{boxes.at(order.constFirst())};
    const QRectF &last{boxes.at(order.constLast())};

    const qreal spanStart{start(first)};
    const qreal spanEnd{start(last) + length(last)};
    const qreal totalLength{std::accumulate(boxes.cbegin(), boxes.cend(), 0.0, [&length](qreal sum, const QRectF &box) {
        return sum + length(box);
    })};

    const qreal gap{(spanEnd - spanStart - totalLength) / static_cast<qreal>(boxes.size() - 1)};

    qreal cursor{spanStart + length(first) + gap};
    for (qsizetype i{1}; i < order.size() - 1; ++i) {
        const qsizetype index{order.at(i)};
        const QRectF &box{boxes.at(index)};

        const qreal shift{cursor - start(box)};
        offsets[index] = horizontal ? QPointF{shift, 0} : QPointF{0, shift};

        cursor += length(box) + gap;
    }

    return offsets;
}
} // namespace Common::Utils::Snap
