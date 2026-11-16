/*
 * SPDX-FileCopyrightText: 2026 Rick Christy <grymmjack@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#include "mimemanagertest.hpp"
#include "common/constants.hpp"
#include "item/rectangle.hpp"
#include "mime/mimemanager.hpp"
#include <QBuffer>
#include <QImage>
#include <QMimeData>
#include <QTest>
#include <memory>
QTEST_MAIN(MimeManagerTest)

namespace
{
std::shared_ptr<RectangleItem> makeRectangle()
{
    auto rectangle{std::make_shared<RectangleItem>()};
    rectangle->setStart(QPointF{10, 20});
    rectangle->setEnd(QPointF{110, 80});
    rectangle->setProperty(Property::Type::StrokeColor, Property{QColor(Qt::red), Property::Type::StrokeColor});
    return rectangle;
}

// What a clipboard manager such as Klipper hands back after taking the clipboard
// over: only the image, re-encoded.
std::unique_ptr<QMimeData> imageOnlyRoundTrip(const QMimeData &original)
{
    QByteArray png;
    QBuffer buffer{&png};
    buffer.open(QIODevice::WriteOnly);
    qvariant_cast<QImage>(original.imageData()).save(&buffer, "PNG");

    auto result{std::make_unique<QMimeData>()};
    result->setImageData(QImage::fromData(png, "PNG"));
    return result;
}
} // namespace

MimeManagerTest::MimeManagerTest(QObject *parent)
    : QObject{parent}
{
}

void MimeManagerTest::shouldRoundTripNativeItemsWithNewIds()
{
    MimeManager manager;
    const auto rectangle{makeRectangle()};

    QMimeData data;
    manager.writeData(data, {rectangle});
    QVERIFY(data.hasFormat(Common::drawyClipboardMime));

    const auto first{manager.readData(data)};
    const auto second{manager.readData(data)};
    QCOMPARE(first.size(), 1);
    QCOMPARE(second.size(), 1);

    QCOMPARE(first[0]->formType(), Item::FormType::Rectangle);
    QCOMPARE(first[0]->boundingBox(), rectangle->boundingBox());
    QCOMPARE(first[0]->property(Property::Type::StrokeColor).value<QColor>(), QColor(Qt::red));

    // pasting the same data twice gives two distinct items, neither the original
    QVERIFY(first[0]->id() != rectangle->id());
    QVERIFY(second[0]->id() != rectangle->id());
    QVERIFY(first[0]->id() != second[0]->id());
}

void MimeManagerTest::shouldPasteNativeItemsWhenOnlyOurImageComesBack()
{
    MimeManager manager;
    const auto rectangle{makeRectangle()};

    QMimeData data;
    manager.writeData(data, {rectangle});
    QVERIFY(data.hasImage());

    const auto managed{imageOnlyRoundTrip(data)};
    QVERIFY(!managed->hasFormat(Common::drawyClipboardMime));

    const auto items{manager.readData(*managed)};
    QCOMPARE(items.size(), 1);
    QCOMPARE(items[0]->formType(), Item::FormType::Rectangle);
    QCOMPARE(items[0]->boundingBox(), rectangle->boundingBox());
}

void MimeManagerTest::shouldNotUseNativeItemsForOtherContent()
{
    MimeManager manager;
    QMimeData data;
    manager.writeData(data, {makeRectangle()});

    // something else copied afterwards must paste as itself
    QMimeData other;
    other.setText(QStringLiteral("hello"));
    const auto items{manager.readData(other)};
    QCOMPARE(items.size(), 1);
    QCOMPARE(items[0]->formType(), Item::FormType::Text);
}

#include "moc_mimemanagertest.cpp"
