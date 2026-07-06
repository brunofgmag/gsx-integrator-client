#ifndef GSX_INTEGRATOR_CLIENT_COMMANDRESULT_H
#define GSX_INTEGRATOR_CLIENT_COMMANDRESULT_H

#include <string>
#include <utility>

struct CommandResult
{
    bool succeeded = false;
    std::string message;

    static CommandResult Success()
    {
        return {true, {}};
    }

    static CommandResult Failure(std::string message)
    {
        return {false, std::move(message)};
    }
};

#endif // GSX_INTEGRATOR_CLIENT_COMMANDRESULT_H
