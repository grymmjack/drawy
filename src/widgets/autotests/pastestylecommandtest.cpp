/*
 * SPDX-FileCopyrightText: 2026 Rick Christy <grymmjack@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#include "pastestylecommandtest.hpp"
#include "command/pastestylecommand.hpp"
#include "item/ellipse.hpp"
#include "item/group.hpp"
#include "item/itemutils.hpp"
#include "item/rectangle.hpp"
#include "item/text.hpp"
#include <QColor>
#include <QTest>
#include <memory>
QTEST_MAIN(PasteStyleCommandTest)

namespace
{
Property strokeColor(const QColor &color)
{
    return Property{color, Property::Type::StrokeColor};
}

Property strokeWidth(int width)
{
    return Property{width, Property::Type::StrokeWidth};
}

Property roundedCorners()
{
    return Property{ItemUtils::convertItemCornerRectangleTypeEnumToString(Item::CornerRectangleType::Rounded), Property::Type::CornerRectangleType};
}
} // namespace

PasteStyleCommandTest::PasteStyleCommandTest(QObject *parent)
    : QObject{parent}
{
}

void PasteStyleCommandTest::shouldApplyAndRestoreStyle()
{
    const auto rectangle{std::make_shared<RectangleItem>()};
    rectangle->setProperty(Property::Type::StrokeColor, strokeColor(Qt::black));
    rectangle->setProperty(Property::Type::StrokeWidth, strokeWidth(2));

    PasteStyleCommand command{{rectangle}, {strokeColor(Qt::red), strokeWidth(8), roundedCorners()}};

    command.applyStyle();
    QCOMPARE(rectangle->property(Property::Type::StrokeColor).value<QColor>(), QColor(Qt::red));
    QCOMPARE(rectangle->property(Property::Type::StrokeWidth).value<int>(), 8);
    QCOMPARE(rectangle->property(Property::Type::CornerRectangleType), roundedCorners());

    command.restoreStyle();
    QCOMPARE(rectangle->property(Property::Type::StrokeColor).value<QColor>(), QColor(Qt::black));
    QCOMPARE(rectangle->property(Property::Type::StrokeWidth).value<int>(), 2);
    QVERIFY(rectangle->property(Property::Type::CornerRectangleType) != roundedCorners());

    // redo after undo applies the style again
    command.applyStyle();
    QCOMPARE(rectangle->property(Property::Type::StrokeWidth).value<int>(), 8);
}

void PasteStyleCommandTest::shouldSkipPropertiesTheItemDoesNotHave()
{
    // a rectangle's style pasted on an ellipse: the corner type has nowhere to go
    const auto ellipse{std::make_shared<EllipseItem>()};
    QVERIFY(!ellipse->hasProperty(Property::Type::CornerRectangleType));

    PasteStyleCommand command{{ellipse}, {strokeColor(Qt::red), roundedCorners()}};
    command.applyStyle();

    QCOMPARE(ellipse->property(Property::Type::StrokeColor).value<QColor>(), QColor(Qt::red));
    QVERIFY(!ellipse->hasProperty(Property::Type::CornerRectangleType));
}

void PasteStyleCommandTest::shouldReportWhetherAnythingChanges()
{
    const auto rectangle{std::make_shared<RectangleItem>()};
    rectangle->setProperty(Property::Type::StrokeColor, strokeColor(Qt::red));

    QVERIFY(!PasteStyleCommand({rectangle}, {strokeColor(Qt::red)}).hasChanged());
    QVERIFY(PasteStyleCommand({rectangle}, {strokeColor(Qt::blue)}).hasChanged());
    // nothing to change when no item supports the style
    QVERIFY(!PasteStyleCommand({std::make_shared<EllipseItem>()}, {roundedCorners()}).hasChanged());
    QVERIFY(!PasteStyleCommand({rectangle}, {}).hasChanged());
}

void PasteStyleCommandTest::shouldApplyToGroupChildrenAndRestoreEachOne()
{
    const auto red{std::make_shared<RectangleItem>()};
    red->setProperty(Property::Type::StrokeColor, strokeColor(Qt::red));
    const auto blue{std::make_shared<EllipseItem>()};
    blue->setProperty(Property::Type::StrokeColor, strokeColor(Qt::blue));

    const auto group{std::make_shared<GroupItem>()};
    group->group({red, blue});

    PasteStyleCommand command{{group}, {strokeColor(Qt::green)}};
    command.applyStyle();
    QCOMPARE(red->property(Property::Type::StrokeColor).value<QColor>(), QColor(Qt::green));
    QCOMPARE(blue->property(Property::Type::StrokeColor).value<QColor>(), QColor(Qt::green));

    // the children had different colours, and each gets its own back
    command.restoreStyle();
    QCOMPARE(red->property(Property::Type::StrokeColor).value<QColor>(), QColor(Qt::red));
    QCOMPARE(blue->property(Property::Type::StrokeColor).value<QColor>(), QColor(Qt::blue));
}

void PasteStyleCommandTest::shouldRestoreTextFormattingExactly()
{
    const auto text{std::make_shared<TextItem>()};
    text->cursor().insertText(QStringLiteral("hello"));
    const QString originalHtml{text->html()};
    const int originalSize{text->property(Property::Type::FontSize).value<int>()};

    PasteStyleCommand command{{text}, {Property{originalSize + 20, Property::Type::FontSize}, strokeColor(Qt::red)}};
    command.applyStyle();
    QCOMPARE(text->property(Property::Type::FontSize).value<int>(), originalSize + 20);

    command.restoreStyle();
    QCOMPARE(text->html(), originalHtml);
    QCOMPARE(text->property(Property::Type::FontSize).value<int>(), originalSize);
}

void PasteStyleCommandTest::shouldHaveText()
{
    QVERIFY(!PasteStyleCommand({}, {}).text().isEmpty());
}

#include "moc_pastestylecommandtest.cpp"
