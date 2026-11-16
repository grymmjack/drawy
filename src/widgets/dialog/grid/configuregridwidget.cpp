/*
 * SPDX-FileCopyrightText: 2026 Rick Christy <grymmjack@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "configuregridwidget.hpp"
#include "drawyglobalconfig.h"
#include <KLocalizedString>
#include <QCheckBox>
#include <QComboBox>
#include <QFormLayout>
#include <QLabel>
#include <QSpinBox>
#include <QVBoxLayout>

using namespace Qt::Literals::StringLiterals;
ConfigureGridWidget::ConfigureGridWidget(QWidget *parent)
    : QWidget{parent}
    , m_showGrid{new QCheckBox(i18nc("@option:check", "Show grid"), this)}
    , m_snapToGrid{new QCheckBox(i18nc("@option:check", "Snap to grid"), this)}
    , m_gridSize{new QSpinBox(this)}
    , m_majorLineEvery{new QSpinBox(this)}
    , m_style{new QComboBox(this)}
{
    m_showGrid->setObjectName(u"m_showGrid"_s);
    m_snapToGrid->setObjectName(u"m_snapToGrid"_s);
    m_gridSize->setObjectName(u"m_gridSize"_s);
    m_majorLineEvery->setObjectName(u"m_majorLineEvery"_s);
    m_style->setObjectName(u"m_style"_s);

    auto config{DrawyGlobalConfig::self()};

    m_snapToGrid->setToolTip(i18nc("@info:tooltip", "Shapes snap to grid intersections while drawing, moving and resizing"));

    m_gridSize->setRange(config->gridSizeItem()->minValue().toInt(), config->gridSizeItem()->maxValue().toInt());
    m_gridSize->setSuffix(i18nc("@label:spinbox unit suffix for the grid size", " px"));
    m_gridSize->setToolTip(i18nc("@info:tooltip", "Distance between grid lines at 100% zoom"));

    m_majorLineEvery->setRange(config->gridMajorLineEveryItem()->minValue().toInt(), config->gridMajorLineEveryItem()->maxValue().toInt());
    m_majorLineEvery->setSpecialValueText(i18nc("@item:inrange no major grid lines", "Never"));
    m_majorLineEvery->setSuffix(i18nc("@label:spinbox unit suffix, counts grid cells", " cells"));
    m_majorLineEvery->setToolTip(i18nc("@info:tooltip", "Draw a stronger line every this many grid cells"));

    m_style->addItem(i18nc("@item:inlistbox grid style", "Lines"), static_cast<int>(DrawyGlobalConfig::EnumGridStyle::Lines));
    m_style->addItem(i18nc("@item:inlistbox grid style", "Dots"), static_cast<int>(DrawyGlobalConfig::EnumGridStyle::Dots));

    auto mainLayout{new QVBoxLayout(this)};
    mainLayout->setObjectName(u"mainLayout"_s);

    auto formLayout{new QFormLayout};
    formLayout->setObjectName(u"formLayout"_s);
    formLayout->addRow(m_showGrid);
    formLayout->addRow(m_snapToGrid);
    formLayout->addRow(i18nc("@label:spinbox", "Grid size:"), m_gridSize);
    formLayout->addRow(i18nc("@label:spinbox", "Major line every:"), m_majorLineEvery);
    formLayout->addRow(i18nc("@label:listbox", "Style:"), m_style);
    mainLayout->addLayout(formLayout);

    auto hint{new QLabel(i18nc("@info", "Hold Ctrl while drawing, moving or resizing to temporarily invert snapping."), this)};
    hint->setObjectName(u"hint"_s);
    hint->setWordWrap(true);
    mainLayout->addWidget(hint);

    mainLayout->addStretch(1);
}

ConfigureGridWidget::~ConfigureGridWidget() = default;

void ConfigureGridWidget::save()
{
    auto config{DrawyGlobalConfig::self()};
    config->setShowGrid(m_showGrid->isChecked());
    config->setSnapToGrid(m_snapToGrid->isChecked());
    config->setGridSize(m_gridSize->value());
    config->setGridMajorLineEvery(m_majorLineEvery->value());
    config->setGridStyle(m_style->currentData().toInt());
    config->save();
}

void ConfigureGridWidget::load()
{
    const auto config{DrawyGlobalConfig::self()};
    m_showGrid->setChecked(config->showGrid());
    m_snapToGrid->setChecked(config->snapToGrid());
    m_gridSize->setValue(config->gridSize());
    m_majorLineEvery->setValue(config->gridMajorLineEvery());
    m_style->setCurrentIndex(m_style->findData(config->gridStyle()));
}

void ConfigureGridWidget::restoreToDefaults()
{
    const bool useDefaults{DrawyGlobalConfig::self()->useDefaults(true)};
    load();
    DrawyGlobalConfig::self()->useDefaults(useDefaults);
}

#include "moc_configuregridwidget.cpp"
