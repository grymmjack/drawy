// SPDX-FileCopyrightText: 2026 Rick Christy <grymmjack@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QPointF>
#include <QRectF>

class ApplicationContext;

/**
 * Glue between the grid settings, the current input event and the pure
 * snapping helpers in Common::Utils::Snap.
 *
 * Snapping follows the "Snap to Grid" setting, inverted while Ctrl is held,
 * so users can briefly place something freely (or snap it) without toggling
 * the setting.
 */
namespace Common::Grid
{
// distance between grid lines in world units
[[nodiscard]] qreal size();

// whether snapping applies to the event currently being handled
[[nodiscard]] bool isSnapActive(ApplicationContext *context);

// snaps a world point to the nearest grid intersection if snapping is active
[[nodiscard]] QPointF snapWorldPoint(ApplicationContext *context, QPointF worldPoint);

// draws the grid onto the canvas (not the overlay), below the items
void render(ApplicationContext *context);
} // namespace Common::Grid
