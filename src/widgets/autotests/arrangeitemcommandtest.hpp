/*
 * SPDX-FileCopyrightText: 2026 Rick Christy <grymmjack@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#pragma once

#include <QObject>

class ArrangeItemCommandTest : public QObject
{
    Q_OBJECT
public:
    explicit ArrangeItemCommandTest(QObject *parent = nullptr);
    ~ArrangeItemCommandTest() override = default;

private Q_SLOTS:
    void shouldDistributeHorizontally();
    void shouldNotChangeWhenAlreadyDistributed();
    void shouldNotDistributeTwoItems();
    void shouldSnapGeometryToGrid();
    void shouldNotChangeItemsAlreadyOnGrid();
    void shouldHaveText();
};
