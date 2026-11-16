/*
 * SPDX-FileCopyrightText: 2026 Rick Christy <grymmjack@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#include "snaptest.hpp"
#include "common/utils/snap.hpp"
#include <QTest>

QTEST_GUILESS_MAIN(SnapTest)

using namespace Common::Utils::Snap;

namespace
{
// gaps between consecutive boxes once ordered along the axis
QList<qreal> gapsAfterDistribution(const QList<QRectF> &boxes, Axis axis)
{
    const auto offsets{distributeOffsets(boxes, axis)};

    QList<QRectF> moved;
    for (qsizetype i{0}; i < boxes.size(); ++i) {
        moved.append(boxes.at(i).translated(offsets.at(i)));
    }

    const bool horizontal{axis == Axis::Horizontal};
    std::ranges::sort(moved, [horizontal](const QRectF &a, const QRectF &b) {
        return horizontal ? a.left() < b.left() : a.top() < b.top();
    });

    QList<qreal> gaps;
    for (qsizetype i{1}; i < moved.size(); ++i) {
        gaps.append(horizontal ? moved.at(i).left() - moved.at(i - 1).right() : moved.at(i).top() - moved.at(i - 1).bottom());
    }
    return gaps;
}
} // namespace

SnapTest::SnapTest(QObject *parent)
    : QObject{parent}
{
}

void SnapTest::shouldSnapValueToNearestGridLine_data()
{
    QTest::addColumn<qreal>("value");
    QTest::addColumn<qreal>("gridSize");
    QTest::addColumn<qreal>("expected");

    QTest::newRow("on line") << 40.0 << 20.0 << 40.0;
    QTest::newRow("round down") << 49.0 << 20.0 << 40.0;
    QTest::newRow("round up") << 51.0 << 20.0 << 60.0;
    QTest::newRow("negative") << -29.0 << 20.0 << -20.0;
    QTest::newRow("negative round away") << -31.0 << 20.0 << -40.0;
    QTest::newRow("fractional grid") << 7.6 << 2.5 << 7.5;
    QTest::newRow("zero") << 0.0 << 20.0 << 0.0;
}

void SnapTest::shouldSnapValueToNearestGridLine()
{
    QFETCH(qreal, value);
    QFETCH(qreal, gridSize);
    QFETCH(qreal, expected);

    QCOMPARE(snapValue(value, gridSize), expected);
}

void SnapTest::shouldNotSnapWithInvalidGridSize()
{
    QCOMPARE(snapValue(13.7, 0.0), 13.7);
    QCOMPARE(snapValue(13.7, -10.0), 13.7);
    QCOMPARE(snapPoint(QPointF{3.3, 4.4}, 0.0), QPointF(3.3, 4.4));
}

void SnapTest::shouldSnapPoint()
{
    QCOMPARE(snapPoint(QPointF{12, 27}, 10), QPointF(10, 30));
    QCOMPARE(snapPoint(QPointF{-12, -27}, 10), QPointF(-10, -30));
}

void SnapTest::shouldSnapDeltaSoAnchorLandsOnGrid()
{
    // anchor already on the grid: delta is rounded to whole cells
    QCOMPARE(snapDelta(QPointF{40, 40}, QPointF{13, 28}, 20), QPointF(20, 20));

    // anchor off the grid: the result moves it onto the nearest intersection
    const QPointF anchor{33, 47};
    const QPointF delta{snapDelta(anchor, QPointF{5, 5}, 20)};
    QCOMPARE(anchor + delta, QPointF(40, 60));

    // a tiny drag of an off-grid anchor still snaps it
    QCOMPARE(anchor + snapDelta(anchor, QPointF{0, 0}, 20), QPointF(40, 40));
}

void SnapTest::shouldNotDistributeFewerThanThreeBoxes()
{
    const QList<QRectF> two{{0, 0, 10, 10}, {100, 0, 10, 10}};
    const auto offsets{distributeOffsets(two, Axis::Horizontal)};
    QCOMPARE(offsets.size(), 2);
    QCOMPARE(offsets.at(0), QPointF(0, 0));
    QCOMPARE(offsets.at(1), QPointF(0, 0));

    QVERIFY(distributeOffsets({}, Axis::Vertical).isEmpty());
}

void SnapTest::shouldDistributeHorizontallyWithEqualGaps()
{
    const QList<QRectF> boxes{{0, 0, 10, 10}, {15, 50, 10, 10}, {100, 20, 10, 10}};
    const auto offsets{distributeOffsets(boxes, Axis::Horizontal)};

    // the outer boxes stay put; the middle one moves from x=15 to x=50 so both gaps are 40
    QCOMPARE(offsets.at(0), QPointF(0, 0));
    QCOMPARE(offsets.at(2), QPointF(0, 0));
    QCOMPARE(offsets.at(1), QPointF(35, 0));

    for (const qreal gap : gapsAfterDistribution(boxes, Axis::Horizontal)) {
        QCOMPARE(gap, 40.0);
    }
}

void SnapTest::shouldDistributeVerticallyWithEqualGaps()
{
    const QList<QRectF> boxes{{0, 0, 10, 10}, {30, 12, 10, 10}, {0, 30, 10, 10}, {7, 90, 10, 10}};
    const auto offsets{distributeOffsets(boxes, Axis::Vertical)};

    for (const QPointF &offset : offsets) {
        QCOMPARE(offset.x(), 0.0);
    }

    for (const qreal gap : gapsAfterDistribution(boxes, Axis::Vertical)) {
        QCOMPARE(gap, 20.0);
    }
}

void SnapTest::shouldReturnOffsetsInInputOrder()
{
    // same boxes as the horizontal case, but shuffled
    const QList<QRectF> boxes{{100, 20, 10, 10}, {15, 50, 10, 10}, {0, 0, 10, 10}};
    const auto offsets{distributeOffsets(boxes, Axis::Horizontal)};

    QCOMPARE(offsets.at(0), QPointF(0, 0));
    QCOMPARE(offsets.at(1), QPointF(35, 0));
    QCOMPARE(offsets.at(2), QPointF(0, 0));
}

void SnapTest::shouldDistributeBoxesOfDifferentSizes()
{
    const QList<QRectF> boxes{{0, 0, 50, 10}, {60, 0, 10, 10}, {80, 0, 30, 10}, {200, 0, 20, 10}};
    // span = 220, total widths = 110, so each of the three gaps is 110 / 3
    for (const qreal gap : gapsAfterDistribution(boxes, Axis::Horizontal)) {
        QVERIFY(qFuzzyCompare(gap, 110.0 / 3.0));
    }
}

#include "moc_snaptest.cpp"
