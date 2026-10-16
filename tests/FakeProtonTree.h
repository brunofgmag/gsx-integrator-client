#ifndef GSX_INTEGRATOR_CLIENT_TESTS_FAKEPROTONTREE_H
#define GSX_INTEGRATOR_CLIENT_TESTS_FAKEPROTONTREE_H

#include <QtCore/QByteArray>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QString>

namespace FakeProtonTree
{
    inline constexpr auto kFolder2024 = "Microsoft Flight Simulator 2024";
    inline constexpr auto kFolder2020 = "Microsoft Flight Simulator";

    struct Tree
    {
        QString home;
        QString defaultSteamapps;
        QString secondLibrary;
        QString prefix2020;
        QString prefix2024;
        QString userHome2020;
        QString userHome2024;
    };

    inline void WriteFile(const QString& path, const QByteArray& content)
    {
        QDir().mkpath(QFileInfo(path).absolutePath());

        QFile file(path);
        if (file.open(QIODevice::WriteOnly | QIODevice::Truncate))
        {
            file.write(content);
        }
    }

    inline QByteArray ReadFile(const QString& path)
    {
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly))
        {
            return {};
        }

        return file.readAll();
    }

    inline void Link(const QString& target, const QString& link)
    {
        QDir().mkpath(QFileInfo(link).absolutePath());

        QFile::link(target, link);
    }

    inline void MakePrefix(const QString& prefix, const QString& drive, const QString& driveTarget)
    {
        QDir().mkpath(prefix + QStringLiteral("/drive_c/users/steamuser/AppData/Roaming"));
        QDir().mkpath(prefix + QStringLiteral("/drive_c/users/steamuser/AppData/Local"));
        Link(QStringLiteral("../drive_c"), prefix + QStringLiteral("/dosdevices/c:"));
        Link(driveTarget, prefix + QStringLiteral("/dosdevices/") + drive + QStringLiteral(":"));
        Link(QStringLiteral("/"), prefix + QStringLiteral("/dosdevices/z:"));
    }

    inline Tree Build(const QString& home)
    {
        Tree tree;
        tree.home = home;
        tree.defaultSteamapps = home + QStringLiteral("/.local/share/Steam/steamapps");
        tree.secondLibrary = home + QStringLiteral("/Jogos/SteamLibrary");
        tree.prefix2020 = tree.defaultSteamapps + QStringLiteral("/compatdata/1250410/pfx");
        tree.prefix2024 = tree.secondLibrary + QStringLiteral("/steamapps/compatdata/2537590/pfx");
        tree.userHome2020 = tree.prefix2020 + QStringLiteral("/drive_c/users/steamuser");
        tree.userHome2024 = tree.prefix2024 + QStringLiteral("/drive_c/users/steamuser");

        WriteFile(tree.defaultSteamapps + QStringLiteral("/libraryfolders.vdf"),
                  "\"libraryfolders\"\n{\n"
                  "\t\"0\"\n\t{\n\t\t\"path\"\t\t\"" + QFile::encodeName(home + QStringLiteral("/.local/share/Steam"))
                  + "\"\n\t}\n"
                  "\t\"1\"\n\t{\n\t\t\"path\"\t\t\"" + QFile::encodeName(tree.secondLibrary) + "\"\n\t}\n}\n");
        QDir().mkpath(tree.secondLibrary + QStringLiteral("/steamapps"));
        Link(home + QStringLiteral("/.local/share/Steam"), home + QStringLiteral("/.steam/steam"));

        MakePrefix(tree.prefix2020, QStringLiteral("s"), tree.defaultSteamapps);
        MakePrefix(tree.prefix2024, QStringLiteral("s"), tree.secondLibrary);
        QDir().mkpath(tree.userHome2020 + QStringLiteral("/AppData/Roaming/") + QLatin1String(kFolder2020));
        QDir().mkpath(tree.userHome2024 + QStringLiteral("/AppData/Roaming/") + QLatin1String(kFolder2024));

        return tree;
    }
}

#endif // GSX_INTEGRATOR_CLIENT_TESTS_FAKEPROTONTREE_H
