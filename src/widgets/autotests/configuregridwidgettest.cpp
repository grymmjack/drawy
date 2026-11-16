/*
 * SPDX-FileCopyrightText: 2026 Rick Christy <grymmjack@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "configuregridwidgettest.hpp"
#include "dialog/grid/configuregridwidget.hpp"
#include "drawyglobalconfig.h"
#include <QCheckBox>
#include <QComboBox>
#include <QSpinBox>
#include <QStandardPaths>
#include <QTest>
#include <QVBoxLayout>

QTEST_MAIN(ConfigureGridWidgetTest)
using namespace Qt::Literals::StringLiterals;
ConfigureGridWidgetTest::ConfigureGridWidgetTest(QObject *parent)
    : QObject{parent}
{
    QStandardPaths::setTestModeEnabled(true);
}

void ConfigureGridWidgetTest::shouldHaveDefaultValues()
{
    ConfigureGridWidget w;
    w.restoreToDefaults();

    QVERIFY(w.findChild<QVBoxLayout *>(u"mainLayout"_s));

    auto m_showGrid = w.findChild<QCheckBox *>(u"m_showGrid"_s);
    QVERIFY(m_showGrid);
    QVERIFY(!m_showGrid->isChecked());

    auto m_snapToGrid = w.findChild<QCheckBox *>(u"m_snapToGrid"_s);
    QVERIFY(m_snapToGrid);
    QVERIFY(!m_snapToGrid->isChecked());

    auto m_gridSize = w.findChild<QSpinBox *>(u"m_gridSize"_s);
    QVERIFY(m_gridSize);
    QCOMPARE(m_gridSize->value(), 20);
    QCOMPARE(m_gridSize->minimum(), 2);
    QCOMPARE(m_gridSize->maximum(), 500);

    auto m_majorLineEvery = w.findChild<QSpinBox *>(u"m_majorLineEvery"_s);
    QVERIFY(m_majorLineEvery);
    QCOMPARE(m_majorLineEvery->value(), 5);
    QCOMPARE(m_majorLineEvery->minimum(), 0);
    QVERIFY(!m_majorLineEvery->specialValueText().isEmpty());

    auto m_style = w.findChild<QComboBox *>(u"m_style"_s);
    QVERIFY(m_style);
    QCOMPARE(m_style->count(), 2);
    QCOMPARE(m_style->currentData().toInt(), static_cast<int>(DrawyGlobalConfig::EnumGridStyle::Lines));
}

void ConfigureGridWidgetTest::shouldSaveAndLoadSettings()
{
    {
        ConfigureGridWidget w;
        w.findChild<QCheckBox *>(u"m_showGrid"_s)->setChecked(true);
        w.findChild<QCheckBox *>(u"m_snapToGrid"_s)->setChecked(true);
        w.findChild<QSpinBox *>(u"m_gridSize"_s)->setValue(32);
        w.findChild<QSpinBox *>(u"m_majorLineEvery"_s)->setValue(0);
        auto style = w.findChild<QComboBox *>(u"m_style"_s);
        style->setCurrentIndex(style->findData(static_cast<int>(DrawyGlobalConfig::EnumGridStyle::Dots)));
        w.save();
    }

    auto config{DrawyGlobalConfig::self()};
    QVERIFY(config->showGrid());
    QVERIFY(config->snapToGrid());
    QCOMPARE(config->gridSize(), 32);
    QCOMPARE(config->gridMajorLineEvery(), 0);
    QCOMPARE(config->gridStyle(), static_cast<int>(DrawyGlobalConfig::EnumGridStyle::Dots));

    ConfigureGridWidget reloaded;
    reloaded.load();
    QCOMPARE(reloaded.findChild<QSpinBox *>(u"m_gridSize"_s)->value(), 32);
    QVERIFY(reloaded.findChild<QCheckBox *>(u"m_snapToGrid"_s)->isChecked());
    QCOMPARE(reloaded.findChild<QComboBox *>(u"m_style"_s)->currentData().toInt(), static_cast<int>(DrawyGlobalConfig::EnumGridStyle::Dots));
}

void ConfigureGridWidgetTest::shouldRestoreDefaults()
{
    ConfigureGridWidget w;
    w.findChild<QSpinBox *>(u"m_gridSize"_s)->setValue(77);
    w.restoreToDefaults();
    QCOMPARE(w.findChild<QSpinBox *>(u"m_gridSize"_s)->value(), 20);
    QVERIFY(!w.findChild<QCheckBox *>(u"m_showGrid"_s)->isChecked());
}

#include "moc_configuregridwidgettest.cpp"
