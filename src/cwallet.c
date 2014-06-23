#include <cwallet.h>

int publen = 33;
const int privlen = 32;

const char * b58chars = "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";
const char * b64chars = " 0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz-";

int fill_latex_lines (char * const target, const char * source, const int maxncharforline, const int startncharforline) {
  char * target_it = target;
  const char * source_it = source;
  int ncharforline = startncharforline;
  while(*source_it != '\0') {
    if (ncharforline >= maxncharforline) {
      memcpy(target_it,"\\\\",2);
      ncharforline = 0;
      target_it += 2;
    }
    if (*source_it == ' ') {
      memcpy(target_it,"\\symbol{32}",11);
      target_it += 11;
    }
    else {
      *target_it = *source_it;
      target_it++;
    }
    source_it++;
    ncharforline++;
  }
  *target_it = '\0';
  return(ncharforline);
}

int qrencode (const char * const bc_address, const char * const bc_privkey, const char * const dvalue, const char * const ovalue, const char * const pvalue, const unsigned char eflag, const unsigned char sflag) {

  int ret = 0;
  char * cmd = malloc(1000);

  if (dvalue != 0) {
    if (strlen(dvalue) > 950) {
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
  
  strcpy(cmd,"qrencode -o '");
  strcat(cmd,bc_address);
  strcat(cmd,".png' -s ");
  if (sflag == 1) {
    strcat(cmd,"3");
  }
  else {
    strcat(cmd,"12");
  }
  strcat(cmd," -l H '");
  strcat(cmd,bc_privkey);
  strcat(cmd,"'");
  ret = system(cmd);
  if (ret != 0) {
    return(ret);
  }

  if ((pvalue != 0) && (eflag == 1)) {
    strcpy(cmd,"qrencode -o '");
    strcat(cmd,bc_privkey);
    strcat(cmd,".png' -s 3 -l H '");
    strcat(cmd,pvalue);
    strcat(cmd,"'");
    ret = system(cmd);
    if (ret != 0) {
      return(ret);
    }
  }
  
  strcpy(cmd,"echo '\\documentclass{article}\\usepackage{graphicx}\\usepackage[top=1in, bottom=1in, left=0in, right=0in]{geometry}\\usepackage{multicol}\\begin{document}\\pagenumbering{gobble}");
  if (sflag == 1) {
    strcat(cmd,"\\newpage\\begin{multicols}{2}{\\raggedleft\\texttt{~\\\\");
    ret = fill_latex_lines(cmd+strlen(cmd),bc_privkey,6,0);
    int nchardiff = 6 - ret;
    while (nchardiff > 0) {
      strcat(cmd,".");
      nchardiff--;
    }
    strcat(cmd,"}\\par}\\columnbreak{\\raggedright\\includegraphics{");
    strcat(cmd,bc_address);
    strcat(cmd,".png}\\par}\\end{multicols}");
  }
  else {
    strcat(cmd,"{\\centering ");
    strcat(cmd,bc_address);
    strcat(cmd,"\\\\\\includegraphics{");
    strcat(cmd,bc_address);
    strcat(cmd,".png}\\\\");
    strcat(cmd,bc_privkey);
    strcat(cmd,"\\par}");
    if ((pvalue != 0) && (eflag == 1)) {
      strcat(cmd,"\\newpage\\begin{multicols}{2}{\\raggedleft\\texttt{~\\\\");
      ret = fill_latex_lines(cmd+strlen(cmd),bc_privkey,11,0);
      strcat(cmd,".");
      ret = fill_latex_lines(cmd+strlen(cmd),pvalue,11,ret+1);
      int nchardiff = 11 - ret;
      while (nchardiff > 0) {
	strcat(cmd,".");
	nchardiff--;
      }
      strcat(cmd,"}\\par}\\columnbreak{\\raggedright\\includegraphics{");
      strcat(cmd,bc_privkey);
      strcat(cmd,".png}\\par}\\end{multicols}");
    }
  }
  strcat(cmd,"\\end{document}' > ");
  strcat(cmd,bc_address);
  strcat(cmd,".tex");
  ret = system(cmd);
  if (ret != 0) {
    return(ret);
  }

  strcpy(cmd,"pdflatex ");
  strcat(cmd,bc_address);
  strcat(cmd,".tex >> /dev/null 2>> /dev/null");
  ret = system(cmd);
  if (ret != 0) {
    return(ret);
  }
  
  char * oldname = (char *)malloc(50);
  strcpy(oldname,bc_address);
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

int indexof (const char c, const char * str) {
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

int generate_key (char ** const result, const unsigned char qflag, const unsigned char rflag, const unsigned char eflag, const unsigned char uflag, const unsigned char sflag, const char * dvalue, const char * ovalue, const char * tvalue, const char * pvalue, const char * kvalue) {

  int ret = 0;
  
  int i = 0;
  unsigned char tvalue_uchar = 0;
  if (tvalue==0) {
    tvalue_uchar=0;
  }
  else {
    tvalue_uchar=(unsigned char)(atoi(tvalue));
  }
  if (uflag==1) {
    publen = 65;
  }
  result[0] = (char *)malloc(200);
  result[1] = (char *)malloc(100);
  strcpy(result[0],"");
  strcpy(result[1],"");

  if (rflag == 1) {
    if (eflag != 1) {
      strcpy(result[0],"A private key is needed to decrypt\n");
      return(1);
    }
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

    /* Get the address twice and compare */
    char ** bc_address = (char **)malloc(sizeof(char *)*2);
    char ** pubkey_bc = (char **)malloc(sizeof(char *)*2);
    for (i=0; i<2; i++) {
      unsigned char * pubkey = malloc(publen);
      ret = priv_to_pub(pubkey,publen,privkey,privlen);
      pubkey_bc[i] = malloc(35);
      ret = pubkey_to_bc_format(pubkey_bc[i],pubkey,publen,tvalue_uchar);
      if (ret==-1) {
	strcpy(result[0],"Cannot convert public key to address\n");
	return(1);
      }
      bc_address[i] = pubkey_bc[i] + ret;
      free(pubkey);
    }
    i = 0;
    do {
      if (bc_address[0][i] != bc_address[1][i]) {
        strcpy(result[0],"Address generations inconsistent\n");
        return(1);
      }
      i++;
    } while (bc_address[0][i] != '\0');
    strcpy(result[0],bc_address[0]);

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
      if (pvalue[0] == ' ') {
	strcpy(result[0],"The first character of the passphrase cannot be the space character\n");
	return(1);
      }

      unsigned char * password_uchar = (unsigned char *)malloc(privlen+1);
      ret = b64_to_uchar(password_uchar,privlen,pvalue,strlen(pvalue),1);
      if (ret < 0) {
	strcpy(result[0],"Cannot parse passphrase\n");
	return(1);
      }

      unsigned char * privkey_encrypted= (unsigned char *)malloc(privlen);
      ret = add_uchars(privkey_encrypted,privkey,password_uchar,privlen);

      unsigned char * privkey_decrypted = (unsigned char *)malloc(privlen);
      ret = subtract_uchars(privkey_decrypted,privkey_encrypted,password_uchar,privlen);

      for (i=0;i<privlen;i++) {
	if (privkey_decrypted[i] != privkey[i]) {
	  strcpy(result[0],"Cannot encrypt private key\n");
	  return(1);
	}
      }
      free(privkey_decrypted);
      free(password_uchar);
      free(privkey);
      privkey = privkey_encrypted;
    }

    /* Get the private key twice and compare */
    char ** bc_privkey = (char **)malloc(sizeof(char *)*2);
    char ** privkey_bc = (char **)malloc(sizeof(char *)*2);
    for (i=0; i<2; i++) {
      privkey_bc[i] = (char *)malloc(53);
      ret = privkey_to_bc_format(privkey_bc[i],privkey,privlen,0,0);
      if (ret==-1) {
	strcpy(result[0],"Cannot convert private key to the Bitcoin format\n");
	return(1);
      }
      else if (ret==-2) {
	strcpy(result[0],"Private key does not match public key\n");
	return(1);
      }
      bc_privkey[i] = privkey_bc[i] + ret;
    }    i = 0;
    do {
      if (bc_privkey[0][i] != bc_privkey[1][i]) {
        strcpy(result[0],"Private key generations inconsistent\n");
        return(1);
      }
      i++;
    } while (bc_privkey[0][i] != '\0');
    strcpy(result[1],bc_privkey[0]);

    if (qflag == 1) {
      ret = qrencode(bc_address[0],bc_privkey[0],dvalue,ovalue,pvalue,eflag,sflag);
      if (ret!=0) {
	strcpy(result[0],"Cannot qr-encode\n");
	return(1);
      }
    }

    free(privkey);
    free(pubkey_bc[0]);
    free(pubkey_bc[1]);
    free(pubkey_bc);
    free(privkey_bc[0]);
    free(privkey_bc[1]);
    free(privkey_bc);
    free(bc_address);
    free(bc_privkey);
  }

  else if (kvalue != 0) {
    
    unsigned char * privkey_1_full = malloc(privlen+7);
    int privkey_offset = b58_to_uchar(privkey_1_full,privlen+6,kvalue,strlen(kvalue),1);
    if (privkey_offset < 0) {
      strcpy(result[0],"Cannot parse private key\n");
      return(1);
    }
    unsigned char * privkey_1 = privkey_1_full + privkey_offset;

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
      if (pvalue[0] == ' ') {
        strcpy(result[0],"The first character of the passphrase cannot be a space\n");
        return(1);
      }

      privkey_2 = (unsigned char *)malloc(privlen);
      unsigned char * password_uchar = (unsigned char *)malloc(privlen+1);
      ret = b64_to_uchar(password_uchar,privlen,pvalue,strlen(pvalue),1);
      if (ret < 0) {
	strcpy(result[0],"Cannot parse passphrase\n");
        return(1);
      }
      unsigned char * privkey_1_check = (unsigned char *)malloc(privlen);
      if (eflag == 1) {
	ret = add_uchars(privkey_2,privkey_1,password_uchar,privlen);
	ret = subtract_uchars(privkey_1_check,privkey_2,password_uchar,privlen);
	for (i=0; i<privlen; i++) {
	  if (privkey_1[i] != privkey_1_check[i]) {
	    strcpy(result[0],"Cannot add passphrase to private key\n");
	    return(1);
	  }
	}
      }
      else {
	ret = subtract_uchars(privkey_2,privkey_1,password_uchar,privlen);
	ret = add_uchars(privkey_1_check,privkey_2,password_uchar,privlen);
	for (i=0; i<privlen; i++) {
          if (privkey_1[i] != privkey_1_check[i]) {
            strcpy(result[0],"Cannot subtract passphrase from private key\n");
            return(1);
          }
        }
      }
      free(privkey_1_check);
      free(password_uchar);
    }

    if (privkey_offset == 1) {
      publen = 65;
    }
    else {
      publen = 33;
    }

    /* Get the address twice and compare */
    char ** bc_address = (char **)malloc(sizeof(char *)*2);
    char ** pubkey_bc = (char **)malloc(sizeof(char *)*2);
    for (i=0; i<2; i++) {
      unsigned char * pubkey = malloc(publen);
      if (eflag == 1) {
	ret = priv_to_pub(pubkey,publen,privkey_1,privlen);
      }
      else {
	ret = priv_to_pub(pubkey,publen,privkey_2,privlen);
      }
      pubkey_bc[i] = malloc(35);
      ret = pubkey_to_bc_format(pubkey_bc[i],pubkey,publen,tvalue_uchar);
      if (ret==-1) {
	strcpy(result[0],"Cannot convert public key to address\n");
	return(1);
      }
      bc_address[i] = pubkey_bc[i] + ret;
      free(pubkey);
    }
    i = 0;
    do {
      if (bc_address[0][i] != bc_address[1][i]) {
	strcpy(result[0],"Address generations inconsistent\n");
	return(1);
      }
      i++;
    } while (bc_address[0][i] != '\0');
    strcpy(result[0],bc_address[0]);

    /* Get the private key twice and compare */
    char ** bc_privkey = (char **)malloc(sizeof(char *)*2);
    char ** privkey_bc = (char **)malloc(sizeof(char *)*2);
    for (i=0; i<2; i++) {
      privkey_bc[i] = (char *)malloc(53);
      ret = privkey_to_bc_format(privkey_bc[i],privkey_2,privlen,0,0);
      if (ret==-1) {
	strcpy(result[0],"Cannot convert private key to the Bitcoin format\n");
	return(1);
      }
      else if (ret==-2) {
	strcpy(result[0],"Private key does not match public key\n");
	return(1);
      }
      bc_privkey[i] = privkey_bc[i] + ret;
    }
    do {
      if (bc_privkey[0][i] != bc_privkey[1][i]) {
        strcpy(result[0],"Private key generations inconsistent\n");
        return(1);
      }
      i++;
    } while (bc_privkey[0][i] != '\0');
    strcpy(result[1],bc_privkey[0]);

    if (qflag == 1) {
      ret = qrencode(bc_address[0],bc_privkey[0],dvalue,ovalue,pvalue,eflag,sflag);
      if (ret!=0) {
        strcpy(result[0],"Cannot qr-encode\n");
        return(1);
      }
    }

    free(privkey_1_full);
    free(privkey_2);
    free(hash1);
    free(hash2);
    free(pubkey_bc[0]);
    free(pubkey_bc[1]);
    free(pubkey_bc);
    free(privkey_bc[0]);
    free(privkey_bc[1]);
    free(privkey_bc);
    free(bc_address);
    free(bc_privkey);
  }
  
  return(0);
}

int priv_to_pub (unsigned char * const result, const size_t m, const unsigned char * const priv, const size_t n) {

  int ret = 0;
  int i = 0;

  BIGNUM * privbn = 0;
  BN_dec2bn(&privbn,"0");
  BIGNUM * pow256 = 0;
  BN_dec2bn(&pow256,"1");
  BIGNUM * one256 = 0;
  BN_dec2bn(&one256,"256");

  const unsigned char * input_it = priv + n - 1;
  for (i=0; i<n; i++) {
    BIGNUM * input_bn = 0;
    char * input_str = malloc(4);
    sprintf(input_str,"%d",*input_it);
    BN_dec2bn(&input_bn,input_str);
    BN_CTX * ctx = BN_CTX_new();
    BN_mul(input_bn, pow256, input_bn, ctx);
    BN_add(privbn,privbn,input_bn);
    BN_mul(pow256, pow256, one256, ctx);
    BN_free(input_bn);
    if (input_str != 0) {
      free(input_str);
    }
    BN_CTX_free(ctx);
    input_it--;
  }

  EC_GROUP * group = EC_GROUP_new_by_curve_name(NID_secp256k1);
  EC_POINT * pub_key = EC_POINT_new(group);
  BN_CTX * ctx = BN_CTX_new();
  EC_POINT_mul(group, pub_key, privbn, 0, 0, ctx);
  if (m == 65) {
    EC_POINT_point2oct(group,pub_key,POINT_CONVERSION_UNCOMPRESSED,result,m,ctx);
  }
  else {
    EC_POINT_point2oct(group,pub_key,POINT_CONVERSION_COMPRESSED,result,m,ctx);
  }

  EC_GROUP_free(group);
  EC_POINT_free(pub_key);
  BN_CTX_free(ctx);
  BN_free(privbn);
  BN_free(pow256);
  BN_free(one256);

  return(0);
}

int b58_to_uchar (unsigned char * const result, const size_t m, const char * b58, const size_t n, const unsigned char check_reverse) {

  BIGNUM * tot_bn = 0;
  BN_dec2bn(&tot_bn,"0");
  BIGNUM * pow58 = 0;
  BN_dec2bn(&pow58,"1");
  BIGNUM * one58 = 0;
  BN_dec2bn(&one58,"58");

  int i = 0;
  const unsigned char * input_it = b58 + n - 1;
  for (i=0; i<n; i++) {
    BIGNUM * input_bn = 0;
    char * input_str = malloc(4);
    sprintf(input_str,"%d",indexof(*input_it,b58chars));
    BN_dec2bn(&input_bn,input_str);
    BN_CTX * ctx = BN_CTX_new();
    BN_mul(input_bn,pow58,input_bn,ctx);
    BN_add(tot_bn,tot_bn,input_bn);
    BN_mul(pow58,pow58,one58,ctx);
    BN_free(input_bn);
    if (input_str != 0) {
      free(input_str);
    }
    BN_CTX_free(ctx);
    input_it--;
  }

  unsigned char * output_it = result + m;
  *output_it = '\0';
  output_it--;

  BIGNUM * one256 = 0;
  BN_dec2bn(&one256,"256");

  int offset = m;
  while (BN_cmp(tot_bn,one256) >= 0) {
    BN_CTX * ctx = BN_CTX_new();
    BIGNUM * tot_mod_256 = BN_new();
    BN_div(tot_bn,tot_mod_256,tot_bn,one256,ctx);
    char * tot_mod_256_str = BN_bn2dec(tot_mod_256);
    *output_it = atoi(tot_mod_256_str);
    BN_free(tot_mod_256);
    BN_CTX_free(ctx);
    free(tot_mod_256_str);
    output_it--;
    offset--;
  }
  char * tot_str = BN_bn2dec(tot_bn);
  *output_it = atoi(tot_str);
  offset--;

  input_it = b58;
  for (i=1; i<n; i++) {
    if ((*input_it == '1') && (offset > 0)) {
      output_it--;
      *output_it = 0;
      offset--;
      input_it++;
    }
    else {
      break;
    }
  }

  if (check_reverse == 1) {
    unsigned char * result_b58 = (char *)malloc(n+1);
    int ret = uchar_to_b58(result_b58,n,result+offset,m-offset,0);
    if (ret < 0) {
      return(-1);
    }
    input_it = b58;
    const unsigned char * input_chk = result_b58 + ret;
    for (i=0; i<n; i++) {
      if (*input_chk != *input_it) {
        return(-1);
      }
      input_it++;
      input_chk++;
    }
    free(result_b58);
  }

  BN_free(tot_bn);
  BN_free(pow58);
  BN_free(one256);
  BN_free(one58);
  free(tot_str);

  return(offset);
}


int uchar_to_b58 (char * const result, const size_t m, const unsigned char * uchar, const size_t n, const unsigned char check_reverse) {

  BIGNUM * tot_bn = 0;
  BN_dec2bn(&tot_bn,"0");
  BIGNUM * pow256 = 0;
  BN_dec2bn(&pow256,"1");
  BIGNUM * one256 = 0;
  BN_dec2bn(&one256,"256");

  int i = 0;
  const unsigned char * input_it = uchar + n - 1;
  for (i=0; i<n; i++) {
    BIGNUM * input_bn = 0;
    char * input_str = malloc(4);
    sprintf(input_str,"%d",*input_it);
    BN_dec2bn(&input_bn,input_str);
    BN_CTX * ctx = BN_CTX_new();
    BN_mul(input_bn, pow256, input_bn, ctx);
    BN_add(tot_bn,tot_bn,input_bn);
    BN_mul(pow256, pow256, one256, ctx);
    BN_free(input_bn);
    if (input_str != 0) {
      free(input_str);
    }
    BN_CTX_free(ctx);
    input_it--;
  }

  char * output_it = result + m;
  *output_it = '\0';
  output_it--;

  BIGNUM * one58 = 0;
  BN_dec2bn(&one58,"58");

  int offset = m;
  while (BN_cmp(tot_bn,one58) >= 0) {
    BN_CTX * ctx = BN_CTX_new();
    BIGNUM * tot_mod_58 = BN_new();
    BN_div(tot_bn, tot_mod_58, tot_bn, one58, ctx);
    char * tot_mod_58_str = BN_bn2dec(tot_mod_58);
    *output_it = b58chars[atoi(tot_mod_58_str)];
    output_it--;
    BN_free(tot_mod_58);
    BN_CTX_free(ctx);
    free(tot_mod_58_str);
    offset--;
  }
  char * tot_str = BN_bn2dec(tot_bn);
  *output_it = b58chars[atoi(tot_str)];
  offset--;

  input_it = uchar;
  for (i=1; i<n; i++) {
    if ((*input_it == '\0') && (offset > 0)) {
      output_it--;
      *output_it = '1';
      offset--;
      input_it++;
    }
    else {
      break;
    }
  }

  if (check_reverse == 1) {
    unsigned char * result_uchar = (unsigned char *)malloc(n+1);
    int ret = b58_to_uchar(result_uchar,n,result+offset,m-offset,0);
    if (ret < 0) {
      return(-1);
    }
    input_it = uchar;
    const unsigned char * input_chk = result_uchar + ret;
    for (i=0; i<n; i++) {
      if (*input_chk != *input_it) {
	return(-1);
      }
      input_it++;
      input_chk++;
    }
    free(result_uchar);
  }

  BN_free(tot_bn);
  BN_free(pow256);
  BN_free(one256);
  BN_free(one58);
  free(tot_str);

  return(offset);
}


int b64_to_uchar (unsigned char * const result, const size_t m, const char * b64, const size_t n, const unsigned char check_reverse) {

  BIGNUM * tot_bn = 0;
  BN_dec2bn(&tot_bn,"0");
  BIGNUM * pow64 = 0;
  BN_dec2bn(&pow64,"1");
  BIGNUM * one64 = 0;
  BN_dec2bn(&one64,"64");

  int i = 0;
  const unsigned char * input_it = b64 + n - 1;
  for (i=0; i<n; i++) {
    BIGNUM * input_bn = 0;
    char * input_str = malloc(4);
    sprintf(input_str,"%d",indexof(*input_it,b64chars));
    BN_dec2bn(&input_bn,input_str);
    BN_CTX * ctx = BN_CTX_new();
    BN_mul(input_bn, pow64, input_bn, ctx);
    BN_add(tot_bn,tot_bn,input_bn);
    BN_mul(pow64, pow64, one64, ctx);
    BN_free(input_bn);
    if (input_str != 0) {
      free(input_str);
    }
    BN_CTX_free(ctx);
    input_it--;
  }

  unsigned char * output_it = result + m;
  *output_it = '\0';
  output_it--;

  BIGNUM * one256 = 0;
  BN_dec2bn(&one256,"256");

  int offset = m;
  while (BN_cmp(tot_bn,one256) >= 0) {
    BN_CTX * ctx = BN_CTX_new();
    BIGNUM * tot_mod_256 = BN_new();
    BN_div(tot_bn, tot_mod_256, tot_bn, one256, ctx);
    char * tot_mod_256_str = BN_bn2dec(tot_mod_256);
    *output_it = atoi(tot_mod_256_str);
    BN_free(tot_mod_256);
    BN_CTX_free(ctx);
    free(tot_mod_256_str);
    output_it--;
    offset--;
  }
  char * tot_str = BN_bn2dec(tot_bn);
  *output_it = atoi(tot_str);
  offset--;

  input_it = b64;
  for (i=1; i<n; i++) {
    if ((*input_it == b64chars[0]) && (offset > 0)) {
      output_it--;
      *output_it = 0;
      offset--;
      input_it++;
    }
    else {
      break;
    }
  }

  if (check_reverse == 1) {
    unsigned char * result_b64 = (char *)malloc(n+1);
    int ret = uchar_to_b64(result_b64,n,result+offset,m-offset,0);
    if (ret < 0) {
      return(-1);
    }
    input_it = b64;
    const char * input_chk = result_b64 + ret;
    for (i=0; i<n; i++) {
      if (*input_chk != *input_it) {
        return(-1);
      }
      input_it++;
      input_chk++;
    }
    free(result_b64);
  }

  int offset_saved = offset;
  while (offset > 0) {
    output_it--;
    *output_it = 0;
    offset--;
  }

  BN_free(tot_bn);
  BN_free(pow64);
  BN_free(one256);
  BN_free(one64);
  free(tot_str);

  return(offset_saved);
}

int uchar_to_b64 (char * const result, const size_t m, const unsigned char * uchar, const size_t n, const unsigned char check_reverse) {

  BIGNUM * tot_bn = 0;
  BN_dec2bn(&tot_bn,"0");
  BIGNUM * pow256 = 0;
  BN_dec2bn(&pow256,"1");
  BIGNUM * one256 = 0;
  BN_dec2bn(&one256,"256");

  int i = 0;
  const unsigned char * input_it = uchar + n - 1;
  for (i=0; i<n; i++) {
    BIGNUM * input_bn = 0;
    char * input_str = malloc(4);
    sprintf(input_str,"%d",*input_it);
    BN_dec2bn(&input_bn,input_str);
    BN_CTX * ctx = BN_CTX_new();
    BN_mul(input_bn, pow256, input_bn, ctx);
    BN_add(tot_bn,tot_bn,input_bn);
    BN_mul(pow256, pow256, one256, ctx);
    BN_free(input_bn);
    if (input_str != 0) {
      free(input_str);
    }
    BN_CTX_free(ctx);
    input_it--;
  }

  char * output_it = result + m;
  *output_it = '\0';
  output_it--;

  BIGNUM * one64 = 0;
  BN_dec2bn(&one64,"64");

  int offset = m;
  while (BN_cmp(tot_bn,one64) >= 0) {
    BN_CTX * ctx = BN_CTX_new();
    BIGNUM * tot_mod_64 = BN_new();
    BN_div(tot_bn, tot_mod_64, tot_bn, one64, ctx);
    char * tot_mod_64_str = BN_bn2dec(tot_mod_64);
    *output_it = b64chars[atoi(tot_mod_64_str)];
    output_it--;
    BN_free(tot_mod_64);
    BN_CTX_free(ctx);
    free(tot_mod_64_str);
    offset--;
  }
  char * tot_str = BN_bn2dec(tot_bn);
  *output_it = b64chars[atoi(tot_str)];
  offset--;

  input_it = uchar;
  for (i=1; i<n; i++) {
    if ((*input_it == '\0') && (offset > 0)) {
      output_it--;
      *output_it = b64chars[0];
      offset--;
      input_it++;
    }
    else {
      break;
    }
  }

  if (check_reverse == 1) {
    unsigned char * result_uchar = (unsigned char *)malloc(n+1);
    int ret = b64_to_uchar(result_uchar,n,result+offset,m-offset,0);
    if (ret < 0) {
      return(-1);
    }
    input_it = uchar;
    const unsigned char * input_chk = result_uchar + ret;
    for (i=0; i<n; i++) {
      if (*input_chk != *input_it) {
	return(-1);
      }
      input_it++;
      input_chk++;
    }
    free(result_uchar);
  }

  BN_free(tot_bn);
  BN_free(pow256);
  BN_free(one256);
  BN_free(one64);
  free(tot_str);

  return(offset);
}

int add_uchars (unsigned char * const result, const unsigned char * c1, const unsigned char * c2, const size_t n) {
  int i;
  for (i=0; i<n; i++) {
    result[i] = (unsigned char)(((unsigned int)(c1[i])+(unsigned int)(c2[i]))%256);
  }
  return(0);
}

int subtract_uchars (unsigned char * const result, const unsigned char * c1, const unsigned char * c2, const size_t n) {
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

int privkey_to_bc_format (char * const result, const unsigned char * key, size_t n, const unsigned char * pubkey, const size_t m) {

  int ret = 0;
  int i = 0;

  if (pubkey != 0) {
    unsigned char * pubkey_check = malloc(m);
    ret = priv_to_pub(pubkey_check,m,key,n);
    const unsigned char * pubkey_it = pubkey;
    unsigned char * pubkey_check_it = pubkey_check;
    for (i=0; i<m; i++) {
      if (*pubkey_it != *pubkey_check_it) {
	return(-2);
      }
      pubkey_it++;
      pubkey_check_it++;
    }
    free(pubkey_check);
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
  ret = uchar_to_b58(result,52,addr,n_ext+4,1);
  free(addr);
  return(ret);
}

int pubkey_to_bc_format (char * const result, const unsigned char * key, const size_t n, const unsigned char type) {

  int ret = 0;
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
  ret = uchar_to_b58(result,34,addr,25,1);
  free(addr);
  return(ret);
}
