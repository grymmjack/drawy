/*
 * SPDX-FileCopyrightText: 2026 Rick Christy <grymmjack@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#pragma once

#include <QObject>

class MimeManagerTest : public QObject
{
    Q_OBJECT
public:
    explicit MimeManagerTest(QObject *parent = nullptr);
    ~MimeManagerTest() override = default;

private Q_SLOTS:
    void shouldRoundTripNativeItemsWithNewIds();
    void shouldPasteNativeItemsWhenOnlyOurImageComesBack();
    void shouldNotUseNativeItemsForOtherContent();
};
