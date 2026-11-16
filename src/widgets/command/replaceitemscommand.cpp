/*
 * SPDX-FileCopyrightText: 2026 Rick Christy <grymmjack@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#include "replaceitemscommand.hpp"
#include "drawy_command_debug.h"
#include <utility>

ReplaceItemsCommand::ReplaceItemsCommand(QList<std::shared_ptr<Item>> oldItems, QList<std::shared_ptr<Item>> newItems, QString text)
    : ItemCommand{newItems}
    , m_remove{std::move(oldItems)}
    , m_insert{std::move(newItems)}
    , m_text{std::move(text)}
{
    qCDebug(DRAWY_COMMAND_LOG) << "ReplaceItemsCommand" << m_items.count();
}

void ReplaceItemsCommand::redo(ApplicationContext *context)
{
    m_remove.redo(context);
    m_insert.redo(context);
}

void ReplaceItemsCommand::undo(ApplicationContext *context)
{
    // the reverse order, so the state between the two steps is the same either way
    m_insert.undo(context);
    m_remove.undo(context);
}

QString ReplaceItemsCommand::text() const
{
    return m_text;
}
