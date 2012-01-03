#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <gtk/gtk.h>

typedef struct _Data Data;
struct _Data
{
  GtkWindow * window;

  GtkHButtonBox * hbuttonbox;
  
  GtkProgressBar * progress;

  GtkLabel * message;

  GtkButton * button_enable;

  GtkEntry * entrywalletfile;
  GtkEntry * entryaddress;
  GtkEntry * entryoutdir;

  gchar * action;
  gchar * curpath;
  gchar * targetname;
  gchar * homedir;

  gchar * processpath;
  gchar * maindir;

  gchar ** argv;
  gint argc;
 
  gint timeout_id;
};

static void cb_execute_install(GtkButton *button, Data *data);
static void cb_execute_reinstall(GtkButton *button, Data *data);
static void cb_execute_remove(GtkButton *button, Data *data);
static void cb_execute_update(GtkButton *button, Data *data);

gchar* substring(const gchar* str, size_t begin, size_t len)
{
  if (str == 0 || strlen(str) == 0 || strlen(str) < begin || strlen(str) < (begin+len))
    return 0;

  return strndup(str + begin, len);
}

gchar* exec(gchar* cmd, gint size) {
  FILE* pipe = popen(cmd, "r");
  if (!pipe) return "ERROR";
  gchar * result = g_malloc(size);
  strcpy(result,"");
  gchar * ret = fgets(result,size,pipe);
  while(ret != 0) {
    size = size*2;
    result = g_realloc(result,size);
    ret = fgets(result,size,pipe);
  }
  pclose(pipe);
  return result;
}

gchar * get_readlink(gchar * procpath) {

    gint size = 100;
    gchar * buffer = g_malloc(size);

    do {
      int nchars = readlink(procpath, buffer, size);
      if (nchars < 0) {
	g_free(buffer);
	return(0);
      }
      if (nchars < size) {
	gchar * index = buffer + nchars;
	*index = '\0';
	return buffer;
      }
      size = size*2;
      buffer = g_realloc(buffer,size);
    } while (1);
}

gchar * get_dirname(gchar * path) {

  gchar * index = path + strlen(path) - 1;
  int len = strlen(path);
  while (*index != '/') {
    index = index - 1;
    len = len - 1;
  }
  index = index - 1;
  len = len - 1;
  return substring(path,0,len);

}

gint getargi(gchar * target, int argc, char ** argv) {
  int i = 1;
  while (i < argc) {
    if (strcmp(target,argv[i]) == 0) {
      return i;
    }
    i++;
  }
  return -1;
}

int numDigits(int num)
{
  int dig = 0;
  if (num < 0) dig = 1; // remove this line if '-' counts as a digit
  while (num) {
    num /= 10;
    dig++;
  }
  return dig;
}

static void
cb_child_watch( GPid  pid,
                gint  status,
                Data *data )
{
 
    /* Close pid */
    g_spawn_close_pid( pid );
}
 
static gboolean
cb_out_watch( GIOChannel   *channel,
              GIOCondition  cond,
              Data         *data )
{
    gchar *string;
    gsize  size;
 
    if( cond == G_IO_HUP )
    {
        g_io_channel_unref( channel );
        return( FALSE );
    }
 
    g_io_channel_read_line( channel, &string, &size, 0, 0 );

    //printf("out_watch:%s\n",string);

    g_free( string );
 
    return( TRUE );
}
 
static gboolean
cb_err_watch( GIOChannel   *channel,
              GIOCondition  cond,
              Data         *data )
{
    gchar *string;
    gsize  size;
 
    if( cond == G_IO_HUP ) {
        g_io_channel_unref( channel );
        return( FALSE );
    }
 
    g_io_channel_read_line( channel, &string, &size, 0, 0 );
    int len = strlen(string);
    if (*(string+len-1)=='\n') {
      *(string+len-1) = '\0';
    }
    gtk_label_set_label( data->message, string);
    g_free( string );

    if (strcmp(data->action,"install")==0) {
      gchar * submessage = substring(gtk_label_get_label(data->message),0,5);
      printf("message: %s\n", gtk_label_get_label(data->message));
      if (g_strcmp0("Saved",submessage) == 0) {

	/* Remove timeout callback */
	g_source_remove( data->timeout_id );
	gtk_progress_bar_set_fraction((GtkProgressBar *)data->progress, 0.0);
	g_free(data->action);

	const gchar * address = gtk_entry_get_text(data->entryaddress);
	const gchar * outdir = gtk_entry_get_text(data->entryoutdir);
	gchar * cmd = g_malloc(strlen(address)+strlen(outdir)+40);
	strcpy(cmd,"evince --presentation '");
	strcat(cmd,outdir);
	strcat(cmd,"/");
	strcat(cmd,address);
	strcat(cmd,".pdf' &");
	int ret = system(cmd);

	gtk_widget_set_sensitive((GtkWidget *)data->button_enable,TRUE);

      }
      else if (g_strcmp0("Error",submessage) == 0) {

	/* Remove timeout callback */
        g_source_remove( data->timeout_id );
        gtk_progress_bar_set_fraction((GtkProgressBar *)data->progress, 0.0);
        g_free(data->action);

	gtk_widget_set_sensitive((GtkWidget *)data->button_enable,TRUE);

      }

      g_free(submessage);
    }

    return( TRUE );
}
 
