
// Beware, this code is shared between the kernel and userspace.

#ifdef _KERNEL
#include <types.h>
#include <lib.h>
#include <kern/errno.h>
#else
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>
#include <stdio.h>
#endif

#include <kern/secure.h>
#include "sha256.h"

// The full length (with null) of the hex string of a byte array
#define HEXLEN(a) 2*a+1

#define SHA256_BLOCK_SIZE 64
#define SHA256_OUTPUT_SIZE 32

// Keep this divisible by 4, or else change make_salt()
#define SALT_BYTES 8

// inner and outer padding for HMAC.
static const unsigned char ipad[SHA256_BLOCK_SIZE] = { [0 ... SHA256_BLOCK_SIZE-1] = 0x36 };
static const unsigned char opad[SHA256_BLOCK_SIZE] = { [0 ... SHA256_BLOCK_SIZE-1] = 0x5c };

// Hack for not having a userspace malloc until ASST3. We 'allocate' these statuc buffers.
// This works because the process single-threaded.
#define NUM_BUFFERS 4
#define BUFFER_LEN 1024

static char temp_buffers[NUM_BUFFERS][BUFFER_LEN];
static int buf_num = 0;

#ifndef _KERNEL
static int did_random = 0;
#define NSEC_PER_MSEC 1000000ULL
#define MSEC_PER_SEC 1000ULL
#endif

// Both userspace and the kernel are using the temp buffers now.
static void * _alloc(size_t size)
{
	(void)size;
	void *ptr = temp_buffers[buf_num];
	buf_num++;
	buf_num = buf_num % NUM_BUFFERS;
	return ptr;
}

static void _free(void *ptr)
{
	(void)ptr;
}

/*
 * hamc_sha256 follows FIPS 198-1 HMAC using sha256.
 * See http://nvlpubs.nist.gov/nistpubs/FIPS/NIST.FIPS.180-4.pdf for details.
 * NOTE: This is only thread-safe if called from within secprintf()!!!
 */
static int hmac_sha256(const char *msg, size_t msg_len, const char *key, size_t key_len, 
		unsigned char output[SHA256_OUTPUT_SIZE])
{
	// We use a total of 320 bytes of array data on the stack
	unsigned char k0[SHA256_BLOCK_SIZE];

	// Steps 1-3.  Anything less than 64 bytes gets 0s appended.
	memset(k0, 0, SHA256_BLOCK_SIZE);
	
	if (key_len <= SHA256_BLOCK_SIZE) {
		memcpy(k0, key, key_len);
	} else {
		mbedtls_sha256((unsigned char *)key, key_len, k0, 0); 
	}

	// Steps 4 and 7.
	unsigned char k_ipad[SHA256_BLOCK_SIZE];
	unsigned char k_opad[SHA256_BLOCK_SIZE];

	int i;
	for (i = 0; i < SHA256_BLOCK_SIZE; i++) {
		k_ipad[i] = k0[i] ^ ipad[i];
		k_opad[i] = k0[i] ^ opad[i];
	}

	// Step 5 (K0 xor ipad) || msg
	// We have no idea how big the message is so we allocate this on the heap.
	unsigned char *data = (unsigned char *)_alloc(msg_len + SHA256_BLOCK_SIZE);
	if (!data)
		return ENOMEM;

	memcpy(data, k_ipad, SHA256_BLOCK_SIZE);
	memcpy(data+SHA256_BLOCK_SIZE, msg, msg_len);

	// Step6: H((k0 xor ipad) || msg)
	unsigned char h1[SHA256_OUTPUT_SIZE];
	mbedtls_sha256(data, msg_len + SHA256_BLOCK_SIZE, h1, 0);
	_free(data);

	// Step 8: (k0 xor opad) || H((k0 xor ipad) || msg)
	unsigned char inner[SHA256_OUTPUT_SIZE + SHA256_BLOCK_SIZE];
	memcpy(inner, k_opad, SHA256_BLOCK_SIZE);
	memcpy(inner + SHA256_BLOCK_SIZE, h1, SHA256_OUTPUT_SIZE);

	// Step 9: Finally, H((k0 xor opad) || H((k0 xor ipad) || msg))
	mbedtls_sha256(inner, SHA256_OUTPUT_SIZE + SHA256_BLOCK_SIZE, output, 0);

	return 0;
}

static inline char to_hex(int n)
{
	return n < 10 ? '0'+n : 'a' + (n-10);
}

static void array_to_hex(unsigned char *a, size_t len, char *res)
{
	size_t i, j;
	j = 0;
	for (i = 0; i < len; i++) {
		res[j++] = to_hex(a[i] >> 4);
		res[j++] = to_hex(a[i] & 0xF);
	}
	res[j] = '\0';
}

static void make_salt(char *salt_str)
{
#ifndef _KERNEL
	if (!did_random) {
		did_random = 1;
		time_t sec;
		unsigned long ns;
		unsigned long long ms;

		__time(&sec, &ns);
		ms = (unsigned long long)sec * MSEC_PER_SEC;
		ms += (ns / NSEC_PER_MSEC);
		srandom((unsigned long)ms);
	}
#endif

	// Compute salt value
	uint32_t salt[SALT_BYTES/sizeof(uint32_t)];

	size_t i;
	for (i = 0; i < SALT_BYTES/sizeof(uint32_t); i++)
	{
		salt[i] = random();
	}

	// Convert to hex string
	array_to_hex((unsigned char *)salt, SALT_BYTES, salt_str);
}

int hmac(const char *msg, size_t msg_len, const char *key, size_t key_len, 
		char **hash_str)
{
	*hash_str = _alloc(HEXLEN(SHA256_OUTPUT_SIZE));

	if (!(*hash_str))
		return ENOMEM;

	// Hash it
	unsigned char hash[SHA256_OUTPUT_SIZE];
	int res = hmac_sha256(msg, msg_len, key, key_len, hash);
	if (res)
		return res;

	// Convert to hex string
	array_to_hex(hash, SHA256_OUTPUT_SIZE, *hash_str);

	return 0;
}

// NOTE: This is only thread-safe if called from within secprintf()!!!
int hmac_salted(const char *msg, size_t msg_len, const char *key, size_t key_len, 
		char **hash_str, char **salt_str)
{
	*salt_str = _alloc(HEXLEN(SALT_BYTES));

	if (!(*salt_str))
		return ENOMEM;

	// Create the salt value
	make_salt(*salt_str);

	// Concatenate the key and salt, with the resulting string being null-terminated
	size_t key2_len = key_len + HEXLEN(SALT_BYTES)-1;
	char *key2 = (char *)_alloc(key2_len+1);
	if (!key2)
		return ENOMEM;

	strcpy(key2, key);
	strcpy(key2+key_len, *salt_str);
	key2[key2_len] = '\0';

	// Hash it
	return hmac(msg, msg_len, key2, key2_len, hash_str);
}
