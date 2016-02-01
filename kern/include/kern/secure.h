#ifndef _KERN_SECURE_H_
#define _KERN_SECURE_H_

/*
 * Compute the FIPS 198-1 complient HMAC of msg using SHA256.
 *
 * hmac_with_salt uses a salted key and sets salt_str to this value (in hex). 
 * Both functions below create hash_str with the hex readable version of the hash.
 *
 * Callers need to free hash_str (and salt_str) when done.
 */
int hmac(const char *msg, size_t msg_len, const char *key, size_t key_len, 
		char **hash_str);

int hmac_salted(const char *msg, size_t msg_len, const char *key, size_t key_len, 
		char **hash_str, char **salt_str);

#endif //_KERN_SECURE_H_