static gboolean
cb_timeout( Data *data )
{
    /* Bounce progress bar */
    gtk_progress_bar_pulse( data->progress );
 
    return( TRUE );
}
 
static void
cb_execute( GtkButton *button,
            Data      *data)
{

    GPid        pid;
    gchar    **argv;
    gint argc = -1;

    if (strcmp(data->action,"install") == 0) {
      argc = 6;
      argv = malloc(sizeof(gchar*) * 7);
      argv[0] = strdup(data->processpath);
      argv[1] = strdup("--install");
      argv[2] = strdup("--async");
      const gchar * walletfile = gtk_entry_get_text(data->entrywalletfile);
      argv[3] = g_malloc(strlen(walletfile)+14);
      strcpy(argv[3],"--walletfile=");
      strcat(argv[3],walletfile);
      const gchar * outdir = gtk_entry_get_text(data->entryoutdir);
      argv[4] = g_malloc(strlen(outdir)+10);
      strcpy(argv[4],"--outdir=");
      strcat(argv[4],outdir);
      const gchar * address = gtk_entry_get_text(data->entryaddress);
      argv[5] = g_malloc(strlen(address)+11);
      strcpy(argv[5],"--address=");
      strcat(argv[5],address);
      argv[6] = 0;
      gtk_widget_set_sensitive((GtkWidget *)data->button_enable,FALSE);
    }

    gint in, out, err;
    GIOChannel *in_ch, *out_ch, *err_ch;
    gboolean ret;
 
    /* Spawn child process */
    ret = g_spawn_async_with_pipes(0, argv, 0, G_SPAWN_DO_NOT_REAP_CHILD, 0, 0, &pid, &in, &out, &err, 0);
    if( ! ret ) {
      g_error( "SPAWN FAILED" );
      return;
    }

    /* Add watch function to catch termination of the process. This function
     * will clean any remnants of process. */
    g_child_watch_add( pid, (GChildWatchFunc)cb_child_watch, data );
 
    /* Create channels that will be used to read data from pipes. */
#ifdef G_OS_WIN32
    in_ch = g_io_channel_win32_new_fd( in );
    out_ch = g_io_channel_win32_new_fd( out );
    err_ch = g_io_channel_win32_new_fd( err );
#else
    in_ch = g_io_channel_unix_new( in );
    out_ch = g_io_channel_unix_new( out );
    err_ch = g_io_channel_unix_new( err );
#endif
 
    /* Add watches to channels */
    g_io_add_watch( out_ch, G_IO_IN | G_IO_HUP, (GIOFunc)cb_out_watch, data );
    g_io_add_watch( err_ch, G_IO_IN | G_IO_HUP, (GIOFunc)cb_err_watch, data );

    /* Install timeout function that will move the progress bar */
    data->timeout_id = g_timeout_add( 100, (GSourceFunc)cb_timeout, data );

    gsize * bytes_written = 0;
    GError * error = 0;
    g_io_channel_write_chars(in_ch, "ready\n", -1, bytes_written, &error);
    g_io_channel_flush(in_ch, &error);
    
    g_free(bytes_written);
    g_free(error);
    g_io_channel_unref(in_ch);
    int i=0;
    while (i < argc) {
      g_free(argv[i]);
      i++;
    }
    g_free(argv);
}

static void cb_execute_install( GtkButton *button,
				Data      *data)
{
 
  data->action = strdup("install");
  cb_execute(button,data);

}

