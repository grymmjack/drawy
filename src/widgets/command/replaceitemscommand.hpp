/*
 * SPDX-FileCopyrightText: 2026 Rick Christy <grymmjack@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#pragma once
#include "insertitemcommand.hpp"
#include "itemcommand.hpp"
#include "libdrawywidgets_private_export.h"
#include "removeitemcommand.hpp"
#include <QString>
class ApplicationContext;

/**
 * Removes some items and inserts others in one undoable step, e.g. replacing
 * handwriting with the text recognized from it.
 */
class LIBDRAWYWIDGETS_TESTS_EXPORT ReplaceItemsCommand : public ItemCommand
{
public:
    ReplaceItemsCommand(QList<std::shared_ptr<Item>> oldItems, QList<std::shared_ptr<Item>> newItems, QString text);

    void redo(ApplicationContext *context) override;
    void undo(ApplicationContext *context) override;
    [[nodiscard]] QString text() const override;

private:
    RemoveItemCommand m_remove;
    InsertItemCommand m_insert;
    QString m_text;
};
