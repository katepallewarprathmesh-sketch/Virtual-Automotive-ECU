#pragma once

#include <string>
#include <vector>

namespace ecu {

class SecureBoot {
public:
    explicit SecureBoot(const std::string& trustedPublicKeyPem);
    bool verifyFirmware(const std::string& firmwarePath,
                        const std::string& signaturePath,
                        std::string& errorMessage) const;

private:
    std::string trustedPublicKeyPem_;
};

} // namespace ecu
