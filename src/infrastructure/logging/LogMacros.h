#ifndef GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_LOGMACROS_H
#define GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_LOGMACROS_H

#include <QtCore/QtLogging>

#define GSXI_TAG "[GSX Integrator] "

#define LOG_INFO(fmt, ...) qInfo(GSXI_TAG fmt, ##__VA_ARGS__)
#define LOG_WARN(fmt, ...) qWarning(GSXI_TAG "WARN: " fmt, ##__VA_ARGS__)
#define LOG_ERROR(fmt, ...) qCritical(GSXI_TAG "ERROR: " fmt, ##__VA_ARGS__)

#endif // GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_LOGMACROS_H
