/*
 * SPDX-FileCopyrightText: 2026 Rick Christy <grymmjack@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#pragma once

#include <QObject>

class ConfigureGridWidgetTest : public QObject
{
    Q_OBJECT
public:
    explicit ConfigureGridWidgetTest(QObject *parent = nullptr);
    ~ConfigureGridWidgetTest() override = default;

private Q_SLOTS:
    void shouldHaveDefaultValues();
    void shouldSaveAndLoadSettings();
    void shouldRestoreDefaults();
};