int
install_pack(int argc, char **argv, Data * data)
{

  int ret;

  gchar * maindir = data->maindir;
  gchar * curpath = g_get_current_dir();
  gchar * homedir = data->homedir;

  gchar * walletfile = substring(argv[3],13,strlen(argv[3])-13);
  gchar * outdir = substring(argv[4],9,strlen(argv[4])-9);
  gchar * address = substring(argv[5],10,strlen(argv[5])-10);

  gchar * str = g_malloc(strlen(maindir)+strlen(walletfile)+strlen(outdir)+strlen(address)+100);

  strcpy(str,maindir);
  strcat(str,"/cwallet -w '");
  strcat(str,walletfile);
  strcat(str,"' -d '");
  strcat(str,outdir);
  strcat(str,"' -qa '");
  strcat(str,address);
  strcat(str,"'");

  ret = system(str);

  if (ret != 0) {
    strcpy(str,"Error. Check your address.");
  }
  else {
    strcpy(str,"Saved to ");
    strcat(str,address);
    strcat(str,".pdf");
  }

  fprintf( stderr, "%s\n", str );
  fflush(stderr);

  g_free(walletfile);
  g_free(outdir);
  g_free(address);
  g_free( curpath );
  g_free( str );

  return( 0 );
}

int gui_status(int argc, char ** argv, Data * data) {

  GtkWindow * window;
  GtkTable * table1, * table2, * table3;
  GtkButton * button, * button_disable, * button_enable, * button_update;
  GtkProgressBar * progress;
  GtkLabel * message = 0;
  GtkLabel * labelwalletfile = 0;
  GtkLabel * labeladdress = 0;
  GtkLabel * labeloutdir = 0;
  GtkImage * logo;
  GtkHButtonBox * hbuttonbox;
  GtkEntry * entrywalletfile = 0;
  GtkEntry * entryaddress = 0;
  GtkEntry * entryoutdir = 0;

  gtk_init( &argc, &argv );

  gchar * maindir = data->maindir;

  gchar * homedir = data->homedir;

  gchar * curpath = data->curpath;

  gchar * str = g_malloc(strlen(maindir)+50);

  window = (GtkWindow *)gtk_window_new( GTK_WINDOW_TOPLEVEL );
  gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
  gtk_window_set_default_size( GTK_WINDOW( window ), 500, 400 );
  gtk_window_set_title( GTK_WINDOW( window ), "Cwallet");

  strcpy(str,maindir);
  strcat(str,"/icon.png");

  gtk_window_set_icon_from_file( GTK_WINDOW( window ), str, 0);
  
  g_signal_connect( G_OBJECT( window ), "destroy", G_CALLBACK( gtk_main_quit ), 0 );
  
  table1 = (GtkTable *)gtk_table_new( 4, 5, FALSE );
  gtk_container_add((GtkContainer *)window, (GtkWidget *)table1);
  
  table2 = (GtkTable *)gtk_table_new( 3, 3, FALSE );
  gtk_table_attach( GTK_TABLE( table1 ), (GtkWidget *)table2, 1, 2, 0, 1, GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0 );

  strcpy(str,maindir);
  strcat(str,"/logo.png");

  logo = (GtkImage *)gtk_image_new_from_file(str);
  gtk_table_attach( GTK_TABLE( table2 ), (GtkWidget *)logo, 1, 2, 1, 2, GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 5, 5 );

  message = (GtkLabel *)gtk_label_new("Generate a PDF with your private key QR encoded");
  gtk_label_set_justify((GtkLabel *)message,GTK_JUSTIFY_CENTER);
  gtk_widget_set_size_request((GtkWidget *)message,-1,80);
  gtk_misc_set_alignment((GtkMisc *)message,0.5,1);
  gtk_table_attach( GTK_TABLE( table1 ), (GtkWidget *)message, 1, 2, 1, 2, GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 5, 5 );

  table3 = (GtkTable *)gtk_table_new( 2, 2, FALSE );
  gtk_table_set_row_spacings(table3,0.5);
  gtk_table_attach( GTK_TABLE( table1 ), (GtkWidget *)table3, 1, 2, 2, 3, GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 5, 5 );

  labelwalletfile = (GtkLabel *)gtk_label_new("Wallet file:");
  gtk_misc_set_alignment((GtkMisc *)labelwalletfile,0.,0.5);
  gtk_table_attach( GTK_TABLE( table3 ), (GtkWidget *)labelwalletfile, 0, 1, 0, 1, GTK_FILL, 0, 5, 5);

  entrywalletfile = (GtkEntry *)gtk_entry_new();
  gtk_entry_set_text(entrywalletfile,homedir);
  gtk_entry_append_text(entrywalletfile,"/.bitcoin/wallet.dat");
  gtk_table_attach( GTK_TABLE( table3 ), (GtkWidget *)entrywalletfile, 1, 2, 0, 1, GTK_EXPAND | GTK_FILL, 0, 5, 5 );
  data->entrywalletfile = entrywalletfile;

  labeloutdir = (GtkLabel *)gtk_label_new("Output directory:");
  gtk_misc_set_alignment((GtkMisc *)labeloutdir,0.,0.5);
  gtk_table_attach( GTK_TABLE( table3 ), (GtkWidget *)labeloutdir, 0, 1, 1, 2, GTK_FILL, 0, 5, 5);

  entryoutdir = (GtkEntry *)gtk_entry_new();
  gtk_entry_set_text(entryoutdir,homedir);
  gtk_entry_append_text(entryoutdir,"/cwallet");
  gtk_table_attach( GTK_TABLE( table3 ), (GtkWidget *)entryoutdir, 1, 2, 1, 2, GTK_EXPAND | GTK_FILL, 0, 5, 5 );
  data->entryoutdir = entryoutdir;

  labeladdress = (GtkLabel *)gtk_label_new("Address:");
  gtk_misc_set_alignment((GtkMisc *)labeladdress,0,0);
  gtk_table_attach( GTK_TABLE( table3 ), (GtkWidget *)labeladdress, 0, 1, 2, 3, GTK_FILL, 0, 5, 5 );

  entryaddress = (GtkEntry *)gtk_entry_new();
  gtk_table_attach( GTK_TABLE( table3 ), (GtkWidget *)entryaddress, 1, 2, 2, 3, GTK_EXPAND | GTK_FILL, 0, 5, 5 );
  data->entryaddress = entryaddress;

  progress = (GtkProgressBar *)gtk_progress_bar_new();
  gtk_table_attach( GTK_TABLE( table1 ),(GtkWidget *)progress, 1, 2, 3, 4, GTK_FILL, GTK_SHRINK | GTK_FILL, 5, 0 );
  hbuttonbox = (GtkHButtonBox *)gtk_hbutton_box_new();
  
  button_enable = (GtkButton *)gtk_button_new_with_label( "Generate" );
  gtk_widget_set_size_request((GtkWidget *)button_enable,120,-1);
  g_signal_connect( G_OBJECT( button_enable ), "clicked", G_CALLBACK( cb_execute_install ), data );
  gtk_container_add((GtkContainer *)hbuttonbox,(GtkWidget *)button_enable);
  data->button_enable = button_enable;
  
  gtk_table_attach( GTK_TABLE( table1 ), (GtkWidget *)hbuttonbox, 1, 2, 4, 5, 0, 0, 5, 5 );
  data->message = message;
  data->progress = progress;
  data->hbuttonbox = hbuttonbox;
  data->window = window;
  
  g_free(str);

  gtk_widget_show_all((GtkWidget *)window);

  gtk_main();
}
 
