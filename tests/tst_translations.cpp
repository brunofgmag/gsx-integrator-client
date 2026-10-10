#include <array>

#include <QtCore/QFile>
#include <QtCore/QHash>
#include <QtCore/QList>
#include <QtCore/QXmlStreamReader>
#include <QtTest/QTest>

namespace
{
    struct Phrase
    {
        const char* context;
        const char* source;
    };

    struct Translation
    {
        QString text;
        QString type;
    };

    using Catalog = QMultiHash<QString, Translation>;

    constexpr std::array kResumePhrases = {
        Phrase{.context = "Turnaround", .source = "The client is waiting for GSX to send its state."},
        Phrase{.context = "Turnaround", .source = "The client found saved turnaround data and is waiting for the aircraft and GSX to check whether it belongs to this flight."},
        Phrase{.context = "Turnaround", .source = "The client is resuming the saved turnaround and waiting for GSX and the simulator to report their state."},
        Phrase{.context = "Turnaround", .source = "The client is resuming the saved turnaround and waiting for the aircraft to respond. If it never does, restart the flow."},
        Phrase{.context = "OperationsScreen", .source = "GSX restarted since this turnaround was saved. Resume it if the aircraft is still as you left it, or restart the flow to start over."},
        Phrase{.context = "OperationsScreen", .source = "Resume turnaround"},
        Phrase{.context = "Integrator", .source = "There is no saved turnaround waiting for an answer."},
        Phrase{.context = "AutomationPane", .source = "Skip repositioning on a new turnaround"},
        Phrase{.context = "AutomationPane", .source = "After deboarding at the destination, the client starts the next turnaround without repositioning the aircraft."},
        Phrase{.context = "ProfilesPane", .source = "Skip repositioning on a new turnaround"},
        Phrase{.context = "ProfilesPane", .source = "After deboarding at the destination, the client starts the next turnaround without repositioning the aircraft."}
    };

    QString KeyOf(const QString& context, const QString& source)
    {
        return context + QLatin1Char('\n') + source;
    }

    Catalog ReadCatalog(const char* fileName)
    {
        Catalog catalog;
        QFile file(QString::fromUtf8(GSXI_I18N_DIR) + QLatin1Char('/') + QString::fromUtf8(fileName));
        if (!file.open(QIODevice::ReadOnly))
        {
            return catalog;
        }

        QXmlStreamReader xml(&file);
        QString context;
        QString source;
        while (!xml.atEnd())
        {
            xml.readNext();
            if (!xml.isStartElement())
            {
                continue;
            }

            if (xml.name() == QLatin1String("name"))
            {
                context = xml.readElementText();
            }
            else if (xml.name() == QLatin1String("source"))
            {
                source = xml.readElementText();
            }
            else if (xml.name() == QLatin1String("translation"))
            {
                Translation translation;
                translation.type = xml.attributes().value(QLatin1String("type")).toString();
                translation.text = xml.readElementText();
                catalog.insert(KeyOf(context, source), translation);
            }
        }

        return catalog;
    }
}

class TranslationsTest final : public QObject
{
    Q_OBJECT

private slots:
    static void everyResumePhraseHasAFinishedEnglishEntry();
    static void everyResumePhraseHasAFinishedPortugueseEntryThatIsNotTheEnglishLeftOver();
};

void TranslationsTest::everyResumePhraseHasAFinishedEnglishEntry()
{
    const Catalog english = ReadCatalog("app_en.ts");

    QVERIFY(!english.isEmpty());

    for (const Phrase& phrase : kResumePhrases)
    {
        const QString source = QString::fromUtf8(phrase.source);
        const QList<Translation> entries = english.values(KeyOf(QString::fromUtf8(phrase.context), source));

        QVERIFY2(!entries.isEmpty(), phrase.source);
        for (const Translation& entry : entries)
        {
            QVERIFY2(entry.type.isEmpty(), phrase.source);
            QCOMPARE(entry.text, source);
        }
    }
}

void TranslationsTest::everyResumePhraseHasAFinishedPortugueseEntryThatIsNotTheEnglishLeftOver()
{
    const Catalog portuguese = ReadCatalog("app_pt_BR.ts");

    QVERIFY(!portuguese.isEmpty());

    for (const Phrase& phrase : kResumePhrases)
    {
        const QString source = QString::fromUtf8(phrase.source);
        const QList<Translation> entries = portuguese.values(KeyOf(QString::fromUtf8(phrase.context), source));

        QVERIFY2(!entries.isEmpty(), phrase.source);
        for (const Translation& entry : entries)
        {
            QVERIFY2(entry.type.isEmpty(), phrase.source);
            QVERIFY2(!entry.text.isEmpty(), phrase.source);
            QVERIFY2(entry.text != source, phrase.source);
        }
    }
}

QTEST_APPLESS_MAIN(TranslationsTest)

#include "tst_translations.moc"
