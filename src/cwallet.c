#include <cwallet.h>

int publen = 33;
const int privlen = 32;

const char * b58chars = "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";
const char * b64chars = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz -";

int qrencode (char * bcaddress, char * bcprivkey, char * dvalue, char * ovalue, char * pvalue, unsigned char eflag) {

  int ret = 0;
  char * cmd = malloc(1000);

  if (dvalue != 0) {
    if (strlen(dvalue) > 450) {
      cmd = malloc(strlen(dvalue)+50);
    }
    strcpy(cmd,"mkdir -p '");
    strcat(cmd,dvalue);
    strcat(cmd,"'");
    ret = system(cmd);
    if (ret != 0) {
      return(ret);
    }
    ret = chdir(dvalue);
    if (ret != 0) {
      return(ret);
    }
  }

  ret = system("mkdir -p .work");
  if (ret != 0) {
    return(ret);
  }
  ret = chdir(".work");
  if (ret != 0) {
    return(ret);
  }

  char * pdffile = 0;
  if (ovalue != 0) {
    pdffile = ovalue;
  }
  else {
    pdffile = (char *)malloc(50);
    strcpy(pdffile,bcaddress);
    strcat(pdffile,".pdf");
  }
  
  strcpy(cmd,"qrencode -o '");
  strcat(cmd,bcaddress);
  strcat(cmd,".png' -s 12 -l H '");
  strcat(cmd,bcprivkey);
  strcat(cmd,"'");
  ret = system(cmd);
  if (ret != 0) {
    return(ret);
  }

  if ((pvalue != 0) && (eflag == 1)) {

    strcpy(cmd,"qrencode -o '");
    strcat(cmd,bcprivkey);
    strcat(cmd,".png' -s 3 -l H '");
    strcat(cmd,pvalue);
    strcat(cmd,"'");
    ret = system(cmd);
    if (ret != 0) {
      return(ret);
    }
  }
  
  strcpy(cmd,"echo '\\documentclass{article}\\usepackage{graphicx}\\usepackage[top=1in, bottom=1in, left=0in, right=0in]{geometry}\\begin{document}\\pagenumbering{gobble}{\\centering ");
  strcat(cmd,bcaddress);
  strcat(cmd,"\\\\\\includegraphics{");
  strcat(cmd,bcaddress);
  strcat(cmd,".png}\\\\");
  strcat(cmd,bcprivkey);
  if ((pvalue != 0) && (eflag ==1)) {
    strcat(cmd,"\\par}\\newpage{\\centering ");
    strcat(cmd,bcaddress);
    strcat(cmd,"\\\\");
    strcat(cmd,bcprivkey);
    strcat(cmd,"\\\\\\includegraphics{");
    strcat(cmd,bcprivkey);
    strcat(cmd,".png}\\\\");
    strcat(cmd,pvalue);
  }
  strcat(cmd,"\\par}\\end{document}' > ");
  strcat(cmd,bcaddress);
  strcat(cmd,".tex");
  ret = system(cmd);
  if (ret != 0) {
    return(ret);
  }

  strcpy(cmd,"pdflatex ");
  strcat(cmd,bcaddress);
  strcat(cmd,".tex >> /dev/null 2>> /dev/null");
  ret = system(cmd);
  if (ret != 0) {
    return(ret);
  }
  
  char * oldname = (char *)malloc(50);
  strcpy(oldname,bcaddress);
  strcat(oldname,".pdf");
  char * newname = (char *)malloc(50);
  strcpy(newname,"../");
  if (ovalue !=0) {
    if (strcmp(ovalue,oldname)!=0) {
      strcat(newname,ovalue);
    }
  }
  else {
    strcat(newname,oldname);
  }
  ret = rename(oldname,newname);
  if (ret != 0) {
    return(ret);
  }

  ret = chdir("..");
  if (ret != 0) {
    return(ret);
  }
  ret = system("rm -r .work");
  if (ret != 0) {
    return(ret);
  }

  free(oldname);
  free(newname);
  free(cmd);
  return(0);
}

