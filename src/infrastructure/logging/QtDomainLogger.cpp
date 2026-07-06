#include "QtDomainLogger.h"

#include <string>
#include "LogMacros.h"

void QtDomainLogger::LogInfo(const std::string_view message)
{
    LOG_INFO("%s", std::string(message).c_str());
}
