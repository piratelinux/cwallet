#include <cwallet.h>

/* Generate an address-private key pair or get one from a bitcoind wallet.dat
   -w [wallet file]
   -d [output directory]
   -a [address]
   -q to qrencode
   -o [output file]
   -t [address type]
   -r to generate a random private key
   -p [passphrase]
   -k [private key]
   -e to encrypt
   -u to use uncompressed public keys
   Return 0 if successful */
int main(int argc, char ** argv) {

  char * walletfile = 0;
  int addressmatch = 0;

  unsigned char wflag = 0;
  unsigned char dflag = 0;
  unsigned char aflag = 0;
  unsigned char qflag = 0;
  unsigned char oflag = 0;
  unsigned char tflag = 0;
  unsigned char rflag = 0;
  unsigned char pflag = 0;
  unsigned char kflag = 0;
  unsigned char eflag = 0;
  unsigned char uflag = 0;
  char * wvalue = 0;
  char * dvalue = 0;
  char * avalue = 0;
  char * ovalue = 0;
  char * tvalue = 0;
  char * pvalue = 0;
  char * kvalue = 0;
  int index;
  int c;
     
  opterr = 0;

  while ((c = getopt (argc, argv, "w:d:a:qo:t:rp:k:eu")) != -1) {
    switch (c) {
    case 'w':
      wflag = 1;
      wvalue = optarg;
      break;
    case 'd':
      dflag = 1;
      dvalue = optarg;
      break;
    case 'a':
      aflag = 1;
      avalue = optarg;
      break;
    case 'q':
      qflag = 1;
      break;
    case 'o':
      oflag = 1;
      ovalue = optarg;
      break;
    case 't':
      tflag = 1;
      tvalue = optarg;
      break;
    case 'r':
      rflag = 1;
      break;
    case 'p':
      pflag = 1;
      pvalue = optarg;
      break;
    case 'k':
      kflag = 1;
      kvalue = optarg;
      break;
    case 'e':
      eflag = 1;
      break;
    case 'u':
      uflag = 1;
      break;
    case '?':
      if ((optopt == 'w') || (optopt == 'd') || (optopt == 'a') || (optopt == 't') || (optopt == 'p') || (optopt == 'k'))
	fprintf (stderr,"Option -%c requires an argument.\n",optopt);
      else if (isprint (optopt))
	fprintf (stderr,"Unknown option `-%c'.\n", optopt);
      else
	fprintf (stderr,"Unknown option character `\\x%x'.\n",optopt);
      return 1;
    default:
      abort ();
    }
  }
  
  for (index = optind; index < argc; index++) {
    printf ("Non-option argument %s\n", argv[index]);
  }

  int ret = 0;

  if ((wflag==0) && (aflag==0) && ((rflag==1)||(kflag==1))) {
    char ** generate_result = (char **)malloc(sizeof(char **)*2);
    ret = generate_key(qflag, rflag, eflag, uflag, dvalue, ovalue, tvalue, pvalue, kvalue, generate_result);
    if (ret == 0) {
      fprintf(stdout,"%s\t%s\n",generate_result[0],generate_result[1]);
      fflush(stdout);
    }
    else {
      fprintf(stderr,"%s",generate_result[0]);
      fflush(stderr);
    }
    free(generate_result[0]);
    free(generate_result[1]);
    free(generate_result);
    return(ret);
  }

  int i = 0;
  unsigned char tvalue_uchar = 0;
  if (tflag==0) {
    tvalue_uchar=0;
  }
  else {
    tvalue_uchar = (unsigned char)(atoi(tvalue));
  }
  if (uflag==1) {
    publen = 65;
  }

  if (wflag == 1) {
    walletfile = strdup(wvalue);
  }
  else {
    char * homedir = getenv("HOME");
    walletfile = malloc(strlen(homedir)+21);
    strcpy(walletfile,homedir);
    strcat(walletfile,"/.bitcoin/wallet.dat");
  }

  DB * dbp = 0;
  u_int32_t flags = 0;
  
  ret = db_create(&dbp,0,0);

  if (ret != 0) {
    fprintf(stderr,"Failed to create database\n");
  }

  flags = DB_RDONLY;

  ret = dbp->open(dbp,0,walletfile,"main",DB_BTREE,flags,0);

  if (ret != 0) {
    fprintf(stderr,"Failed to open database\n");
    return(1);
  }

  DBC * cursorp = 0;

  dbp->cursor(dbp,0,&cursorp,0);

  DBT key, data;

  memset(&key, 0, sizeof(DBT));
  memset(&data, 0, sizeof(DBT));

  int nkey = 0;

  char * output = (char *)malloc(100);
  strcpy(output,"");

  while ((ret = cursorp->get(cursorp, &key, &data, DB_NEXT)) == 0) {

    int ret2 = 0;

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
      free(type);
      continue;
    }

    memcpy(&curchar,pcurchar,1);

    int publen = curchar;

    if ((publen != 65) && (publen != 33)) {
      strcat(output,"(UNFAMILIAR PUBLIC KEY)\n");
      fprintf(stdout,"%s",output);
      free(type);
      continue;
    }

    pcurchar +=1;

    /* Get the address twice and compare */
    char ** bc_address = (char **)malloc(sizeof(char *)*2);
    char ** pubkey_bc = (char **)malloc(sizeof(char *)*2);
    unsigned char ** pubkey = (unsigned char **)malloc(sizeof(unsigned char *)*2);
    unsigned char break_continue = 0;
    for (i=0; i<2; i++) {
      pubkey[i] = malloc(publen);
      memcpy(pubkey[i],pcurchar,publen);
      pubkey_bc[i] = malloc(35);
      ret2 = pubkey_to_bc_format(pubkey_bc[i],pubkey[i],publen,tvalue_uchar);
      if (ret2 == -1) {
	fprintf(stderr,"Cannot convert public key to address\n");
	break_continue = 1;
	break;
      }
      bc_address[i] = pubkey_bc[i] + ret2;
    }
    if (break_continue == 1) {
      continue;
    }
    break_continue = 0;
    i = 0;
    do {
      if (bc_address[0][i] != bc_address[1][i]) {
        fprintf(stderr,"Address generations inconsistent\n");
	break_continue = 1;
	break;
      }
      i++;
    } while (bc_address[0][i] != '\0');
    if (break_continue == 1) {
      continue;
    }
    for (i=0; i<publen; i++) {
      if (pubkey[0][i] != pubkey[1][i]) {
        fprintf(stderr,"Public keys inconsistent\n");
	break_continue = 1;
	break;
      }
    }
    if (break_continue == 1) {
      continue;
    }
    if ((aflag == 1) && (strcmp(bc_address[0],avalue) != 0)) {
      free(type);
      free(pubkey[0]);
      free(pubkey[1]);
      free(pubkey);
      free(pubkey_bc);
      free(bc_address);
      continue;
    }
    strcat(output,bc_address[0]);
    strcat(output,"\t");

    pcurchar = data.data;
    memcpy(&curchar,pcurchar,1);

    if ((curchar != 253) && (curchar != 214)) {
      strcat(output,"(UNFAMILIAR PRIVATE KEY)\n");
      fprintf(stdout,"%s",output);
      free(type);
      free(pubkey[0]);
      free(pubkey[1]);
      free(pubkey);
      free(pubkey_bc);
      free(bc_address);
      continue;
    }

    if (curchar == 253) {
      pcurchar += 11;
    }
    else {
      pcurchar += 8;
    }

    pcurchar += 1;

    /* Get the private key twice and compare */
    char ** bc_privkey = (char **)malloc(sizeof(char *)*2);
    char ** privkey_bc = (char **)malloc(sizeof(char *)*2);
    unsigned char ** privkey = (unsigned char **)malloc(sizeof(unsigned char *)*2);
    break_continue = 0;
    for (i=0; i<2; i++) {
      privkey[i] = malloc(privlen);
      memcpy(privkey[i],pcurchar,privlen);
      privkey_bc[i] = malloc(53);
      ret2 = privkey_to_bc_format(privkey_bc[i],privkey[i],privlen,pubkey[0],publen);
      if (ret2 == -1) {
	strcat(output,"(ERROR)\n");
	fprintf(stdout,"%s",output);
	break_continue = 1;
	break;
      }
      else if (ret2 == -2) {
	strcat(output,"(INVALID KEY)\n");
	fprintf(stdout,"%s",output);
	break_continue = 1;
	break;
      }
      bc_privkey[i] = privkey_bc[i] + ret2;
    }
    if (break_continue == 1) {
      continue;
    }
    addressmatch = 1;
    break_continue = 0;
    i = 0;
    do {
      if (bc_privkey[0][i] != bc_privkey[1][i]) {
        fprintf(stderr,"Bitcoin private key generations inconsistent\n");
        break_continue = 1;
	break;
      }
      i++;
    } while (bc_privkey[0][i] != '\0');
    if (break_continue == 1) {
      continue;
    }
    for (i=0; i<privlen; i++) {
      if (privkey[0][i] != privkey[1][i]) {
        fprintf(stderr,"Private keys inconsistent\n");
	break_continue = 1;
	break;
      }
    }
    if (break_continue == 1) {
      continue;
    }
    strcat(output,bc_privkey[0]);
    strcat(output,"\n");

    fprintf(stdout,"%s",output);
    strcpy(output,"");

    if ((qflag == 1) && (aflag == 1)) {
      ret2 = qrencode(bc_address[0],bc_privkey[0],dvalue,ovalue,pvalue,eflag);
    }

    free(type);
    free(pubkey[0]);
    free(pubkey[1]);
    free(privkey[0]);
    free(privkey[1]);
    free(pubkey);
    free(pubkey_bc[0]);
    free(pubkey_bc[1]);
    free(pubkey_bc);
    free(bc_address);
    free(privkey);
    free(privkey_bc[0]);
    free(privkey_bc[1]);
    free(privkey_bc);
    free(bc_privkey);

    if (aflag == 1) {
      ret = DB_NOTFOUND;
      break;
    }

  }

  free(walletfile);
  free(output);

  if ((aflag == 1) && (addressmatch == 0)) {
    fprintf(stderr,"Address not found\n");
    return(1);
  }

  if (ret != DB_NOTFOUND) {
    printf("Cannot get database record\n");
  }

  if (cursorp != NULL) 
    cursorp->close(cursorp); 

  if (dbp != NULL) 
    dbp->close(dbp,0);

  return(0);
}
