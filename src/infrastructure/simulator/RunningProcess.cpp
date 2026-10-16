#include "RunningProcess.h"

#include <algorithm>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QList>

#ifdef Q_OS_WIN
#include <windows.h>
#include <tlhelp32.h>
#endif

namespace
{
    bool IsProcessId(const QString& name)
    {
        return !name.isEmpty() && std::ranges::all_of(name, [](const QChar letter)
        {
            return letter.isDigit();
        });
    }

    QString BaseName(const QByteArray& argument)
    {
        const QString text = QString::fromUtf8(argument);

        return text.mid(qMax(text.lastIndexOf(u'\\'), text.lastIndexOf(u'/')) + 1);
    }

    bool RunsExecutable(const QString& cmdlinePath, const QString& exeName)
    {
        QFile file(cmdlinePath);
        if (!file.open(QIODevice::ReadOnly))
        {
            return false;
        }

        const QList<QByteArray> arguments = file.readAll().split('\0');

        return std::ranges::any_of(arguments, [&exeName](const QByteArray& argument)
        {
            return exeName.compare(BaseName(argument), Qt::CaseInsensitive) == 0;
        });
    }
}

bool IsProcessRunningUnder(const QString& procRoot, const QString& exeName)
{
    const QStringList entries = QDir(procRoot).entryList(QDir::Dirs | QDir::NoDotAndDotDot);

    return std::ranges::any_of(entries, [&procRoot, &exeName](const QString& entry)
    {
        return IsProcessId(entry) && RunsExecutable(procRoot + u'/' + entry + QStringLiteral("/cmdline"), exeName);
    });
}

#ifdef Q_OS_WIN
bool IsProcessRunning(const QString& exeName)
{
    const HANDLE& snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE)
    {
        return false;
    }

    PROCESSENTRY32W entry{};
    entry.dwSize = sizeof(entry);

    bool found = false;
    if (Process32FirstW(snapshot, &entry) != FALSE)
    {
        do
        {
            if (exeName.compare(QString::fromWCharArray(entry.szExeFile), Qt::CaseInsensitive) == 0)
            {
                found = true;
                break;
            }
        } while (Process32NextW(snapshot, &entry) != FALSE);
    }
    CloseHandle(snapshot);

    return found;
}
#else
bool IsProcessRunning(const QString& exeName)
{
    return IsProcessRunningUnder(QStringLiteral("/proc"), exeName);
}
#endif
