#ifndef GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_STOREDTEXT_H
#define GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_STOREDTEXT_H

#include <string>

#include <QString>

namespace checkpoint
{
    [[nodiscard]] inline QString ToStoredString(const std::string& text)
    {
        return QString::fromStdString(text);
    }

    [[nodiscard]] inline std::string StoredText(const std::string& text)
    {
        return ToStoredString(text).toStdString();
    }
}

#endif // GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_STOREDTEXT_H
