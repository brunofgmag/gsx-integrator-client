#include "ExeXml.h"

#include <algorithm>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QList>
#include <QtXml/QDomDocument>
#include "SimulatorCandidates.h"

namespace
{
    const auto kLaunchAddonTag = QStringLiteral("Launch.Addon");
    const auto kDisabledTag = QStringLiteral("Disabled");
    const auto kExeXmlName = QStringLiteral("EXE.xml");

    bool LoadExisting(const QString& path, QDomDocument* doc)
    {
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        {
            return false;
        }

        return static_cast<bool>(doc->setContent(&file));
    }

    bool LoadOrCreate(const QString& path, QDomDocument* doc)
    {
        if (QFile::exists(path))
        {
            return LoadExisting(path, doc);
        }

        if (!QFileInfo(QFileInfo(path).absolutePath()).isDir())
        {
            return false;
        }

        return static_cast<bool>(doc->setContent(QStringLiteral(
            "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
            "<SimBase.Document Type=\"Launch\" version=\"1,0\">\n"
            "  <Descr>Launch</Descr>\n"
            "  <Filename>EXE.xml</Filename>\n"
            "  <Disabled>False</Disabled>\n"
            "  <Launch.ManualLoad>False</Launch.ManualLoad>\n"
            "</SimBase.Document>\n")));
    }

    bool Save(const QDomDocument& doc, const QString& path)
    {
        QFile file(path);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text))
        {
            return false;
        }

        return file.write(doc.toByteArray(2)) >= 0;
    }

    void SetChildText(QDomDocument& doc, QDomElement& parent, const QString& tag, const QString& text)
    {
        QDomElement element = parent.firstChildElement(tag);
        if (element.isNull())
        {
            element = doc.createElement(tag);
            parent.appendChild(element);
        }

        while (!element.firstChild().isNull())
        {
            element.removeChild(element.firstChild());
        }

        element.appendChild(doc.createTextNode(text));
    }

    void FillAddon(QDomDocument& doc, QDomElement& addon, const QString& exePath, const QString& appName)
    {
        SetChildText(doc, addon, kDisabledTag, QStringLiteral("False"));
        SetChildText(doc, addon, QStringLiteral("ManualLoad"), QStringLiteral("False"));
        SetChildText(doc, addon, QStringLiteral("Name"), appName);
        SetChildText(doc, addon, QStringLiteral("Path"), exePath);
        SetChildText(doc, addon, QStringLiteral("CommandLine"), QStringLiteral("--tray"));
    }

    bool IsEnabled(const QDomElement& addon)
    {
        const QString disabled = addon.firstChildElement(kDisabledTag).text().trimmed();

        return disabled.compare(QStringLiteral("True"), Qt::CaseInsensitive) != 0;
    }

    QList<QDomElement> AddonsLaunching(const QDomElement& root, const QString& exeName)
    {
        QList<QDomElement> addons;
        for (QDomElement addon = root.firstChildElement(kLaunchAddonTag); !addon.isNull();
             addon = addon.nextSiblingElement(kLaunchAddonTag))
        {
            if (addon.text().contains(exeName, Qt::CaseInsensitive))
            {
                addons.append(addon);
            }
        }

        return addons;
    }
}

bool ExeXmlAddUpdate(const QString& exeXmlPath, const QString& exePath, const QString& appName)
{
    QDomDocument doc;
    if (!LoadOrCreate(exeXmlPath, &doc))
    {
        return false;
    }

    QDomElement root = doc.documentElement();
    if (root.isNull())
    {
        return false;
    }

    QList<QDomElement> launching = AddonsLaunching(root, QFileInfo(exePath).fileName());
    for (QDomElement& addon : launching)
    {
        FillAddon(doc, addon, exePath, appName);
    }

    if (launching.isEmpty())
    {
        QDomElement addon = doc.createElement(kLaunchAddonTag);
        FillAddon(doc, addon, exePath, appName);
        root.appendChild(addon);
    }

    return Save(doc, exeXmlPath);
}

bool ExeXmlRemove(const QString& exeXmlPath, const QString& exeName)
{
    if (!QFile::exists(exeXmlPath))
    {
        return true;
    }

    QDomDocument doc;
    if (!LoadExisting(exeXmlPath, &doc))
    {
        return false;
    }

    QDomElement root = doc.documentElement();
    if (root.isNull())
    {
        return false;
    }

    const QList<QDomElement> obsolete = AddonsLaunching(root, exeName);
    if (obsolete.isEmpty())
    {
        return true;
    }

    for (const QDomElement& addon : obsolete)
    {
        root.removeChild(addon);
    }

    return Save(doc, exeXmlPath);
}

bool ExeXmlHasEnabledEntry(const QString& exeXmlPath, const QString& exeName)
{
    QDomDocument doc;
    if (!LoadExisting(exeXmlPath, &doc))
    {
        return false;
    }

    return std::ranges::any_of(AddonsLaunching(doc.documentElement(), exeName), IsEnabled);
}

std::vector<ExeXmlTarget> CandidateExeXmlTargets(const QString& homeDir)
{
    std::vector<ExeXmlTarget> targets;
    for (const SimulatorCandidate& candidate : kSimulatorCandidates)
    {
        const QFileInfo userCfg(homeDir + u'/' + QLatin1String(candidate.userCfgSubPath));
        if (!QFileInfo(userCfg.absolutePath()).isDir())
        {
            continue;
        }

        targets.push_back({
            QLatin1String(candidate.label),
            QDir::toNativeSeparators(userCfg.absolutePath() + u'/' + kExeXmlName)
        });
    }

    return targets;
}
