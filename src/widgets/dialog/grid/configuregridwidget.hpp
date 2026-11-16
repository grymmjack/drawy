/*
 * SPDX-FileCopyrightText: 2026 Rick Christy <grymmjack@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once
#include "libdrawywidgets_private_export.h"
#include <QWidget>

class QCheckBox;
class QComboBox;
class QSpinBox;

class LIBDRAWYWIDGETS_TESTS_EXPORT ConfigureGridWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ConfigureGridWidget(QWidget *parent = nullptr);
    ~ConfigureGridWidget() override;

    void save();
    void load();
    void restoreToDefaults();

private:
    QCheckBox *const m_showGrid;
    QCheckBox *const m_snapToGrid;
    QSpinBox *const m_gridSize;
    QSpinBox *const m_majorLineEvery;
    QComboBox *const m_style;
};
