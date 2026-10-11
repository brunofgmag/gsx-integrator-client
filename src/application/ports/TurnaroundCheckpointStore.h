#ifndef GSX_INTEGRATOR_CLIENT_APPLICATION_TURNAROUNDCHECKPOINTSTORE_H
#define GSX_INTEGRATOR_CLIENT_APPLICATION_TURNAROUNDCHECKPOINTSTORE_H

#include <optional>

#include "../model/TurnaroundDocument.h"

class TurnaroundCheckpointStore
{
public:
    virtual ~TurnaroundCheckpointStore() = default;

    [[nodiscard]] virtual std::optional<TurnaroundDocument> Read() const = 0;
    [[nodiscard]] virtual bool Write(const TurnaroundDocument& document) = 0;
    [[nodiscard]] virtual bool Erase() = 0;
};

#endif // GSX_INTEGRATOR_CLIENT_APPLICATION_TURNAROUNDCHECKPOINTSTORE_H
