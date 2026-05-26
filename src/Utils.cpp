#include "Utils.h"
#include "Packet.h"
#include <AES.h>
#include <CTR.h>
#include <SHA256.h>

#ifdef ARDUINO
  #include <Arduino.h>
#endif

namespace mesh {

uint32_t RNG::nextInt(uint32_t _min, uint32_t _max) {
  uint32_t num;
  random((uint8_t *) &num, sizeof(num));
  return (num % (_max - _min)) + _min;
}

void Utils::sha256(uint8_t *hash, size_t hash_len, const uint8_t* msg, int msg_len) {
  SHA256 sha;
  sha.update(msg, msg_len);
  sha.finalize(hash, hash_len);
}

void Utils::sha256(uint8_t *hash, size_t hash_len, const uint8_t* frag1, int frag1_len, const uint8_t* frag2, int frag2_len) {
  SHA256 sha;
  sha.update(frag1, frag1_len);
  sha.update(frag2, frag2_len);
  sha.finalize(hash, hash_len);
}


static int decryptCTR(const uint8_t* shared_secret, uint8_t* dest, const uint8_t* src, int src_len) {
  // CTR decrypt (VER_2): src = [IV 8B][ciphertext src_len-8 B]
  if (src_len <= CTR_IV_SIZE) return 0;

  uint8_t iv[16];
  memset(iv, 0, 16);
  memcpy(iv, src, CTR_IV_SIZE);   // first 8B from packet; upper 8B stay zero

  // static to avoid ~230B stack allocation on every decrypt (safe: single-threaded Arduino loop)
  static CTR<AES128> ctr;
  ctr.setKey(shared_secret, CIPHER_KEY_SIZE);
  ctr.setIV(iv, 16);
  ctr.decrypt(dest, src + CTR_IV_SIZE, src_len - CTR_IV_SIZE);

  return src_len - CTR_IV_SIZE;
}

int Utils::encrypt(const uint8_t* shared_secret, uint8_t* dest, const uint8_t* src, int src_len, RNG* rng) {
  // CTR encrypt (VER_2): dest = [IV CTR_IV_SIZE B][ciphertext src_len B]
  if (!rng) return 0;  // null guard: should never happen, but prevents hard fault

  rng->random(dest, CTR_IV_SIZE);

  uint8_t iv[16];
  memset(iv, 0, 16);
  memcpy(iv, dest, CTR_IV_SIZE);

  // static to avoid ~230B stack allocation on every encrypt (safe: single-threaded Arduino loop)
  static CTR<AES128> ctr;
  ctr.setKey(shared_secret, CIPHER_KEY_SIZE);
  ctr.setIV(iv, 16);
  ctr.encrypt(dest + CTR_IV_SIZE, src, src_len);

  return CTR_IV_SIZE + src_len;
}

int Utils::encryptThenMAC(const uint8_t* shared_secret, uint8_t* dest, const uint8_t* src, int src_len, RNG* rng) {
  int enc_len = encrypt(shared_secret, dest + CIPHER_MAC_SIZE, src, src_len, rng);

  SHA256 sha;
  sha.resetHMAC(shared_secret, PUB_KEY_SIZE);
  sha.update(dest + CIPHER_MAC_SIZE, enc_len);
  sha.finalizeHMAC(shared_secret, PUB_KEY_SIZE, dest, CIPHER_MAC_SIZE);

  return CIPHER_MAC_SIZE + enc_len;
}

int Utils::MACThenDecrypt(const uint8_t* shared_secret, uint8_t* dest, const uint8_t* src, int src_len) {
  if (src_len <= CIPHER_MAC_SIZE + CTR_IV_SIZE) return 0;  // invalid src bytes

  uint8_t hmac[CIPHER_MAC_SIZE];
  {
    SHA256 sha;
    sha.resetHMAC(shared_secret, PUB_KEY_SIZE);
    sha.update(src + CIPHER_MAC_SIZE, src_len - CIPHER_MAC_SIZE);
    sha.finalizeHMAC(shared_secret, PUB_KEY_SIZE, hmac, CIPHER_MAC_SIZE);
  }
  if (memcmp(hmac, src, CIPHER_MAC_SIZE) == 0) {
    return decryptCTR(shared_secret, dest, src + CIPHER_MAC_SIZE, src_len - CIPHER_MAC_SIZE);
  }
  return 0; // invalid HMAC
}

static const char hex_chars[] = "0123456789ABCDEF";

void Utils::toHex(char* dest, const uint8_t* src, size_t len) {
  while (len > 0) {
    uint8_t b = *src++;
    *dest++ = hex_chars[b >> 4];
    *dest++ = hex_chars[b & 0x0F];
    len--;
  }
  *dest = 0;
}

void Utils::printHex(Stream& s, const uint8_t* src, size_t len) {
  while (len > 0) {
    uint8_t b = *src++;
    s.print(hex_chars[b >> 4]);
    s.print(hex_chars[b & 0x0F]);
    len--;
  }
}

static uint8_t hexVal(char c) {
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  if (c >= '0' && c <= '9') return c - '0';
  return 0;
}

bool Utils::isHexChar(char c) {
  return c == '0' || hexVal(c) > 0;
}

bool Utils::fromHex(uint8_t* dest, int dest_size, const char *src_hex) {
  int len = strlen(src_hex);
  if (len != dest_size*2) return false;  // incorrect length

  uint8_t* dp = dest;
  while (dp - dest < dest_size) {
    char ch = *src_hex++;
    char cl = *src_hex++;
    *dp++ = (hexVal(ch) << 4) | hexVal(cl);
  }
  return true;
}

int Utils::parseTextParts(char* text, const char* parts[], int max_num, char separator) {
  int num = 0;
  char* sp = text;
  while (*sp && num < max_num) {
    parts[num++] = sp;
    while (*sp && *sp != separator) sp++;
    if (*sp) {
       *sp++ = 0;  // replace the seperator with a null, and skip past it
    }
  }
  // if we hit the maximum parts, make sure LAST entry does NOT have separator 
  while (*sp && *sp != separator) sp++;
  if (*sp) {
    *sp = 0;  // replace the separator with null
  }
  return num;
}

}