int indexof(const char c, const char * str) {
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

int generate_key(unsigned char qflag, unsigned char rflag, unsigned char eflag, unsigned char uflag, char * dvalue, char * ovalue, char * tvalue, char * pvalue, char * kvalue, char ** result) {

  int ret = 0;
  
  int i = 0;
  unsigned char tvaluei = 0;
  if (tvalue==0) {
    tvaluei=0;
  }
  else {
    tvaluei=(unsigned char)(atoi(tvalue));
  }
  if (uflag==1) {
    publen = 65;
  }
  result[0] = (char *)malloc(200);
  result[1] = (char *)malloc(100);
  strcpy(result[0],"");
  strcpy(result[1],"");

  if (rflag == 1) {
    unsigned char * privkey = malloc(privlen);
    int randomData = open("/dev/random", O_RDONLY);
    size_t randomDataLen = 0;
    while (randomDataLen < privlen) {
      ssize_t res = read(randomData, privkey + randomDataLen, privlen - randomDataLen);
      if (res < 0) {
	strcpy(result[0],"Cannot read /dev/random\n");
	return(1);
      }
      randomDataLen += res;
    }
    close(randomData);
    
    unsigned char * pubkey = malloc(publen);
    ret = priv_to_pub(privkey,privlen,publen,&pubkey);
    char * pubkey_bc = malloc(35);
    ret = pubkey_to_bc_format(pubkey,publen,pubkey_bc,tvaluei);
    if (ret==-1) {
      strcpy(result[0],"Cannot convert public key to address\n");
      return(1);
    }
    char * bcaddress = pubkey_bc+ret;
    strcpy(result[0],bcaddress);

    if (pvalue != 0) {
      
      int passlen = strlen(pvalue);
      if (passlen > 42) {
	strcpy(result[0],"The passphrase cannot be longer than 42 characters\n");
	return(1);
      }
      for (i=0;i<passlen;i++) {
	if (indexof(pvalue[i],b64chars)==-1) {
	  strcpy(result[0],"The passphrase may only contain letters, numbers, spaces, and hyphens\n");
	  return(1);
	}
      }

      unsigned char * password_uchar = (unsigned char *)malloc(privlen+1);
      ret = b64_to_uchar(pvalue,strlen(pvalue),password_uchar,privlen);

      unsigned char * privkey_encrypted= (unsigned char *)malloc(privlen);
      ret = add_uchars(privkey,password_uchar,privlen,privkey_encrypted);

      unsigned char * privkey_decrypted = (unsigned char *)malloc(privlen);
      ret = subtract_uchars(privkey_encrypted,password_uchar,privlen,privkey_decrypted);

      for (i=0;i<privlen;i++) {
	if (privkey_decrypted[i] != privkey[i]) {
	  strcpy(result[0],"Cannot encrypt private key\n");
	  return(1);
	}
      }

      free(password_uchar);
      free(privkey);
      privkey = privkey_encrypted;
    }

    char * privkey_bc = malloc(53);
    ret = privkey_to_bc_format(privkey,privlen,pubkey,publen,privkey_bc,0);
    if (ret==-1) {
      strcpy(result[0],"Cannot convert private key to the Bitcoin format\n");
      return(1);
    }
    else if (ret==-2) {
      strcpy(result[0],"Private key does not match public key\n");
      return(1);
    }
    char * bcprivkey = privkey_bc + ret;
    strcpy(result[1],bcprivkey);

    if (qflag == 1) {
      ret = qrencode(bcaddress,bcprivkey,dvalue,ovalue,pvalue,eflag);
      if (ret!=0) {
	strcpy(result[0],"Cannot qr-encode\n");
	return(1);
      }
    }
  }

  else if (kvalue != 0) {
    
    unsigned char * privkey_1 = malloc(privlen+7);
    int privkey_offset = b58_to_uchar(privkey_1, privlen+6, kvalue, strlen(kvalue));
    privkey_1 += privkey_offset;

    unsigned char * hash1 = malloc(32);
    SHA256(privkey_1,privlen+1+1-privkey_offset,hash1);
    unsigned char * hash2 = malloc(32);
    SHA256(hash1,32,hash2);
    privkey_1 += 1;
    unsigned char * privkey_1_hash = privkey_1+privlen+1-privkey_offset;
    for (i=0;i<4;i++) {
      if (privkey_1_hash[i]!=hash2[i]) {
	strcpy(result[0],"Invalid private key\n");
	return(1);
      }
    }

    unsigned char * privkey_2 = 0;

    if (pvalue == 0) {
      privkey_2 = privkey_1;
    }
    else {
      privkey_2 = (unsigned char *)malloc(privlen);
      unsigned char * password_uchar = (unsigned char *)malloc(privlen+1);
      ret = b64_to_uchar(pvalue,strlen(pvalue),password_uchar,privlen);
      if (eflag == 1) {
	ret = add_uchars(privkey_1,password_uchar,privlen,privkey_2);
      }
      else {
	ret = subtract_uchars(privkey_1,password_uchar,privlen,privkey_2);
      }
    }

    if (privkey_offset == 1) {
      publen = 65;
    }
    else {
      publen = 33;
    }

    unsigned char * pubkey = malloc(publen);
    if (eflag == 1) {
      ret = priv_to_pub(privkey_1,privlen,publen,&pubkey);
    }
    else {
      ret = priv_to_pub(privkey_2,privlen,publen,&pubkey);
    }
    char * pubkey_bc = malloc(35);
    ret = pubkey_to_bc_format(pubkey,publen,pubkey_bc,tvaluei);
    if (ret==-1) {
      strcpy(result[0],"Cannot convert public key to address\n");
      return(1);
    }
    char * bcaddress = pubkey_bc+ret;
    strcpy(result[0],bcaddress);

    char * privkey_bc = malloc(53);
    ret = privkey_to_bc_format(privkey_2,privlen,pubkey,publen,privkey_bc,0);
    if (ret==-1) {
      strcpy(result[0],"Cannot convert private key to the Bitcoin format\n");
      return(1);
    }
    else if (ret==-2) {
      strcpy(result[0],"Private key does not match public key\n");
      return(1);
    }
    char * bcprivkey = privkey_bc + ret;
    strcpy(result[1],bcprivkey);

    if (qflag == 1) {
      ret = qrencode(bcaddress,bcprivkey,dvalue,ovalue,pvalue,eflag);
      if (ret!=0) {
        strcpy(result[0],"Cannot qr-encode\n");
        return(1);
      }
    }
  }
  return(0);
  
}

int priv_to_pub(const unsigned char * priv, size_t n, size_t m, unsigned char ** result) {

  int ret = 0;
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

  EC_GROUP_free(group);
  EC_POINT_free(pub_key);
  BN_CTX_free(ctx);
  BN_free(privbn);
  BN_free(pow256);
  BN_free(one256);

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
    char * lvalmod256_str = BN_bn2dec(lvalmod256);
    int lvalmod256int = atoi(lvalmod256_str);
    *addrb256i = lvalmod256int;
    addrb256i--;
    BN_free(lvalmod256);
    BN_CTX_free(ctx);
    free(lvalmod256_str);
    offset--;
  }
  char * lval_str = BN_bn2dec(lval);
  int lvalint = atoi(lval_str);
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

  BN_free(lval);
  BN_free(pow58);
  BN_free(one256);
  BN_free(one58);
  free(lval_str);

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
    char * lvalmod58_str = BN_bn2dec(lvalmod58);
    int lvalmod58int = atoi(lvalmod58_str);
    *addrb58i = b58chars[lvalmod58int];
    addrb58i--;
    BN_free(lvalmod58);
    BN_CTX_free(ctx);
    free(lvalmod58_str);
    offset--;
  }
  char * lval_str = BN_bn2dec(lval);
  int lvalint = atoi(lval_str);
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

  unsigned char * result_uchar = (unsigned char *)malloc(n+1);
  int ret = b58_to_uchar(result_uchar,n,result+offset,nr-offset);
  addri = uchar;
  const unsigned char * addri_chk = result_uchar;
  for (i=0; i<n; i++) {
    if (*addri_chk != *addri) {
      return(-1);
    }
    addri++;
    addri_chk++;
  }

  free(uchar);
  BN_free(lval);
  BN_free(pow256);
  BN_free(one256);
  BN_free(one58);
  free(lval_str);
  free(result_uchar);

  return(offset);
}


