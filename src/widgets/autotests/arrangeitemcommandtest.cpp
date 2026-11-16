/*
 * SPDX-FileCopyrightText: 2026 Rick Christy <grymmjack@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "arrangeitemcommandtest.hpp"
#include "command/arrangeitemcommand.hpp"
#include "item/rectangle.hpp"
#include <QTest>

QTEST_MAIN(ArrangeItemCommandTest)

namespace
{
std::shared_ptr<Item> makeRectangle(QPointF start, QPointF end, int strokeWidth = 1)
{
    auto rectangle{std::make_shared<RectangleItem>()};
    rectangle->setProperty(Property::Type::StrokeWidth, Property(strokeWidth, Property::Type::StrokeWidth));
    rectangle->setStart(start);
    rectangle->setEnd(end);
    return rectangle;
}
} // namespace

ArrangeItemCommandTest::ArrangeItemCommandTest(QObject *parent)
    : QObject{parent}
{
}

void ArrangeItemCommandTest::shouldDistributeHorizontally()
{
    // padded boxes: [0,10], [15,25], [100,110] -> middle box should start at 50
    const QList<std::shared_ptr<Item>> items{makeRectangle({1, 1}, {9, 9}), makeRectangle({16, 1}, {24, 9}), makeRectangle({101, 1}, {109, 9})};
    const ArrangeItemCommand command{items, ArrangeItemCommand::Arrangement::DistributeHorizontally};

    QVERIFY(command.hasChanged());
    QCOMPARE(command.offsets().size(), 3);
    QCOMPARE(command.offsets().at(0), QPointF(0, 0));
    QCOMPARE(command.offsets().at(1), QPointF(35, 0));
    QCOMPARE(command.offsets().at(2), QPointF(0, 0));
}

void ArrangeItemCommandTest::shouldNotChangeWhenAlreadyDistributed()
{
    const QList<std::shared_ptr<Item>> items{makeRectangle({0, 0}, {10, 10}), makeRectangle({0, 30}, {10, 40}), makeRectangle({0, 60}, {10, 70})};
    const ArrangeItemCommand command{items, ArrangeItemCommand::Arrangement::DistributeVertically};
    QVERIFY(!command.hasChanged());
}

void ArrangeItemCommandTest::shouldNotDistributeTwoItems()
{
    const QList<std::shared_ptr<Item>> items{makeRectangle({0, 0}, {10, 10}), makeRectangle({33, 0}, {43, 10})};
    const ArrangeItemCommand command{items, ArrangeItemCommand::Arrangement::DistributeHorizontally};
    QVERIFY(!command.hasChanged());
}

void ArrangeItemCommandTest::shouldSnapGeometryToGrid()
{
    // a thick stroke must not affect where the outline lands
    const QList<std::shared_ptr<Item>> items{makeRectangle({33, 47}, {73, 87}, 6), makeRectangle({-9, 12}, {11, 32}, 2)};
    const ArrangeItemCommand command{items, ArrangeItemCommand::Arrangement::SnapToGrid, 20};

    QVERIFY(command.hasChanged());
    QCOMPARE(items.at(0)->geometryBoundingBox().topLeft() + command.offsets().at(0), QPointF(40, 40));
    QCOMPARE(items.at(1)->geometryBoundingBox().topLeft() + command.offsets().at(1), QPointF(-0.0, 20));
}

void ArrangeItemCommandTest::shouldNotChangeItemsAlreadyOnGrid()
{
    const QList<std::shared_ptr<Item>> items{makeRectangle({40, 60}, {75, 99}, 6)};
    const ArrangeItemCommand command{items, ArrangeItemCommand::Arrangement::SnapToGrid, 20};
    QVERIFY(!command.hasChanged());
}

void ArrangeItemCommandTest::shouldHaveText()
{
    const QList<std::shared_ptr<Item>> items{makeRectangle({0, 0}, {10, 10})};
    QVERIFY(!ArrangeItemCommand(items, ArrangeItemCommand::Arrangement::SnapToGrid, 20).text().isEmpty());
    QVERIFY(!ArrangeItemCommand(items, ArrangeItemCommand::Arrangement::DistributeHorizontally).text().isEmpty());
}

#include "moc_arrangeitemcommandtest.cpp"
