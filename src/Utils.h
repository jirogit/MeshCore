#pragma once

#include <MeshCore.h>
#include <Stream.h>
#include <string.h>

#define CTR_IV_SIZE  8   // 8B IV prepended to CTR ciphertext (2^64 unique IVs)

namespace mesh {

class RNG {
public:
  virtual void random(uint8_t* dest, size_t sz) = 0;

  /**
   * \returns  random number between _min (inclusive) and _max (exclusive)
   */
  uint32_t nextInt(uint32_t _min, uint32_t _max);
};

class Utils {
public:
  /**
   * \brief  calculates the SHA256 hash of 'msg', storing in 'hash' and truncating the hash to 'hash_len' bytes.
  */
  static void sha256(uint8_t *hash, size_t hash_len, const uint8_t* msg, int msg_len);

  /**
   * \brief  calculates the SHA256 hash of two fragments, 'frag1' and 'frag2' (in that order), storing in 'hash' and truncating.
  */
  static void sha256(uint8_t *hash, size_t hash_len, const uint8_t* frag1, int frag1_len, const uint8_t* frag2, int frag2_len);

  /**
   * \brief  Encrypts src using AES-128-CTR. Generates CTR_IV_SIZE random IV via rng, prepends it to dest,
   *         then encrypts src. No padding. Result stored in dest.
   * \returns  CTR_IV_SIZE + src_len
  */
  static int encrypt(const uint8_t* shared_secret, uint8_t* dest, const uint8_t* src, int src_len, RNG* rng);

  /**
   * \brief  encrypts bytes in src using AES-128-CTR (VER_2), then calculates MAC on ciphertext,
   *         inserting into leading bytes of 'dest'.
   * \returns  total length of bytes in 'dest' (MAC + IV + ciphertext)
  */
  static int encryptThenMAC(const uint8_t* shared_secret, uint8_t* dest, const uint8_t* src, int src_len, RNG* rng);

  /**
   * \brief  checks the MAC (in leading bytes of 'src'), then if valid, decrypts remaining bytes in src.
   * \returns  zero if MAC is invalid, otherwise the length of decrypted bytes in 'dest'
  */
  static int MACThenDecrypt(const uint8_t* shared_secret, uint8_t* dest, const uint8_t* src, int src_len);

  /**
   * \brief  converts 'src' bytes with given length to Hex representation, and null terminates.
  */
  static void toHex(char* dest, const uint8_t* src, size_t len);

  /**
   * \brief  converts 'src_hex' hexadecimal string (should be null term) back to raw bytes, storing in 'dest'.
   * \param dest_size   must be exactly the expected size in bytes.
   * \returns  true if successful
  */
  static bool fromHex(uint8_t* dest, int dest_size, const char *src_hex);

  /**
   * \brief  Prints the hexadecimal representation of 'src' bytes of given length, to Stream 's'.
  */
  static void printHex(Stream& s, const uint8_t* src, size_t len);

  /**
   * \brief  parse 'text' into parts separated by 'separator' char.
   * \param  text  the text to parse (note is MODIFIED!)
   * \param  parts  destination array to store pointers to starts of parse parts
   * \param  max_num  max elements to store in 'parts' array
   * \param  separator  the separator character
   * \returns  the number of parts parsed (in 'parts')
   */
  static int parseTextParts(char* text, const char* parts[], int max_num, char separator=',');

  static bool isHexChar(char c);
};

}