int b64_to_uchar(char * b64, size_t n, unsigned char * result, size_t nr) {

  BIGNUM * lval = 0;
  BN_dec2bn(&lval,"0");
  BIGNUM * pow64 = 0;
  BN_dec2bn(&pow64,"1");
  BIGNUM * one64 = 0;
  BN_dec2bn(&one64,"64");

  int i = 0;
  const unsigned char * addri = b64 + n - 1;
  for (i=0; i<n; i++) {
    BIGNUM * addribn = 0;
    char * addristr = malloc(4);
    sprintf(addristr,"%d",indexof(*addri,b64chars));
    BN_dec2bn(&addribn,addristr);
    BN_CTX * ctx = BN_CTX_new();
    BN_mul(addribn, pow64, addribn, ctx);
    BN_add(lval,lval,addribn);
    BN_mul(pow64, pow64, one64, ctx);
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
    char * lvalmod256_str = BN_bn2dec(lvalmod256);
    int lvalmod256int = atoi(lvalmod256_str);
    *addrb256i = lvalmod256int;
    addrb256i--;
    BN_free(lvalmod256);
    BN_CTX_free(ctx);
    free(lvalmod256_str);
    offset--;
  }
  char * lval_str = BN_bn2dec(lval);
  int lvalint = atoi(lval_str);
  *addrb256i = lvalint;
  offset--;

  addri = b64;
  for (i=0; i<n; i++) {
    if (*addri == '0') {
      addrb256i--;
      *addrb256i = 0;
      offset--;
      addri++;
    }
    else {
      break;
    }
  }

  while (offset>0) {
    addrb256i--;
    *addrb256i = 0;
    offset--;
  }

  BN_free(lval);
  BN_free(pow64);
  BN_free(one256);
  BN_free(one64);
  free(lval_str);
  
}

