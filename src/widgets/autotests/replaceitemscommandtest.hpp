/*
 * SPDX-FileCopyrightText: 2026 Rick Christy <grymmjack@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#pragma once

#include <QObject>

class ReplaceItemsCommandTest : public QObject
{
    Q_OBJECT
public:
    explicit ReplaceItemsCommandTest(QObject *parent = nullptr);
    ~ReplaceItemsCommandTest() override = default;

private Q_SLOTS:
    void shouldReplaceItemsAndUndoInOneStep();
};
