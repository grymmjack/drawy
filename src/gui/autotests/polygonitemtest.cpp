/*
 * SPDX-FileCopyrightText: 2026 Laurent Montel <montel@kde.org>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "polygonitemtest.hpp"
#include "drawy_autotest_helper.hpp"
#include "item/polygon.hpp"
#include "item/rectangle.hpp"
#include <QTest>

QTEST_MAIN(PolygonItemTest)
using namespace Qt::Literals::StringLiterals;
PolygonItemTest::PolygonItemTest(QObject *parent)
    : QObject{parent}
{
}

void PolygonItemTest::shouldHaveDefaultValues()
{
}

void PolygonItemTest::shouldExcludeStrokeFromGeometryBox()
{
    RectangleItem r;
    r.setProperty(Property::Type::StrokeWidth, Property(6, Property::Type::StrokeWidth));
    r.setStart(QPointF(60, 40));
    r.setEnd(QPointF(20, 80));

    // the bounding box is padded by the stroke width, the geometry box is not
    QCOMPARE(r.boundingBox(), QRectF(14, 34, 52, 52));
    QCOMPARE(r.normalizedGeometryBox(), QRectF(20, 40, 40, 40));
    QCOMPARE(r.geometryBoundingBox(), QRectF(20, 40, 40, 40));

    r.translate(QPointF(5, -5));
    QCOMPARE(r.geometryBoundingBox(), QRectF(25, 35, 40, 40));
}

#include "moc_polygonitemtest.cpp"
