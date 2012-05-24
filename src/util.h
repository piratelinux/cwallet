#include <string.h>
#include <db.h>
#include <openssl/bn.h>
#include <openssl/sha.h>
#include <openssl/ripemd.h>
#include <openssl/ec.h>
#include <openssl/obj_mac.h>

const char * b58chars = "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";

int indexof(char c, const char * str) {
  int i=0;
  while(*str != '\0') {
    if (*str == c) {
      return(i);
    }
    i++;
    str++;
  }
  return(-1);
}

int priv_to_pub(const unsigned char * priv, size_t n, size_t m, unsigned char ** result) {

  int ret = 0;
  BIGNUM * retbn = 0;
  EC_KEY * retkey = 0;
  int i = 0;

  BIGNUM * privbn = 0;
  BN_dec2bn(&privbn,"0");
  BIGNUM * pow256 = 0;
  BN_dec2bn(&pow256,"1");
  BIGNUM * one256 = 0;
  BN_dec2bn(&one256,"256");

  const unsigned char * addri = priv + n - 1;
  for (i=0; i<n; i++) {
    BIGNUM * addribn = 0;
    char * addristr = malloc(4);
    sprintf(addristr,"%d",*addri);
    BN_dec2bn(&addribn,addristr);
    BN_CTX * ctx = BN_CTX_new();
    BN_mul(addribn, pow256, addribn, ctx);
    BN_add(privbn,privbn,addribn);
    BN_mul(pow256, pow256, one256, ctx);
    BN_free(addribn);
    if (addristr != 0) {
      free(addristr);
    }
    BN_CTX_free(ctx);
    addri--;
  }

  EC_GROUP * group = EC_GROUP_new_by_curve_name(NID_secp256k1);
  EC_POINT * pub_key = EC_POINT_new(group);
  BN_CTX * ctx = BN_CTX_new();
  EC_POINT_mul(group, pub_key, privbn, 0, 0, ctx);
  if (m == 65) {
    EC_POINT_point2oct(group,pub_key,POINT_CONVERSION_UNCOMPRESSED,*result,m,ctx);
  }
  else {
    EC_POINT_point2oct(group,pub_key,POINT_CONVERSION_COMPRESSED,*result,m,ctx);
  }
  EC_POINT_free(pub_key);
  BN_CTX_free(ctx);

  return(0);

}

int b58_to_uchar(unsigned char * result, size_t nr, char * b58, size_t n) {

  BIGNUM * lval = 0;
  BN_dec2bn(&lval,"0");
  BIGNUM * pow58 = 0;
  BN_dec2bn(&pow58,"1");
  BIGNUM * one58 = 0;
  BN_dec2bn(&one58,"58");

  int i = 0;
  const unsigned char * addri = b58 + n - 1;
  for (i=0; i<n; i++) {
    BIGNUM * addribn = 0;
    char * addristr = malloc(4);
    sprintf(addristr,"%d",indexof(*addri,b58chars));
    BN_dec2bn(&addribn,addristr);
    BN_CTX * ctx = BN_CTX_new();
    BN_mul(addribn, pow58, addribn, ctx);
    BN_add(lval,lval,addribn);
    BN_mul(pow58, pow58, one58, ctx);
    BN_free(addribn);
    if (addristr != 0) {
      free(addristr);
    }
    BN_CTX_free(ctx);
    addri--;
  }

  unsigned char * addrb256i = result + nr;
  *addrb256i = '\0';
  addrb256i--;

  BIGNUM * one256 = 0;
  BN_dec2bn(&one256,"256");

  int offset = nr;
  while (BN_cmp(lval,one256) >= 0) {
    BN_CTX * ctx = BN_CTX_new();
    BIGNUM * lvalmod256 = BN_new();
    BN_div(lval, lvalmod256, lval, one256, ctx);
    int lvalmod256int = atoi(BN_bn2dec(lvalmod256));
    *addrb256i = lvalmod256int;
    addrb256i--;
    BN_free(lvalmod256);
    BN_CTX_free(ctx);
    offset--;
  }
  int lvalint = atoi(BN_bn2dec(lval));
  *addrb256i = lvalint;
  offset--;

  addri = b58;
  for (i=0; i<n; i++) {
    if (*addri == '1') {
      addrb256i--;
      *addrb256i = 0;
      offset--;
      addri++;
    }
    else {
      break;
    }
  }

  return(offset);
}

