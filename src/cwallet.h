#ifndef CWALLET_H
#define CWALLET_H
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

/* Fill out the LaTeX code for the human-readable part of the passphrase or small private key page of the paper wallet.
   target: the result that is to contain the LaTeX code
   source: the string of characters to print in human-readable format
   maxcharforline: the maximum number of characters per line that will be shown on the PDF file
   startncharforline: the number of characters already added to last human-readable line of the PDF file */
int fill_latex_lines (char * const target, const char * source, const int maxncharforline, const int startncharforline);

/* Create a PDF file with the private key QR encoded. If pvalue is nonzero and eflag is 1, then add a second page with the passphrase QR encoded.
   bc_address: the Bitcoin address in standard Bitcoin base 58 format
   bc_privkey: the Bitcoin private key in the standard Bitcoin base 58 format
   dvalue: the output directory
   ovalue: the output file name
   pvalue: the passphrase
   eflag: 1 if the private key is encrypted
   sflag: 1 for the small/minimal format
   Return 0 if successful */
int qrencode (const char * const bc_address, const char * const bc_privkey, const char * const dvalue, const char * const ovalue, const char * const pvalue, const unsigned char eflag, const unsigned char sflag);

/* Get the first occurence of c in the string str.
   c: the character to search for
   str: the string
   Return -1 if c is not found in str. Otherwise, return the index of the first occurence of c in str. */
int indexof (const char c, const char * str);

/* Generate a Bitcoin private key and save the output to result. If successful, the first string in result is the address in base 58 format, and the second string is the private key in base 58 format. If not successful, the first string in result is an error message.
   qflag: 1 to QR encode the result
   rflag: 1 to generate a random key (with /dev/random)
   eflag: 1 to encrypt the private key via a one-time-pad determined from the passphrase
   uflag: 1 to use uncompressed public keys
   sflag: 1 to create a small/minimal wallet
   dvalue: the output directory
   ovalue: the output file name
   tvalue: the type of address, i.e. the address prefix code (0 for Bitcoin, 52 for Namecoin, ...)
   pvalue: the passphrase
   kvalue: the private key in the standard Bitcoin base 58 format
   result: The result the will contain the address and private key, or the error message.
   Return 0 if successful. */
int generate_key (char ** const result, const unsigned char qflag, const unsigned char rflag, const unsigned char eflag, const unsigned char uflag, const unsigned char sflag, const char * const dvalue, const char * const ovalue, const char * const tvalue, const char * const pvalue, const char * const kvalue);

/* Get the public key corresponding to priv, and put the public key in result.
   result: The pointer to the unsigned char array that is to contain the public key
   m: The length of the public key (number of bytes) (33 for compressed, 65 for uncompressed)
   priv: The secret part of the private key in base 256 format
   n: The length of priv (number of bytes) (32 is the standard)
   Return 0 if successful. */
int priv_to_pub (unsigned char * const result, const size_t m, const unsigned char * const priv, const size_t n);

/* Convert the base 58 char array b58 to a base 256 unsigned char array, and store the result in result.
   result: The result that is to contain the base 256 unsigned char array
   m: The length of the result
   b58: The base 58 char array
   n: The length of the base 58 char array
   check_reverse: 1 to check if uchar_to_b58 gives back b58
   Return the offset of the result, i.e. the pointer to the actual result is result+offset. A null byte will be appended to the end of the result. */
int b58_to_uchar (unsigned char * const result, const size_t m, const char * b58, const size_t n, const unsigned char check_reverse);

/* Convert the base 256 unsigned char array uchar to a base 58 char array, and put the result in result.
   result: The result that is to contain the base 58 char array
   m: The length of the result
   uchar: The unsigned char array
   n: The length of the unsigned char array
   check_reverse: 1 to check if b58_to_uchar gives back uchar
   Return the offset of the result, i.e. the pointer to the actual result is result+offset. A null byte will be appended to the end of the result. */
int uchar_to_b58 (char * const result, const size_t m, const unsigned char * uchar, const size_t n, const unsigned char check_reverse);

/* Convert the base 64 char array b64 to a base 256 unsigned char array, and store the result in result.
   result: The result that is to contain the base 256 unsigned char array
   m: The length of the result
   b64: The base 64 char array 
   n: The length of the base 64 char array
   check_reverse: 1 to check if uchar_to_b64 gives back b64
   Return the offset of the result, i.e. the pointer to the actual result is result+offset. A null byte will be appended to the end of the result. The entries before the offset are filled with NULL chars, as the unoffsetted result is used for modular addition with passphrases. */
int b64_to_uchar (unsigned char * const result, const size_t m, const char * b64, const size_t n, const unsigned char check_reverse);

/* Convert the base 256 unsigned char array uchar to a base 64 char array, and put the result in result.
   result: The result that is to contain the base 256 unsigned char array
   m: The length of the result
   uchar: The unsigned char array
   n: The length of the unsigned char array
   check_reverse: 1 to check if b64_to_uchar gives back uchar
   Return the offset of the result, i.e. the pointer to the actual result is result+offset. A null byte will be appended to the end of the result. */
int uchar_to_b64 (char * const result, const size_t m, const unsigned char * uchar, const size_t n, const unsigned char check_reverse);

/* Add the base 256 unsigned char array c2 to the base 256 unsigned char array c1 by performing byte by byte addition modulo 256.
   result: The result that is to contain the result of the addition
   c1: The unsigned char array to be added to
   c2: The unsigned char array to be added
   n: The length of each array
   Return 0 if successful */
int add_uchars (unsigned char * const result, const unsigned char * c1, const unsigned char * c2, const size_t n);

/* Subtract the base 256 unsigned char array c2 from the base 256 unsigned char array c1 by performing byte by byte subtraction modulo 256.
   result: The result that is to contain the result of the subtraction
   c1: The unsigned char array to be subtracted from
   c2: The unsigned char array to be subtracted
   n: The length of each array
   Return 0 if successful */
int subtract_uchars (unsigned char * const result, const unsigned char * c1, const unsigned char * c2, const size_t n);

/* Get the private key in the standard Bitcoin base 58 format from the secret part of the private key in base 256 format, and put the result in result.
   result: The result that is to contain the private key in the standard Bitcoin base 58 format
   key: The secret part of the private key in base 256 format
   n: The length of key
   pubkey: The public key in base 256 format. Set this to NULL if you don't want to check whether key corresponds to pubkey.
   m: The length of the public key (33 for compressed, 65 for uncompressed)
   Return the offset of the result, i.e. the pointer to the actual result is result+offset */
int privkey_to_bc_format (char * const result, const unsigned char * key, const size_t n, const unsigned char * pubkey, const size_t m);

/* Get the Bitcoin address in the standard Bitcoin base 58 format from the public key in base 256 format, and put the result in result.
   result: The result that is to contain the address in the standard Bitcoin base 58 format
   key: The secret part of the public key in base 256 format
   n: The length of key
   type: the type of address, i.e. the address prefix code (0 for Bitcoin, 52 for Namecoin, ...)
   Return the offset of the result, i.e. the pointer to the actual result is result+offset */
int pubkey_to_bc_format (char * const result, const unsigned char * key, const size_t n, const unsigned char type);
#endif
