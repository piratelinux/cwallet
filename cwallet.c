#include <string.h>
#include <db.h>
#include <openssl/bn.h>
#include <openssl/sha.h>
#include <openssl/ripemd.h>
#include <openssl/ec.h>
#include <openssl/obj_mac.h>

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

  EC_POINT_point2oct(group,pub_key,POINT_CONVERSION_UNCOMPRESSED,*result,m,ctx);
  
  EC_POINT_free(pub_key);

  BN_CTX_free(ctx);

  return(0);
}

int uchar_to_b58(const unsigned char * uchar, size_t n, size_t nr, char * result) {

  const char * b58chars = "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";

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
      printf("INVALID-KEY");
      return(-1);
    }
    pubkeyi++;
    pubchecki++;
  }

  free(pubcheck);

  unsigned char * keyext = malloc(n+1);
  *keyext = 128;
  memcpy(keyext+1,key,n);
  unsigned char * hash1 = malloc(32);
  SHA256(keyext,n+1,hash1);
  unsigned char * hash2 = malloc(32);
  SHA256(hash1,32,hash2);
  unsigned char * addr = malloc(n+5);
  memcpy(addr,keyext,n+1);
  memcpy(addr+n+1,hash2,4);
  free(keyext);
  free(hash1);
  free(hash2);
  return uchar_to_b58(addr,n+5,51,result);
  free(addr);

}

int pubkey_to_bc_format(const unsigned char * key, size_t n, char * result) {

  unsigned char * hash1 = malloc(32);
  SHA256(key,n,hash1);
  unsigned char * hash2 = malloc(20);
  RIPEMD160(hash1,32,hash2);
  unsigned char * vh160 = malloc(21);
  *vh160 = 0;
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
  return uchar_to_b58(addr,25,34,result);
  free(addr);
}

int main(int argc, char ** argv) {

  char * walletfile = 0;

  if (argc > 1) {
    walletfile = strdup(argv[1]);
  }
  else {
    char * homedir = getenv("HOME");
    walletfile = malloc(strlen(homedir)+21);
    strcpy(walletfile,homedir);
    strcat(walletfile,"/.bitcoin/wallet.dat");
  }

  DB * dbp;
  u_int32_t flags;
  int ret;
  
  ret = db_create(&dbp,0,0);

  if (ret != 0) {
    printf("db_create failed\n");
  }

  flags = DB_RDONLY;

  ret = dbp->open(dbp,0,walletfile,"main",DB_BTREE,flags,0);

  if (ret != 0) {
    printf("dbp->open failed\n");
  }

  DBC *cursorp;

  dbp->cursor(dbp,0,&cursorp,0);

  DBT key, data;

  memset(&key, 0, sizeof(DBT));
  memset(&data, 0, sizeof(DBT));

  int nkey = 0;

  while ((ret = cursorp->get(cursorp, &key, &data, DB_NEXT)) == 0) {

    int ret = 0;
    int i = 0;

    u_int32_t datasize = 0;
    datasize = key.size;
    void * pcurchar = key.data;
    unsigned char curchar;
    memcpy(&curchar,pcurchar,1);

    if (curchar != 3) {
      continue;
    }

    pcurchar += 1;
    char * type = malloc(curchar+1);
    memcpy(type,pcurchar,curchar);
    *(type+3) = '\0';
    pcurchar += 3;

    if ((strcmp(type,"key") != 0) && (strcmp(type,"wkey") != 0)) {
      continue;
    }

    memcpy(&curchar,pcurchar,1);

    int publen = curchar;

    if (publen != 65) {
      printf("UNFAMILIAR-PUBLIC-KEY\n");
      continue;
    }

    pcurchar +=1;

    unsigned char * pubkey = malloc(publen);
    memcpy(pubkey,pcurchar,publen);

    char * pubkey_bc = malloc(35);
    
    ret = 0;

    ret = pubkey_to_bc_format(pubkey,publen,pubkey_bc);

    printf("%s\t", pubkey_bc+ret);

    pcurchar = data.data;
    memcpy(&curchar,pcurchar,1);

    if (curchar != 253) {
      printf("UNFAMILIAR-PRIVATE-KEY\n");
      continue;
    }

    pcurchar += 11;

    int privlen = 32;

    pcurchar += 1;

    unsigned char * privkey = malloc(privlen);
    memcpy(privkey,pcurchar,privlen);

    char * privkey_bc = malloc(52);
    
    ret = 0;

    ret = privkey_to_bc_format(privkey,privlen,pubkey,publen,privkey_bc);

    if (ret == -1) {
      printf("\n");
      continue;
    }

    printf("%s\n", privkey_bc+ret);

    free(type);
    free(pubkey);
    free(pubkey_bc);
    free(privkey);
    free(privkey_bc);

  }

  free(walletfile);

  if (ret != DB_NOTFOUND) {
    printf("error getting record\n");
  }

  if (cursorp != NULL) 
    cursorp->close(cursorp); 

  if (dbp != NULL) 
    dbp->close(dbp,0);

  return(0);
  
}
