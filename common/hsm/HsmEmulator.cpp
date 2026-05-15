#include "HsmEmulator.h"
#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <openssl/rand.h>
#include <openssl/pem.h>

namespace ecu {

HsmEmulator::HsmEmulator() = default;

bool HsmEmulator::generateRsaKey(const std::string& keyId, int bits) {
    EVP_PKEY* pkey = nullptr;
    EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new_id(EVP_PKEY_RSA, nullptr);
    if (!ctx) {
        return false;
    }

    if (EVP_PKEY_keygen_init(ctx) <= 0) {
        EVP_PKEY_CTX_free(ctx);
        return false;
    }
    if (EVP_PKEY_CTX_set_rsa_keygen_bits(ctx, bits) <= 0) {
        EVP_PKEY_CTX_free(ctx);
        return false;
    }

    if (EVP_PKEY_keygen(ctx, &pkey) <= 0) {
        EVP_PKEY_CTX_free(ctx);
        return false;
    }
    EVP_PKEY_CTX_free(ctx);

    BIO* privateBio = BIO_new(BIO_s_mem());
    BIO* publicBio = BIO_new(BIO_s_mem());
    if (!privateBio || !publicBio) {
        EVP_PKEY_free(pkey);
        BIO_free_all(privateBio);
        BIO_free_all(publicBio);
        return false;
    }

    if (PEM_write_bio_PrivateKey(privateBio, pkey, nullptr, nullptr, 0, nullptr, nullptr) != 1 ||
        PEM_write_bio_PUBKEY(publicBio, pkey) != 1) {
        EVP_PKEY_free(pkey);
        BIO_free_all(privateBio);
        BIO_free_all(publicBio);
        return false;
    }

    char* privateData = nullptr;
    long privateLen = BIO_get_mem_data(privateBio, &privateData);
    char* publicData = nullptr;
    long publicLen = BIO_get_mem_data(publicBio, &publicData);

    privateKeys_[keyId] = std::string(privateData, privateLen);
    publicKeys_[keyId] = std::string(publicData, publicLen);

    EVP_PKEY_free(pkey);
    BIO_free_all(privateBio);
    BIO_free_all(publicBio);
    return true;
}

std::vector<uint8_t> HsmEmulator::sign(const std::string& keyId,
                                       const std::vector<uint8_t>& data,
                                       bool& success) const {
    success = false;
    auto it = privateKeys_.find(keyId);
    if (it == privateKeys_.end()) {
        return {};
    }

    BIO* bio = BIO_new_mem_buf(it->second.data(), it->second.size());
    EVP_PKEY* pkey = PEM_read_bio_PrivateKey(bio, nullptr, nullptr, nullptr);
    BIO_free(bio);
    if (!pkey) {
        return {};
    }

    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    if (!ctx) {
        EVP_PKEY_free(pkey);
        return {};
    }

    if (EVP_DigestSignInit(ctx, nullptr, EVP_sha256(), nullptr, pkey) != 1) {
        EVP_PKEY_free(pkey);
        EVP_MD_CTX_free(ctx);
        return {};
    }

    if (EVP_DigestSignUpdate(ctx, data.data(), data.size()) != 1) {
        EVP_PKEY_free(pkey);
        EVP_MD_CTX_free(ctx);
        return {};
    }

    size_t sigLen = 0;
    if (EVP_DigestSignFinal(ctx, nullptr, &sigLen) != 1) {
        EVP_PKEY_free(pkey);
        EVP_MD_CTX_free(ctx);
        return {};
    }

    std::vector<uint8_t> signature(sigLen);
    if (EVP_DigestSignFinal(ctx, signature.data(), &sigLen) != 1) {
        EVP_PKEY_free(pkey);
        EVP_MD_CTX_free(ctx);
        return {};
    }
    signature.resize(sigLen);
    success = true;

    EVP_PKEY_free(pkey);
    EVP_MD_CTX_free(ctx);
    return signature;
}

bool HsmEmulator::verify(const std::string& keyId,
                         const std::vector<uint8_t>& data,
                         const std::vector<uint8_t>& signature) const {
    auto it = publicKeys_.find(keyId);
    if (it == publicKeys_.end()) {
        return false;
    }

    BIO* bio = BIO_new_mem_buf(it->second.data(), it->second.size());
    EVP_PKEY* pkey = PEM_read_bio_PUBKEY(bio, nullptr, nullptr, nullptr);
    BIO_free(bio);
    if (!pkey) {
        return false;
    }

    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    if (!ctx) {
        EVP_PKEY_free(pkey);
        return false;
    }

    bool verified = false;
    if (EVP_DigestVerifyInit(ctx, nullptr, EVP_sha256(), nullptr, pkey) == 1 &&
        EVP_DigestVerifyUpdate(ctx, data.data(), data.size()) == 1 &&
        EVP_DigestVerifyFinal(ctx, signature.data(), signature.size()) == 1) {
        verified = true;
    }

    EVP_MD_CTX_free(ctx);
    EVP_PKEY_free(pkey);
    return verified;
}

bool HsmEmulator::generateSymmetricKey(const std::string& keyId, int size) {
    if (size <= 0) {
        return false;
    }
    if (symmetricKeys_.count(keyId) > 0) {
        return true;
    }
    std::vector<uint8_t> key(size);
    if (RAND_bytes(key.data(), size) != 1) {
        return false;
    }
    symmetricKeys_[keyId] = std::move(key);
    return true;
}

std::vector<uint8_t> HsmEmulator::computeMac(const std::string& keyId,
                                             const std::vector<uint8_t>& data,
                                             bool& success) const {
    success = false;
    auto it = symmetricKeys_.find(keyId);
    if (it == symmetricKeys_.end()) {
        return {};
    }
    const std::vector<uint8_t>& key = it->second;
    unsigned int len = EVP_MAX_MD_SIZE;
    std::vector<uint8_t> mac(len);
    unsigned char* result = HMAC(EVP_sha256(), key.data(), static_cast<int>(key.size()), data.data(), data.size(), mac.data(), &len);
    if (!result) {
        return {};
    }
    mac.resize(len);
    success = true;
    return mac;
}

bool HsmEmulator::verifyMac(const std::string& keyId,
                            const std::vector<uint8_t>& data,
                            const std::vector<uint8_t>& mac) const {
    bool success = false;
    std::vector<uint8_t> expected = computeMac(keyId, data, success);
    if (!success) {
        return false;
    }
    return expected == mac;
}

} // namespace ecu
