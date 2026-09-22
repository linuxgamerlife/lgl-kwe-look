#include <QtTest>

#include "core/gsettingsclient.h"

// Only the pure helpers: nothing here talks to the real gsettings.
class TstGSettings : public QObject
{
    Q_OBJECT
private slots:
    void normalizeStripsGVariantQuotes()
    {
        QCOMPARE(GSettings::normalize("'Adwaita'"), QString("Adwaita"));
        QCOMPARE(GSettings::normalize("'Sans 10'\n"), QString("Sans 10"));
        QCOMPARE(GSettings::normalize("24"), QString("24"));
        QCOMPARE(GSettings::normalize("''"), QString(""));
    }

    void floatsCompareNumerically()
    {
        // We send 1.000000, gsettings reads it back as 1.0.
        QVERIFY(GSettings::valuesEquivalent("1.000000", "1.0"));
        QVERIFY(GSettings::valuesEquivalent("1.25", "1.250000"));
        QVERIFY(!GSettings::valuesEquivalent("1.0", "1.5"));
    }

    void stringsCompareExactly()
    {
        QVERIFY(GSettings::valuesEquivalent("Adwaita", "'Adwaita'"));
        QVERIFY(!GSettings::valuesEquivalent("Adwaita", "'Adwaita-dark'"));
        // "10" vs "Sans 10": neither is a plain number match
        QVERIFY(!GSettings::valuesEquivalent("Sans 10", "10"));
    }

    void booleansCompare()
    {
        QVERIFY(GSettings::valuesEquivalent("true", "true"));
        QVERIFY(!GSettings::valuesEquivalent("true", "false"));
    }
};

QTEST_MAIN(TstGSettings)
#include "tst_gsettings.moc"
