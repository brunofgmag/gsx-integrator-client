#ifndef GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_DISTRIBUTIONPARSER_H
#define GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_DISTRIBUTIONPARSER_H

#include <QtCore/QByteArray>
#include "../../application/model/Distribution.h"

[[nodiscard]] Distribution ParseDistribution(const QByteArray& json);
[[nodiscard]] Distribution ParseFlightsimToDistribution(const QByteArray& json);

#endif // GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_DISTRIBUTIONPARSER_H
