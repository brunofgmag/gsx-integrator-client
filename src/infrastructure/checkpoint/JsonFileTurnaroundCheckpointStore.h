#ifndef GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_JSONFILETURNAROUNDCHECKPOINTSTORE_H
#define GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_JSONFILETURNAROUNDCHECKPOINTSTORE_H

#include <QString>

#include "../../application/ports/TurnaroundCheckpointStore.h"

class JsonFileTurnaroundCheckpointStore final : public TurnaroundCheckpointStore
{
public:
    JsonFileTurnaroundCheckpointStore(QString directory, QString clientVersion);

    [[nodiscard]] std::optional<TurnaroundDocument> Read() const override;
    [[nodiscard]] bool Write(const TurnaroundDocument& document) override;
    [[nodiscard]] bool Erase() override;

private:
    [[nodiscard]] QString FilePath() const;

    QString directory_;
    QString clientVersion_;
};

#endif // GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_JSONFILETURNAROUNDCHECKPOINTSTORE_H
