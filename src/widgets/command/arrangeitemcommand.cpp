/*
 * SPDX-FileCopyrightText: 2026 Rick Christy <grymmjack@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#include "arrangeitemcommand.hpp"
#include "common/utils/snap.hpp"
#include "context/applicationcontext.hpp"
#include "context/coordinatetransformer.hpp"
#include "context/renderingcontext.hpp"
#include "context/spatialcontext.hpp"
#include "data-structures/cachegrid.hpp"
#include "data-structures/quadtree.hpp"
#include "drawy_command_debug.h"
#include "item/item.hpp"
#include <KLocalizedString>

using Common::Utils::Snap::Axis;

ArrangeItemCommand::ArrangeItemCommand(QList<std::shared_ptr<Item>> items, Arrangement arrangement, qreal gridSize)
    : ItemCommand{std::move(items)}
    , m_arrangement{arrangement}
{
    qCDebug(DRAWY_COMMAND_LOG) << "ArrangeItemCommand" << m_items.count();

    switch (m_arrangement) {
    case Arrangement::DistributeHorizontally:
    case Arrangement::DistributeVertically: {
        // use the same boxes as the align actions so the two combine predictably
        QList<QRectF> boxes;
        for (const auto &item : std::as_const(m_items)) {
            boxes.append(item->boundingBox());
        }
        const Axis axis{m_arrangement == Arrangement::DistributeHorizontally ? Axis::Horizontal : Axis::Vertical};
        m_offsets = Common::Utils::Snap::distributeOffsets(boxes, axis);
        break;
    }
    case Arrangement::SnapToGrid:
        for (const auto &item : std::as_const(m_items)) {
            const QPointF anchor{item->geometryBoundingBox().topLeft()};
            m_offsets.append(Common::Utils::Snap::snapDelta(anchor, QPointF{0, 0}, gridSize));
        }
        break;
    }
}

void ArrangeItemCommand::redo(ApplicationContext *context)
{
    translateItems(context, true);
}

void ArrangeItemCommand::undo(ApplicationContext *context)
{
    translateItems(context, false);
}

void ArrangeItemCommand::translateItems(ApplicationContext *context, bool forward)
{
    auto &transformer{context->spatialContext()->coordinateTransformer()};
    auto &cacheGrid{context->renderingContext()->cacheGrid()};
    auto &quadtree{context->spatialContext()->quadtree()};

    QRectF dirtyRegion;
    for (qsizetype i{0}; i < m_items.size(); ++i) {
        const QPointF worldOffset{forward ? m_offsets.at(i) : -m_offsets.at(i)};
        if (worldOffset.isNull()) {
            continue;
        }

        const auto &item{m_items.at(i)};
        dirtyRegion |= item->boundingBox();
        quadtree.deleteItem(item, false);

        // items translate in their own (possibly rotated) coordinate system
        const QTransform invertedTransform{item->transformObj().inverted()};
        const QPointF origin{item->boundingBox().topLeft()};
        item->translate(invertedTransform.map(origin + worldOffset) - invertedTransform.map(origin));

        quadtree.insertItem(item, false);
        dirtyRegion |= item->boundingBox();
    }

    cacheGrid.markDirty(transformer.worldToGrid(dirtyRegion).toAlignedRect());
}

QString ArrangeItemCommand::text() const
{
    switch (m_arrangement) {
    case Arrangement::DistributeHorizontally:
    case Arrangement::DistributeVertically:
        return i18n("Distribute Items");
    case Arrangement::SnapToGrid:
        return i18n("Snap Items to Grid");
    }
    return {};
}

bool ArrangeItemCommand::hasChanged() const
{
    return std::ranges::any_of(m_offsets, [](const QPointF &offset) {
        return !offset.isNull();
    });
}

const QList<QPointF> &ArrangeItemCommand::offsets() const
{
    return m_offsets;
}