int uchar_to_b58(unsigned char * uchar, size_t n, size_t nr, char * result) {

  BIGNUM * lval = 0;
  BN_dec2bn(&lval,"0");
  BIGNUM * pow256 = 0;
  BN_dec2bn(&pow256,"1");
  BIGNUM * one256 = 0;
  BN_dec2bn(&one256,"256");

  int i = 0;
  const unsigned char * addri = uchar + n - 1;
  for (i=0; i<n; i++) {
    BIGNUM * addribn = 0;
    char * addristr = malloc(4);
    sprintf(addristr,"%d",*addri);
    BN_dec2bn(&addribn,addristr);
    BN_CTX * ctx = BN_CTX_new();
    BN_mul(addribn, pow256, addribn, ctx);
    BN_add(lval,lval,addribn);
    BN_mul(pow256, pow256, one256, ctx);
    BN_free(addribn);
    if (addristr != 0) {
      free(addristr);
    }
    BN_CTX_free(ctx);
    addri--;
  }

  char * addrb58i = result + nr;
  *addrb58i = '\0';
  addrb58i--;

  BIGNUM * one58 = 0;
  BN_dec2bn(&one58,"58");

  int offset = nr;
  while (BN_cmp(lval,one58) >= 0) {
    BN_CTX * ctx = BN_CTX_new();
    BIGNUM * lvalmod58 = BN_new();
    BN_div(lval, lvalmod58, lval, one58, ctx);
    int lvalmod58int = atoi(BN_bn2dec(lvalmod58));
    *addrb58i = b58chars[lvalmod58int];
    addrb58i--;
    BN_free(lvalmod58);
    BN_CTX_free(ctx);
    offset--;
  }
  int lvalint = atoi(BN_bn2dec(lval));
  *addrb58i = b58chars[lvalint];
  offset--;

  addri = uchar;
  for (i=0; i<n; i++) {
    if (*addri == '\0') {
      addrb58i--;
      *addrb58i = '1';
      offset--;
      addri++;
    }
    else {
      break;
    }
  }

  //check reverse
  unsigned char * result_uchar = (unsigned char *)malloc(n);
  int ret = b58_to_uchar(result_uchar,n,result,nr);
  addri = uchar;
  const unsigned char * addri_chk = result_uchar+ret;
  for (i=0; i<n-ret; i++) {
    if (*addri_chk != *addri) {
      printf("(ERROR)");
      return(-1);
    }
    addri++;
    addri_chk++;
  }

  free(uchar);

  return(offset);
}

int privkey_to_bc_format(const unsigned char * key, size_t n, unsigned char * pubkey, size_t m, char * result) {

  int ret = 0;
  int i = 0;

  unsigned char * pubcheck = malloc(m);
  ret = priv_to_pub(key,n,m,&pubcheck);

  unsigned char * pubkeyi = pubkey;
  unsigned char * pubchecki = pubcheck;
  for (i=0; i<m; i++) {
    if (*pubkeyi != * pubchecki) {
      printf("(INVALID KEY)");
      return(-1);
    }
    pubkeyi++;
    pubchecki++;
  }

  free(pubcheck);

  int n_ext = n+1;
  if (m != 65) {
    n_ext = n+2;
  }

  unsigned char * keyext = malloc(n_ext);
  *keyext = 128;
  memcpy(keyext+1,key,n);
  if (m != 65) {
    *(keyext+n+1) = 1;
  }
  unsigned char * hash1 = malloc(32);
  SHA256(keyext,n_ext,hash1);
  unsigned char * hash2 = malloc(32);
  SHA256(hash1,32,hash2);
  unsigned char * addr = malloc(n_ext+4);
  memcpy(addr,keyext,n_ext);
  memcpy(addr+n_ext,hash2,4);
  free(keyext);
  free(hash1);
  free(hash2);
  return(uchar_to_b58(addr,n_ext+4,52,result));

}

int pubkey_to_bc_format(const unsigned char * key, size_t n, char * result, int type) {

  unsigned char * hash1 = malloc(32);
  SHA256(key,n,hash1);
  unsigned char * hash2 = malloc(20);
  RIPEMD160(hash1,32,hash2);
  unsigned char * vh160 = malloc(21);
  *vh160 = type;
  memcpy(vh160+1,hash2,20);
  unsigned char * hash3 = malloc(32);
  SHA256(vh160,21,hash3);
  unsigned char * hash4 = malloc(32);
  SHA256(hash3,32,hash4);
  unsigned char * addr = malloc(25);
  memcpy(addr,vh160,21);
  memcpy(addr+21,hash4,4);
  free(hash1);
  free(hash2);
  free(vh160);
  free(hash3);
  free(hash4);
  return(uchar_to_b58(addr,25,34,result));

}
