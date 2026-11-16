/*
 * SPDX-FileCopyrightText: 2026 Rick Christy <grymmjack@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#pragma once
#include "itemcommand.hpp"
#include "libdrawywidgets_private_export.h"
#include <QList>
#include <QPointF>
class ApplicationContext;

/**
 * Moves each item by its own offset, computed once from the items' positions
 * when the command is created: distributing items with equal gaps, or snapping
 * each item's top left corner to the grid.
 */
class LIBDRAWYWIDGETS_TESTS_EXPORT ArrangeItemCommand : public ItemCommand
{
public:
    enum class Arrangement : uint8_t {
        DistributeHorizontally,
        DistributeVertically,
        SnapToGrid,
    };

    /**
     * @param gridSize only used by Arrangement::SnapToGrid
     */
    explicit ArrangeItemCommand(QList<std::shared_ptr<Item>> items, Arrangement arrangement, qreal gridSize = 0);

    void redo(ApplicationContext *context) override;
    void undo(ApplicationContext *context) override;

    [[nodiscard]] QString text() const override;
    // false if no item would move, in which case the command need not be pushed
    [[nodiscard]] bool hasChanged() const;

    // world offset applied to each item, in the same order as the items
    [[nodiscard]] const QList<QPointF> &offsets() const;

private:
    void translateItems(ApplicationContext *context, bool forward);

    Arrangement m_arrangement;
    QList<QPointF> m_offsets;
};
