#ifndef _SECURE_H
#define _SECURE_H

#define SHA256_BLOCK_SIZE 64
#define SHA256_OUTPUT_SIZE 32

#define TOHEX(n) n < 10 ? '0'+n : 'a' + (n-10)

// Compute the hex string from SHA256 hash output
void hex_from_hash(unsigned char hash[SHA256_OUTPUT_SIZE], char res[SHA256_OUTPUT_SIZE*2 + 1]);

// Compute the FIPS 198-1 complient HMAC of msg using SHA256
void hmac_sha256(const char *msg, size_t msg_len, char *key, size_t key_len, 
		unsigned char output[SHA256_OUTPUT_SIZE]);

#endif //_SECURE_H
