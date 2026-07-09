#ifndef GSX_INTEGRATOR_CLIENT_UPDATEINFO_H
#define GSX_INTEGRATOR_CLIENT_UPDATEINFO_H

#include <QtCore/QString>

struct UpdateInfo
{
    QString version;
    QString releasePageUrl;
    QString zipUrl;
    QString shaUrl;
    QString zipName;
};

#endif // GSX_INTEGRATOR_CLIENT_UPDATEINFO_H
