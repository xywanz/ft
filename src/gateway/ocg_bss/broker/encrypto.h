// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

// #include <openssl/err.h>
// #include <openssl/pem.h>
// #include <openssl/rsa.h>

#include <cstring>
#include <stdexcept>
#include <string>

#ifndef BSS_BROKER_ENCRYPTO_H_
#define BSS_BROKER_ENCRYPTO_H_

namespace ft::bss {

class PasswordEncrptor {
 public:
  explicit PasswordEncrptor(const std::string& pub_key_file) noexcept(false) {
    // FILE* fp = fopen(pub_key_file.c_str(), "r");
    // if (!fp) throw std::runtime_error("cannot open rsa file");

    // rsa_ = PEM_read_RSA_PUBKEY(fp, nullptr, nullptr, nullptr);
    // fclose(fp);

    // if (!rsa_) throw std::runtime_error("cannot read rsa public key");
  }

  int encrypt(const void* plain_text, std::size_t plain_text_size,
              void* encrypted_text) {
    // uint8_t* dst = reinterpret_cast<uint8_t*>(encrypted_text);

    // int len = RSA_public_encrypt(plain_text_size,
    //                              reinterpret_cast<const
    //                              uint8_t*>(plain_text), dst, rsa_,
    //                              RSA_PKCS1_PADDING);
    // if (len >= 0) dst[len] = 0;
    int len = static_cast<int>(plain_text_size);
    if (plain_text_size != static_cast<std::size_t>(len)) return -1;

    memcpy(encrypted_text, plain_text, plain_text_size);
    reinterpret_cast<char*>(encrypted_text)[len] = 0;

    return len;
  }

 private:
  // RSA* rsa_ = nullptr;
};

}  // namespace ft::bss

#endif  // BSS_BROKER_ENCRYPTO_H_
