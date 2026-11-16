/*
 * SPDX-FileCopyrightText: 2026 Rick Christy <grymmjack@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#pragma once
#include "itemcommand.hpp"
#include "libdrawywidgets_private_export.h"
#include "properties/property.hpp"
#include <QList>
#include <QRectF>
#include <QString>
#include <unordered_map>
class ApplicationContext;

/**
 * Applies a copied style (a list of style properties) to items in one undoable
 * step. Each item only takes the properties it supports, so the style of a
 * rectangle can be pasted onto text, an arrow or a group.
 */
class LIBDRAWYWIDGETS_TESTS_EXPORT PasteStyleCommand : public ItemCommand
{
public:
    PasteStyleCommand(QList<std::shared_ptr<Item>> items, QList<Property> style);

    void redo(ApplicationContext *context) override;
    void undo(ApplicationContext *context) override;
    [[nodiscard]] QString text() const override;

    // false if no item would change, in which case the command need not be pushed
    [[nodiscard]] bool hasChanged() const;

    // The item changes without the canvas bookkeeping done by redo()/undo().
    // Each returns the world region the items covered before and after.
    QRectF applyStyle();
    QRectF restoreStyle();

private:
    void updateCanvas(ApplicationContext *context, const QRectF &dirtyRegion);

    QList<Property> m_style;
    std::unordered_map<std::shared_ptr<Item>, std::unordered_map<Property::Type, Property>> m_oldProperties;
    // text items keep their formatting in rich text, so restoring the html undoes
    // font changes exactly, including ones applied to only part of the text
    std::unordered_map<std::shared_ptr<Item>, QString> m_oldHtmls;
};
