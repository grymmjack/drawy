/*
 * SPDX-FileCopyrightText: 2026 Rick Christy <grymmjack@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#pragma once

#include <QObject>

class PasteStyleCommandTest : public QObject
{
    Q_OBJECT
public:
    explicit PasteStyleCommandTest(QObject *parent = nullptr);
    ~PasteStyleCommandTest() override = default;

private Q_SLOTS:
    void shouldApplyAndRestoreStyle();
    void shouldSkipPropertiesTheItemDoesNotHave();
    void shouldReportWhetherAnythingChanges();
    void shouldApplyToGroupChildrenAndRestoreEachOne();
    void shouldRestoreTextFormattingExactly();
    void shouldHaveText();
};
