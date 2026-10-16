#ifndef GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_WINDOWSFILENAME_H
#define GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_WINDOWSFILENAME_H

#include <QtCore/QString>

[[nodiscard]] inline QString WindowsFileName(const QString& path)
{
    return path.mid(qMax(path.lastIndexOf(u'\\'), path.lastIndexOf(u'/')) + 1);
}

#endif // GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_WINDOWSFILENAME_H