int add_uchars(unsigned char * c1, unsigned char * c2, size_t n, unsigned char * result) {
  int i;
  for (i=0; i<n; i++) {
    result[i] = (unsigned char)(((unsigned int)(c1[i])+(unsigned int)(c2[i]))%256);
  }
  return(0);
}

int subtract_uchars(unsigned char * c1, unsigned char * c2, size_t n, unsigned char * result) {
  int i;
  for (i=0; i<n; i++) {
    int result_int = (((int)(c1[i])-(int)(c2[i]))%256);
    if (result_int < 0) {
      result[i] = (unsigned char)(result_int + 256);
    }
    else {
      result[i] = (unsigned char)result_int;
    }
  }
  return(0);
}

int privkey_to_bc_format(const unsigned char * key, size_t n, unsigned char * pubkey, size_t m, char * result, unsigned char check) {

  int ret = 0;
  int i = 0;

  if (check == 1) {
    unsigned char * pubcheck = malloc(m);
    ret = priv_to_pub(key,n,m,&pubcheck);
    unsigned char * pubkeyi = pubkey;
    unsigned char * pubchecki = pubcheck;
    for (i=0; i<m; i++) {
      if (*pubkeyi != *pubchecki) {
	return(-2);
      }
      pubkeyi++;
      pubchecki++;
    }
    free(pubcheck);
  }

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

int pubkey_to_bc_format(const unsigned char * key, size_t n, char * result, unsigned char type) {

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
