#include <QtTest>

#include "core/xcursor.h"

// Builds Xcursor files by hand, so the decoder is checked against the format rather than
// against itself.
class TstXCursor : public QObject
{
    Q_OBJECT

    static void put32(QByteArray &b, quint32 v)
    {
        for (int i = 0; i < 4; ++i)
            b.append(char((v >> (8 * i)) & 0xff));
    }

    // One image chunk per entry: {nominal size, width, height, xhot, yhot, delay, argb}
    struct Img { quint32 size, w, h, xhot, yhot, delay, argb; };

    static QByteArray build(const QList<Img> &imgs)
    {
        QByteArray file("Xcur");
        put32(file, 16);                    // header size
        put32(file, 0x10000);               // version
        put32(file, quint32(imgs.size()));  // ntoc

        QByteArray chunks;
        const quint32 firstChunk = 16 + quint32(imgs.size()) * 12;
        for (const Img &i : imgs) {
            put32(file, 0xfffd0002u);       // type: image
            put32(file, i.size);            // subtype: nominal size
            put32(file, firstChunk + quint32(chunks.size()));

            put32(chunks, 36);              // chunk header size
            put32(chunks, 0xfffd0002u);
            put32(chunks, i.size);
            put32(chunks, 1);               // version
            put32(chunks, i.w);
            put32(chunks, i.h);
            put32(chunks, i.xhot);
            put32(chunks, i.yhot);
            put32(chunks, i.delay);
            for (quint32 p = 0; p < i.w * i.h; ++p)
                put32(chunks, i.argb);
        }
        return file + chunks;
    }

private slots:
    void decodesASingleImage()
    {
        const auto frames = XCursor::decode(build({{24, 2, 2, 1, 1, 0, 0xff102030u}}));
        QCOMPARE(frames.size(), 1);
        QCOMPARE(frames.first().nominalSize, 24);
        QCOMPARE(frames.first().xhot, 1);
        QCOMPARE(frames.first().image.size(), QSize(2, 2));
        QCOMPARE(frames.first().image.pixel(0, 0), 0xff102030u);
    }

    void keepsPremultipliedPixels()
    {
        // Xcursor stores premultiplied ARGB: 50% alpha white is 0x80808080.
        const auto frames = XCursor::decode(build({{16, 1, 1, 0, 0, 0, 0x80808080u}}));
        QCOMPARE(frames.size(), 1);
        QCOMPARE(frames.first().image.format(), QImage::Format_ARGB32_Premultiplied);
        QCOMPARE(frames.first().image.constScanLine(0)[0], uchar(0x80));
        QCOMPARE(qAlpha(frames.first().image.pixel(0, 0)), 0x80);
    }

    void bestFramePicksTheNearestSize()
    {
        const auto frames = XCursor::decode(build({{16, 1, 1, 0, 0, 0, 0xff000001u},
                                                   {32, 1, 1, 0, 0, 0, 0xff000002u},
                                                   {48, 1, 1, 0, 0, 0, 0xff000003u}}));
        QCOMPARE(frames.size(), 3);
        QCOMPARE(XCursor::bestFrame(frames, 30).nominalSize, 32);
        QCOMPARE(XCursor::bestFrame(frames, 8).nominalSize, 16);
        QCOMPARE(XCursor::bestFrame(frames, 96).nominalSize, 48);
    }

    void animatedCursorKeepsFirstFrameOfTheChosenSize()
    {
        const auto frames = XCursor::decode(build({{24, 1, 1, 0, 0, 50, 0xff000001u},
                                                   {24, 1, 1, 0, 0, 50, 0xff000002u}}));
        QCOMPARE(frames.size(), 2);
        QCOMPARE(frames.first().delayMs, 50);
        QCOMPARE(XCursor::bestFrame(frames, 24).image.pixel(0, 0), 0xff000001u);
    }

    void rejectsGarbage()
    {
        QVERIFY(XCursor::decode(QByteArray()).isEmpty());
        QVERIFY(XCursor::decode("not a cursor at all, definitely").isEmpty());
    }

    void truncatedFileNeverCrashes()
    {
        const QByteArray good = build({{24, 4, 4, 0, 0, 0, 0xff00ff00u}});
        for (int cut = 0; cut < good.size(); ++cut)
            XCursor::decode(good.left(cut));   // must return, whatever it returns
        QVERIFY(XCursor::decode(good.left(good.size() - 4)).isEmpty());
    }

    void hostileHeaderIsBounded()
    {
        QByteArray file("Xcur");
        put32(file, 16);
        put32(file, 0x10000);
        put32(file, 0xffffffffu);   // absurd table of contents
        file.append(QByteArray(64, '\0'));
        QVERIFY(XCursor::decode(file).isEmpty());

        // An image claiming a huge size with almost no pixel data.
        QByteArray big = build({{24, 1, 1, 0, 0, 0, 0}});
        big.replace(16 + 12 + 16, 4, QByteArray("\xff\xff\xff\x7f", 4));   // width
        QVERIFY(XCursor::decode(big).isEmpty());
    }
};

QTEST_MAIN(TstXCursor)
#include "tst_xcursor.moc"