int
main( int argc, char ** argv ) {

  int ret;
  gchar * curpath = g_get_current_dir();

  gchar * procpath = g_malloc(32);
  gint pid = getpid();
  sprintf(procpath, "/proc/%d/exe", pid);

  gchar * processpath = (gchar *) get_readlink(procpath);

  g_free(procpath);

  if ((processpath == 0) || (strlen(processpath) == 0)) {
    g_free(curpath);
    return(0);
  }

  gchar * processpathdir = get_dirname(processpath);
  ret = chdir(processpathdir);

  gchar * maindir = processpathdir;

  gchar * homedir = strdup(getenv("HOME"));
  
  if (getargi("--async",argc,argv) != -1) {
    gchar * strin = g_malloc(100);
    while ((strin == 0) || (strcmp(strin,"ready") != 0)) {
      ret = scanf("%s",strin);
    }
    g_free(strin);
  }

  Data * data = g_malloc(sizeof(Data));
  data->curpath = curpath;
  data->processpath = processpath;
  data->homedir = homedir;
  data->maindir = maindir;

  if (argc > 1) {
    if (strcmp(argv[1],"--install")==0) {
      ret = install_pack(argc,argv,data);
    }
  }
  else {
    ret = gui_status(argc,argv,data);
  }

  g_free(data->curpath);
  g_free(data->homedir);
  g_free(data->maindir);
  g_free(data);

  return(0);

}
