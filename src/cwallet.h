#ifndef CWALLET_H
#define CWALLET_H
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <db.h>
#include <openssl/bn.h>
#include <openssl/sha.h>
#include <openssl/ripemd.h>
#include <openssl/ec.h>
#include <openssl/obj_mac.h>

extern int publen; /* The length of the public key (33 for compressed, 65 for uncompressed) */
extern const int privlen; /* The length of the secret part of the private key (32 is the standard) */
extern const char * b58chars; /* The alphabet for the base 58 characters */
extern const char * b64chars; /* The alphabet for the base 64 characters */

/* Create a PDF file with the private key QR encoded. If pvalue is nonzero and eflag is 1, then add a second page with the passphrase QR encoded.
   bcaddress: the Bitcoin address in standard Bitcoin base 58 format
   bcprivkey: the Bitcoin private key in the standard Bitcoin base 58 format
   dvalue: the output directory
   ovalue: the output file name
   pvalue: the passphrase
   eflag: the encryption flag
   Return 0 if successful */
int qrencode (char * bcaddress, char * bcprivkey, char * dvalue, char * ovalue, char * pvalue, unsigned char eflag);

/* Get the first occurence of c in the string str.
   c: the character to search for
   str: the string
   Return -1 if c is not found in str. */
int generate_key(unsigned char qflag, unsigned char rflag, unsigned char eflag, unsigned char uflag, char * dvalue, char * ovalue, char * tvalue, char * pvalue, char * kvalue, char ** result);

/* Generate a Bitcoin private key and save the output to result. If successful, the first string in result is the address in base 58 format, and the second string is the private key in base 58 format. If not successful, the first string in result is an error message.
   qflag: 1 to QR encode the result
   rflag: 1 to generate a random key (with /dev/random)
   eflag: 1 to encrypt the private key via a one-time-pad determined from the passphrase.
   uflag: 1 to use uncompressed public keys
   dvalue: the output directory
   ovalue: the output file name
   tvalue: the type of address, i.e. the address prefix code (0 for Bitcoin, 52 for Namecoin, ...)
   pvalue: the passphrase
   kvalue: the private key in the standard Bitcoin base 58 format
   result: The result the will contain the address and private key, or the error message.
   Return 0 if successful. */
int indexof(const char c, const char * str);

/* Get the public key corresponding to priv, and put the public key in result.
   priv: The secret part of the private key in base 256 format
   n: The length of priv (number of bytes) (32 is the standard)
   m: The length of the public key (number of bytes) (33 for compressed, 65 for uncompressed)
   result: The pointer to the unsigned char array that is to contain the public key
   Return 0 if successful. */
int priv_to_pub(const unsigned char * priv, size_t n, size_t m, unsigned char ** result);

/* Convert the base 58 char array b58 to a base 256 unsigned char array, and store the result in result.
   result: The result that is to contain the base 256 unsigned char array
   nr: The length of the result
   b58: The base 58 char array
   n: The length of the base 58 char array
   Return the offset of the result, i.e. the pointer to the actual result is result+offset */
int b58_to_uchar(unsigned char * result, size_t nr, char * b58, size_t n);

/* Convert the base 256 unsigned char array uchar to a base 58 char array, and put the result in result.
   uchar: The unsigned char array
   n: The length of the unsigned char array
   nr: The length of the result
   result: The result that is to contain the base 58 char array
   Return the offset of the result, i.e. the pointer to the actual result is result+offset */
int uchar_to_b58(unsigned char * uchar, size_t n, size_t nr, char * result);

/* Convert the base 64 char array b64 to a base 256 unsigned char array, and store the result in result.
   result: The result that is to contain the base 256 unsigned char array
   nr: The length of the result
   b64: The base 64 char array 
   n: The length of the base 64 char array
   Return the offset of the result, i.e. the pointer to the actual result is result+offset */
int b64_to_uchar(char * b64, size_t n, unsigned char * result, size_t nr);

/* Add the base 256 unsigned char array c2 to the base 256 unsigned char array c1 by performing byte by byte addition modulo 256.
   c1: The unsigned char array to be added to
   c2: The unsigned char array to be added
   n: The length of each array
   result: The result that is to contain the result of the addition
   Return 0 if successful */
int add_uchars(unsigned char * c1, unsigned char * c2, size_t n, unsigned char * result);

/* Subtract the base 256 unsigned char array c2 from the base 256 unsigned char array c1 by performing byte by byte subtraction modulo 256.
   c1: The unsigned char array to be subtracted from
   c2: The unsigned char array to be subtracted
   n: The length of each array
   result: The result that is to contain the result of the subtraction
   Return 0 if successful */
int subtract_uchars(unsigned char * c1, unsigned char * c2, size_t n, unsigned char * result);

/* Get the private key in the standard Bitcoin base 58 format from the secret part of the private key in base 256 format, and put the result in result.
   key: The secret part of the private key in base 256 format
   n: The length of key
   pubkey: The public key in base 256 format
   m: The length of the public key (33 for compressed, 65 for uncompressed)
   result: The result that is to contain the private key in the standard Bitcoin base 58 format
   check: 1 to check whether key correctly corresponds to pubkey
   Return the offset of the result, i.e. the pointer to the actual result is result+offset */
int privkey_to_bc_format(const unsigned char * key, size_t n, unsigned char * pubkey, size_t m, char * result, unsigned char check);

/* Get the Bitcoin address in the standard Bitcoin base 58 format from the public key in base 256 format, and put the result in result.
   key: The secret part of the public key in base 256 format
   n: The length of key
   pubkey: The public key in base 256 format
   m: The length of the public key (33 for compressed, 65 for uncompressed)
   result: The result that is to contain the address in the standard Bitcoin base 58 format
   type: the type of address, i.e. the address prefix code (0 for Bitcoin, 52 for Namecoin, ...)
   Return the offset of the result, i.e. the pointer to the actual result is result+offset */
int pubkey_to_bc_format(const unsigned char * key, size_t n, char * result, unsigned char type);
#endif
