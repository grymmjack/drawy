/*
 * SPDX-FileCopyrightText: 2026 Rick Christy <grymmjack@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#include "pastestylecommand.hpp"
#include "context/applicationcontext.hpp"
#include "context/coordinatetransformer.hpp"
#include "context/renderingcontext.hpp"
#include "context/spatialcontext.hpp"
#include "data-structures/cachegrid.hpp"
#include "data-structures/quadtree.hpp"
#include "drawy_command_debug.h"
#include "item/item.hpp"
#include "item/text.hpp"
#include <KLocalizedString>
#include <algorithm>
#include <utility>

PasteStyleCommand::PasteStyleCommand(QList<std::shared_ptr<Item>> items, QList<Property> style)
    : ItemCommand{std::move(items)}
    , m_style{std::move(style)}
{
    qCDebug(DRAWY_COMMAND_LOG) << "PasteStyleCommand" << m_items.count();
}

bool PasteStyleCommand::hasChanged() const
{
    return std::ranges::any_of(m_items, [this](const std::shared_ptr<Item> &item) {
        return std::ranges::any_of(m_style, [&item](const Property &property) {
            return item->hasProperty(property.type()) && item->needsPropertyUpdate(property);
        });
    });
}

QRectF PasteStyleCommand::applyStyle()
{
    m_oldProperties.clear();
    m_oldHtmls.clear();

    QRectF dirtyRegion;
    for (const auto &item : std::as_const(m_items)) {
        bool changed{false};
        for (const auto &property : std::as_const(m_style)) {
            const Property::Type type{property.type()};
            if (!item->hasProperty(type)) {
                continue;
            }

            if (!changed) {
                // the item may shrink (e.g. a thinner stroke), so its old area must be redrawn too
                dirtyRegion |= item->boundingBox();
                if (item->formType() == Item::FormType::Text) {
                    m_oldHtmls[item] = std::static_pointer_cast<TextItem>(item)->html();
                }
                changed = true;
            }

            m_oldProperties[item].insert_or_assign(type, item->property(type));
            item->setProperty(type, property);
        }

        if (changed) {
            item->setDirty(true);
            dirtyRegion |= item->boundingBox();
        }
    }

    return dirtyRegion;
}

QRectF PasteStyleCommand::restoreStyle()
{
    QRectF dirtyRegion;
    for (const auto &[item, oldProperties] : m_oldProperties) {
        dirtyRegion |= item->boundingBox();

        for (const auto &[type, oldProperty] : oldProperties) {
            item->setProperty(type, oldProperty);
        }
        // last, so the rich text is restored exactly as it was
        if (const auto it{m_oldHtmls.find(item)}; it != m_oldHtmls.end()) {
            std::static_pointer_cast<TextItem>(item)->setHtml(it->second);
        }

        item->setDirty(true);
        dirtyRegion |= item->boundingBox();
    }

    return dirtyRegion;
}

void PasteStyleCommand::redo(ApplicationContext *context)
{
    // a new stroke width or font size changes the bounding box the quadtree indexes
    // the item by, so take the items out before changing them and put them back after
    auto &quadtree{context->spatialContext()->quadtree()};
    for (const auto &item : std::as_const(m_items)) {
        quadtree.deleteItem(item, false);
    }

    const QRectF dirtyRegion{applyStyle()};

    for (const auto &item : std::as_const(m_items)) {
        quadtree.insertItem(item, false);
    }
    updateCanvas(context, dirtyRegion);
}

void PasteStyleCommand::undo(ApplicationContext *context)
{
    auto &quadtree{context->spatialContext()->quadtree()};
    for (const auto &item : std::as_const(m_items)) {
        quadtree.deleteItem(item, false);
    }

    const QRectF dirtyRegion{restoreStyle()};

    for (const auto &item : std::as_const(m_items)) {
        quadtree.insertItem(item, false);
    }
    updateCanvas(context, dirtyRegion);
}

void PasteStyleCommand::updateCanvas(ApplicationContext *context, const QRectF &dirtyRegion)
{
    auto &transformer{context->spatialContext()->coordinateTransformer()};
    context->renderingContext()->cacheGrid().markDirty(transformer.worldToGrid(dirtyRegion).toAlignedRect());
}

QString PasteStyleCommand::text() const
{
    return i18n("Paste Style");
}
