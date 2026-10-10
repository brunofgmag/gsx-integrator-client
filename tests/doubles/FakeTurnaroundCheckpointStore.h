#ifndef GSX_INTEGRATOR_CLIENT_TESTS_FAKETURNAROUNDCHECKPOINTSTORE_H
#define GSX_INTEGRATOR_CLIENT_TESTS_FAKETURNAROUNDCHECKPOINTSTORE_H

#include "../../src/application/ports/TurnaroundCheckpointStore.h"

class FakeTurnaroundCheckpointStore final : public TurnaroundCheckpointStore
{
public:
    std::optional<TurnaroundDocument> stored;
    bool writeResult = true;
    bool eraseResult = true;
    mutable int readCalls = 0;
    int writeCalls = 0;
    int eraseCalls = 0;

    [[nodiscard]] std::optional<TurnaroundDocument> Read() const override
    {
        ++readCalls;

        return stored;
    }

    [[nodiscard]] bool Write(const TurnaroundDocument& document) override
    {
        ++writeCalls;
        if (writeResult)
        {
            stored = document;
        }

        return writeResult;
    }

    [[nodiscard]] bool Erase() override
    {
        ++eraseCalls;
        if (eraseResult)
        {
            stored.reset();
        }

        return eraseResult;
    }
};

#endif // GSX_INTEGRATOR_CLIENT_TESTS_FAKETURNAROUNDCHECKPOINTSTORE_H
