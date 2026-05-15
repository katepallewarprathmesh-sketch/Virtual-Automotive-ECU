#include "SecureBoot.h"
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/sha.h>
#include <fstream>
#include <sstream>

namespace ecu {

SecureBoot::SecureBoot(const std::string& trustedPublicKeyPem)
    : trustedPublicKeyPem_(trustedPublicKeyPem) {
}

static bool readFile(const std::string& path, std::vector<uint8_t>& output) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        return false;
    }
    output.assign(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
    return true;
}

bool SecureBoot::verifyFirmware(const std::string& firmwarePath,
                                const std::string& signaturePath,
                                std::string& errorMessage) const {
    std::vector<uint8_t> firmware;
    if (!readFile(firmwarePath, firmware)) {
        errorMessage = "unable to read firmware file: " + firmwarePath;
        return false;
    }

    std::vector<uint8_t> signature;
    if (!readFile(signaturePath, signature)) {
        errorMessage = "unable to read signature file: " + signaturePath;
        return false;
    }

    BIO* bio = BIO_new_mem_buf(trustedPublicKeyPem_.data(), trustedPublicKeyPem_.size());
    if (!bio) {
        errorMessage = "failed to create OpenSSL BIO";
        return false;
    }

    EVP_PKEY* pubkey = PEM_read_bio_PUBKEY(bio, nullptr, nullptr, nullptr);
    BIO_free(bio);
    if (!pubkey) {
        errorMessage = "failed to load trusted public key";
        return false;
    }

    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    if (!ctx) {
        EVP_PKEY_free(pubkey);
        errorMessage = "failed to create OpenSSL digest context";
        return false;
    }

    bool verified = false;
    do {
        if (EVP_DigestVerifyInit(ctx, nullptr, EVP_sha256(), nullptr, pubkey) != 1) {
            errorMessage = "DigestVerifyInit failed";
            break;
        }
        if (EVP_DigestVerifyUpdate(ctx, firmware.data(), firmware.size()) != 1) {
            errorMessage = "DigestVerifyUpdate failed";
            break;
        }
        if (EVP_DigestVerifyFinal(ctx, signature.data(), signature.size()) != 1) {
            errorMessage = "firmware signature verification failed";
            break;
        }
        verified = true;
    } while (false);

    EVP_MD_CTX_free(ctx);
    EVP_PKEY_free(pubkey);
    return verified;
}

} // namespace ecu
