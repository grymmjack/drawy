// SPDX-FileCopyrightText: 2026 Rick Christy <grymmjack@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once
#include "libdrawygui_export.h"
#include <QList>
#include <QPointF>
#include <QRectF>

/**
 * Pure helpers for snapping world coordinates to a square grid whose lines
 * pass through the world origin, and for distributing boxes evenly.
 *
 * None of these functions know about settings or modifiers; callers decide
 * whether snapping is active and pass the grid size in world units.
 */
namespace Common::Utils::Snap
{
/**
 * @brief rounds a single coordinate to the nearest grid line
 * @returns the value unchanged if gridSize is not positive
 */
[[nodiscard]] LIBDRAWYGUI_EXPORT qreal snapValue(qreal value, qreal gridSize);

/**
 * @brief rounds a point to the nearest grid intersection
 */
[[nodiscard]] LIBDRAWYGUI_EXPORT QPointF snapPoint(QPointF point, qreal gridSize);

/**
 * @brief adjusts a drag so that an anchor point ends up on the grid
 *
 * When moving a selection, the anchor (usually the top left corner of the
 * selection geometry) is placed at anchor + rawDelta and then snapped.
 *
 * @param anchor the anchor point before the drag started
 * @param rawDelta how far the cursor moved
 * @returns the delta that puts the anchor on the nearest grid intersection
 */
[[nodiscard]] LIBDRAWYGUI_EXPORT QPointF snapDelta(QPointF anchor, QPointF rawDelta, qreal gridSize);

/**
 * @brief the axis along which boxes are distributed
 */
enum class Axis : uint8_t {
    Horizontal,
    Vertical,
};

/**
 * @brief computes offsets that space boxes with equal gaps between them
 *
 * The first and last boxes (ordered by their leading edge along the axis)
 * stay where they are; the rest move so that the gaps between neighbouring
 * boxes are equal. If the boxes are wider than the span they occupy, the gaps
 * are negative and the boxes overlap evenly.
 *
 * @param boxes the boxes to distribute, in any order
 * @returns one offset per box, in the same order as boxes; all zero if there
 * are fewer than three boxes
 */
[[nodiscard]] LIBDRAWYGUI_EXPORT QList<QPointF> distributeOffsets(const QList<QRectF> &boxes, Axis axis);
} // namespace Common::Utils::Snap
