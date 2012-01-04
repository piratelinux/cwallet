#include <util.h>

int main(int argc, char ** argv) {

  char * walletfile = 0;
  int addressmatch = 0;

  int wflag = 0;
  int dflag = 0;
  int aflag = 0;
  int qflag = 0;
  int oflag = 0;
  char * wvalue = 0;
  char * dvalue = 0;
  char * avalue = 0;
  char * ovalue = 0;
  int index;
  int c;
     
  opterr = 0;

  while ((c = getopt (argc, argv, "w:d:a:qo:")) != -1) {
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
      case '?':
	if ((optopt == 'w') || (optopt == 'd') || (optopt == 'a'))
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
  
  //printf ("wvalue = %s, dvalue = %s, avalue = %s, qflag = %d, ovalue = %s\n",wvalue, dvalue, avalue, qflag, ovalue);
    
  for (index = optind; index < argc; index++) {
    printf ("Non-option argument %s\n", argv[index]);
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

    int ret2 = 0;
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
    
    ret2 = 0;

    ret2 = pubkey_to_bc_format(pubkey,publen,pubkey_bc);

    char * bcaddress = pubkey_bc+ret2;

    if ((aflag == 1) && (strcmp(bcaddress,avalue) != 0)) {
      continue;
    }

    printf("%s\t", bcaddress);

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
    
    ret2 = 0;

    ret2 = privkey_to_bc_format(privkey,privlen,pubkey,publen,privkey_bc);

    if (ret2 == -1) {
      printf("\n");
      continue;
    }

    addressmatch = 1;

    char * bcprivkey = privkey_bc + ret2;

    printf("%s\n", bcprivkey);

    if ((qflag == 1) && (aflag == 1)) {

      char * cmd = malloc(500);

      if (dflag == 1) {
	if (strlen(dvalue) > 450) {
	  cmd = malloc(strlen(dvalue)+50);
	}
	strcpy(cmd,"mkdir -p '");
	strcat(cmd,dvalue);
	strcat(cmd,"'");
	ret2 = system(cmd);
	ret2 = chdir(dvalue);
      }

      char * pngfile = 0;
      if (oflag == 1) {
	pngfile = ovalue;
      }
      else {
	pngfile = malloc(39);
	strcpy(pngfile,bcaddress);
	strcat(pngfile,".png");
      }

      strcpy(cmd,"qrencode -s 20 -o \"");
      strcat(cmd,pngfile);
      strcat(cmd,"\" \"");
      strcat(cmd,bcprivkey);
      strcat(cmd,"\"");
      ret2 = system(cmd);

      strcpy(cmd,"convert \"");
      strcat(cmd,pngfile);
      strcat(cmd,"\" -draw \"text 50,20 '");
      strcat(cmd,bcaddress);
      strcat(cmd,"'\" \"");
      strcat(cmd,pngfile);
      strcat(cmd,"\"");
      ret2 = system(cmd);

      strcpy(cmd,"convert \"");
      strcat(cmd,pngfile);
      strcat(cmd,"\" -draw \"text 50,730 '");
      strcat(cmd,bcprivkey);
      strcat(cmd,"'\" \"");
      strcat(cmd,pngfile);
      strcat(cmd,"\"");
      ret2 = system(cmd);

      strcpy(cmd,"convert \"");
      strcat(cmd,pngfile);
      strcat(cmd,"\" \"");
      strcat(cmd,bcaddress);
      strcat(cmd,".pdf\"");
      ret2 = system(cmd);

      remove(pngfile);

      free(cmd);

    }

    free(type);
    free(pubkey);
    free(pubkey_bc);
    free(privkey);
    free(privkey_bc);

    if (aflag == 1) {
      ret = DB_NOTFOUND;
      break;
    }

  }

  free(walletfile);

  if ((aflag == 1) && (addressmatch == 0)) {
    fprintf (stderr,"address not found\n");
    return(1);
  }

  if (ret != DB_NOTFOUND) {
    printf("error getting record\n");
  }

  if (cursorp != NULL) 
    cursorp->close(cursorp); 

  if (dbp != NULL) 
    dbp->close(dbp,0);

  return(0);
  
}
