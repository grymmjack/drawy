/*
 * SPDX-FileCopyrightText: 2026 Rick Christy <grymmjack@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#include "replaceitemscommandtest.hpp"
#include "command/commandhistory.hpp"
#include "command/replaceitemscommand.hpp"
#include "context/applicationcontext.hpp"
#include "context/spatialcontext.hpp"
#include "data-structures/quadtree.hpp"
#include "item/freeform.hpp"
#include "item/text.hpp"
#include <QTest>
#include <QWidget>
#include <memory>
QTEST_MAIN(ReplaceItemsCommandTest)

using namespace Qt::Literals::StringLiterals;

namespace
{
std::shared_ptr<FreeformItem> stroke(QPointF from, QPointF to)
{
    auto item{std::make_shared<FreeformItem>()};
    item->addPoint(from, 1.0);
    item->addPoint(to, 1.0);
    item->finalizeStroke();
    return item;
}
} // namespace

ReplaceItemsCommandTest::ReplaceItemsCommandTest(QObject *parent)
    : QObject{parent}
{
}

void ReplaceItemsCommandTest::shouldReplaceItemsAndUndoInOneStep()
{
    QWidget window;
    ApplicationContext context{&window};
    auto *const spatialContext{context.spatialContext()};
    auto &quadtree{spatialContext->quadtree()};

    const std::shared_ptr<Item> first{stroke({0, 0}, {10, 40})};
    const std::shared_ptr<Item> second{stroke({20, 0}, {30, 40})};
    quadtree.insertItem(first);
    quadtree.insertItem(second);

    const auto text{std::make_shared<TextItem>()};
    text->createTextBox({0, 0});
    text->cursor().insertText(u"Hi"_s);
    const std::shared_ptr<Item> textItem{text};

    auto *const history{spatialContext->commandHistory()};
    history->push(std::make_shared<ReplaceItemsCommand>(QList{first, second}, QList{textItem}, u"Convert"_s));

    QCOMPARE(quadtree.getAllItems(), QList{textItem});

    // a single undo restores both strokes and removes the text
    history->undo();
    const auto restored{quadtree.getAllItems()};
    QCOMPARE(restored.size(), 2);
    QVERIFY(restored.contains(first));
    QVERIFY(restored.contains(second));
    QVERIFY(!history->canUndo());

    history->redo();
    QCOMPARE(quadtree.getAllItems(), QList{textItem});
}

#include "moc_replaceitemscommandtest.cpp"
