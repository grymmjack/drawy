/*
 * SPDX-FileCopyrightText: 2026 Rick Christy <grymmjack@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#pragma once

#include <QObject>

class SnapTest : public QObject
{
    Q_OBJECT
public:
    explicit SnapTest(QObject *parent = nullptr);
    ~SnapTest() override = default;

private Q_SLOTS:
    void shouldSnapValueToNearestGridLine_data();
    void shouldSnapValueToNearestGridLine();
    void shouldNotSnapWithInvalidGridSize();
    void shouldSnapPoint();
    void shouldSnapDeltaSoAnchorLandsOnGrid();

    void shouldNotDistributeFewerThanThreeBoxes();
    void shouldDistributeHorizontallyWithEqualGaps();
    void shouldDistributeVerticallyWithEqualGaps();
    void shouldReturnOffsetsInInputOrder();
    void shouldDistributeBoxesOfDifferentSizes();
};
