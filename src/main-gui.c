#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <gtk/gtk.h>
#include <cwallet.h>

/* The data structure that contains information about graphical events */
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
  GtkEntry * entryprivkey;
  GtkEntry * entrypassphrase;

  gchar * action;
  gchar * curpath;
  gchar * targetname;
  gchar * homedir;

  gchar * processpath;
  gchar * maindir;

  gchar ** argv;
  gint argc;
 
  gint timeout_id;
  gint timeout_id2;

  gchar * outdir;
  GtkWidget * button_decrypt;

};

static void cb_execute_install(GtkButton *button, Data *data);
static void cb_execute_reinstall(GtkButton *button, Data *data);
static void cb_execute_remove(GtkButton *button, Data *data);
static void cb_execute_update(GtkButton *button, Data *data);

/* Get the substring of str starting at index begin and with length len
   str: The string to get the substring of
   begin: The starting index
   len: The length of the substring
   Return a newly allocated copy of the substring, or 0 if the substring is improperly defined */
gchar* substring(const gchar* str, size_t begin, size_t len) {
  if (str == 0 || strlen(str) == 0 || strlen(str) < begin || strlen(str) < (begin+len))
    return 0;

  return strndup(str + begin, len);
}

/* Get the absolute path of the given process path, by following all links via readlink()
   procpath: The given process path
   Return the absolute path */
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

/* Get the directory containing the given file.
   path: The path of the given file
   Return the name of the directory */
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

/* Get the index of the argument that is equal (as a string) to target.
   target: The argument being searched for
   argc: The number of arguments
   argv: The array of arguments to search in
   Return the index of the wanted argument, or -1 if it does not exist. */
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

/* Clean any remnants of the child process */
static void cb_child_watch(GPid  pid, gint  status, Data * data) {
    g_spawn_close_pid(pid);
}
 
/* Read from stderr
   Return TRUE if successful */
static gboolean cb_err_watch(GIOChannel * channel, GIOCondition  cond, Data * data) {
    gchar *string;
    gsize  size;
    if (cond == G_IO_HUP) {
      g_io_channel_unref(channel);
      return(FALSE);
    }
    g_io_channel_read_line(channel,&string,&size, 0, 0);
    if (string!=0) {
      g_free(string);
    }
    return(TRUE);
}

/* Read from stdout and change the displayed message and button state accordingly
   Return TRUE if successful */
static gboolean cb_out_watch(GIOChannel * channel, GIOCondition  cond, Data * data) {

    gchar *string;
    gsize  size;
 
    if(cond == G_IO_HUP) {
        g_io_channel_unref(channel);
        return(FALSE);
    }
 
    g_io_channel_read_line(channel, &string, &size, 0, 0);
    int len = strlen(string);
    if (*(string+len-1)=='\n') {
      *(string+len-1) = '\0';
    }

    if (strcmp(data->action,"install")==0) {
      gchar * submessage = substring(string,0,5);
      if (g_strcmp0("Saved",submessage) == 0) {

	g_source_remove(data->timeout_id);
	g_source_remove(data->timeout_id2);
	gtk_progress_bar_set_fraction((GtkProgressBar *)data->progress, 0.0);
	g_free(data->action);

	gtk_label_set_label(data->message, string);
	fprintf(stdout,"%s\n",string);

	gtk_widget_set_sensitive((GtkWidget *)data->button_enable,TRUE);
	gtk_widget_grab_focus((GtkWidget *)data->button_enable);

      }
      else if (g_strcmp0("Error",submessage) == 0) {

        g_source_remove(data->timeout_id);
	g_source_remove(data->timeout_id2);
        gtk_progress_bar_set_fraction((GtkProgressBar *)data->progress, 0.0);
	g_free(data->action);

	int tab_index = indexof('\t',string);
	if (tab_index!=-1) {
	  string[tab_index] = '\n';
	}

	gtk_label_set_label(data->message, string);
	fprintf(stderr,"%s\n",string);

	gtk_widget_set_sensitive((GtkWidget *)data->button_enable,TRUE);

      }
      g_free(submessage);
    }
    return(TRUE);
}

/* Move the progess bar once the corresponding timeout event occurs */
static gboolean cb_timeout(Data *data) {
    gtk_progress_bar_pulse(data->progress);
    return(TRUE);
}

/* Ask for more entropy once the corresponding timeout event occurs */
static gboolean cb_timeout2(Data *data) {
  gtk_label_set_label(data->message, "Shake your mouse for more entropy");
  return(FALSE);
}

/* Perform the action that was set in the action variable */
static void cb_execute(GtkButton * button, Data * data) {

    GPid pid;
    gchar ** argv;
    gint argc = -1;

    if (strcmp(data->action,"install") == 0) {
      argc = 4;
      argv = malloc(sizeof(gchar*) * 11);
      argv[0] = strdup(data->processpath);
      argv[1] = strdup("--install");
      argv[2] = strdup("--async");
      const gchar * outdir = gtk_entry_get_text(data->entryoutdir);
      if (strlen(outdir)<1) {
	gtk_label_set_label(data->message,"Error: The output directory is mandatory\n");
	return;
      }
      if (indexof('\'',outdir)!=-1) {
	gtk_label_set_label(data->message,"Error: The output directory has unsupported characters\n");
	return;
      }
      argv[3] = strdup("-d");
      argv[4] = g_malloc(strlen(outdir)+1);
      strcpy(argv[4],outdir);
      const gchar * privkey = gtk_entry_get_text(data->entryprivkey);
      if (strcmp(privkey,"")!=0) {
	argc++;
	argv[argc] = strdup("-k");
	argc++;
	argv[argc] = g_malloc(strlen(privkey)+1);
	strcpy(argv[argc],privkey);
      }
      const gchar * passphrase = gtk_entry_get_text(data->entrypassphrase);
      if (strcmp(passphrase,"")!=0) {
	argc++;
	argv[argc] = strdup("-p");
	argc++;
	argv[argc] = g_malloc(strlen(passphrase)+1);
	strcpy(argv[argc],passphrase);
      }
      if (gtk_toggle_button_get_active((GtkToggleButton *)(data->button_decrypt))==FALSE) {
	argc++;
	argv[argc] = strdup("-e");
      }
      argc++;
      argv[argc] = 0;
      gtk_widget_set_sensitive((GtkWidget *)data->button_enable,FALSE);
      gtk_label_set_label(data->message, "Generating");
      data->outdir = strdup(outdir);
    }

    gint in, out, err;
    GIOChannel *in_ch, *out_ch, *err_ch;
    gboolean ret;
 
    ret = g_spawn_async_with_pipes(0, argv, 0, G_SPAWN_DO_NOT_REAP_CHILD, 0, 0, &pid, &in, &out, &err, 0);
    if(! ret) {
      g_error("SPAWN FAILED");
      return;
    }

    /* Add watch function to catch termination of the process. This function will clean any remnants of process. */
    g_child_watch_add(pid, (GChildWatchFunc)cb_child_watch, data);
 
    /* Create channels that will be used to read data from pipes. */
#ifdef G_OS_WIN32
    in_ch = g_io_channel_win32_new_fd(in);
    out_ch = g_io_channel_win32_new_fd(out);
    err_ch = g_io_channel_win32_new_fd(err);
#else
    in_ch = g_io_channel_unix_new(in);
    out_ch = g_io_channel_unix_new(out);
    err_ch = g_io_channel_unix_new(err);
#endif
 
    /* Add watches to channels */
    g_io_add_watch(out_ch, G_IO_IN | G_IO_HUP, (GIOFunc)cb_out_watch, data);
    g_io_add_watch(err_ch, G_IO_IN | G_IO_HUP, (GIOFunc)cb_err_watch, data);

    /* Install timeout function that will move the progress bar */
    data->timeout_id = g_timeout_add(100, (GSourceFunc)cb_timeout, data);
    data->timeout_id2 = g_timeout_add_seconds (3,(GSourceFunc)cb_timeout2,data);

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

/* Set action to install and call for it to execute, once the corresponding event occurs */
static void cb_execute_install(GtkButton *button,
				Data      *data)
{
 
  data->action = strdup("install");
  cb_execute(button,data);

}

/* Perform the install action, i.e. call the qrencode function with the given parameters.
   Return 0 on success */
int install_pack(int argc, char **argv, Data * data) {

  int ret;

  gchar * maindir = data->maindir;
  gchar * curpath = g_get_current_dir();
  gchar * homedir = data->homedir;

  gchar * outdir = 0;
  gchar * privkey = 0;
  gchar * passphrase = 0;
  gint arg_d = getargi("-d", argc, argv);
  if (arg_d!=-1) {
    outdir = strdup(argv[arg_d+1]);
  }
  else {
    fprintf(stdout,"Error: No output directory given\n");
    fflush(stdout);
    return(1);
  }
  gint arg_k = getargi("-k", argc, argv);
  if (arg_k!=-1) {
    privkey = strdup(argv[arg_k+1]);
  }
  gint arg_p = getargi("-p", argc, argv);
  if(arg_p!=-1) {
    passphrase = strdup(argv[arg_p+1]);
  }
  unsigned char eflag = 0;
  gint arg_e = getargi("-e", argc, argv);
  if(arg_e!=-1) {
    eflag = 1;
  }

  gchar * str = g_malloc(strlen(maindir)+strlen(outdir)+200);

  unsigned char rflag = 0;

  if (privkey == 0) {
    rflag = 1;
  }

  char ** generate_result = (char **)malloc(sizeof(char *)*2);
  ret = generate_key(1, rflag, eflag, 0, outdir, 0, 0, passphrase, privkey, generate_result);
  if (ret==0) {
    char * address = generate_result[0];
    fprintf(stdout,"Saved to %s.pdf\n",address);
    fflush(stdout);
  }
  else {
    int nl_index = indexof('\n',generate_result[0]);
    if (nl_index!=-1) {
      if (strlen(generate_result[0])!=(nl_index+1)){
	generate_result[0][nl_index] = '\t';
      }
    }
    fprintf(stdout,"Error: %s",generate_result[0]);
    fflush(stdout);
  }
  g_free(generate_result[0]);

  if (outdir!=0){
    g_free(outdir);
  }
  if (privkey!=0) {
    g_free(privkey);
  }
  if (passphrase!=0){
    g_free(passphrase);
  }
  g_free(curpath);
  g_free(str);

  return(0);
}

/* Show the GUI.
   Return 0 on success */
int gui_status(int argc, char ** argv, Data * data) {

  GtkWindow * window;
  GtkTable * table1, * table2, * table3;
  GtkButton * button, * button_disable, * button_enable, * button_update;
  GtkProgressBar * progress;
  GtkLabel * message = 0;
  GtkLabel * labelwalletfile = 0;
  GtkLabel * labeladdress = 0;
  GtkLabel * labeloutdir = 0;
  GtkLabel * labelprivkey = 0;
  GtkLabel * labelpassphrase = 0;
  GtkImage * logo;
  GtkHButtonBox * hbuttonbox;
  GtkEntry * entrywalletfile = 0;
  GtkEntry * entryaddress = 0;
  GtkEntry * entryoutdir = 0;
  GtkEntry * entryprivkey = 0;
  GtkEntry * entrypassphrase = 0;
  GtkWidget * button_radio = 0;
  GSList * group_radio = 0;

  gtk_init(&argc, &argv);

  gchar * maindir = data->maindir;

  gchar * homedir = data->homedir;

  gchar * curpath = data->curpath;

  gchar * str = g_malloc(strlen(maindir)+50);

  window = (GtkWindow *)gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
  gtk_window_set_default_size(GTK_WINDOW(window), 500, 400);
  gtk_window_set_title(GTK_WINDOW(window), "Cwallet");

  strcpy(str,maindir);
  strcat(str,"/icon.png");

  gtk_window_set_icon_from_file(GTK_WINDOW(window), str, 0);
  
  g_signal_connect(G_OBJECT(window), "destroy", G_CALLBACK(gtk_main_quit), 0);
  
  table1 = (GtkTable *)gtk_table_new(4, 5, FALSE);
  gtk_container_add((GtkContainer *)window, (GtkWidget *)table1);
  
  table2 = (GtkTable *)gtk_table_new(3, 3, FALSE);
  gtk_table_attach(GTK_TABLE(table1), (GtkWidget *)table2, 1, 2, 0, 1, GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);

  strcpy(str,maindir);
  strcat(str,"/logo.png");

  logo = (GtkImage *)gtk_image_new_from_file(str);
  gtk_table_attach(GTK_TABLE(table2), (GtkWidget *)logo, 1, 2, 1, 2, GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 5, 5);

  message = (GtkLabel *)gtk_label_new("Generate a PDF for your paper wallet.\nAll fields are optional except the output directory.\n");
  gtk_label_set_justify((GtkLabel *)message,GTK_JUSTIFY_CENTER);
  gtk_widget_set_size_request((GtkWidget *)message,-1,60);
  gtk_misc_set_alignment((GtkMisc *)message,0.5,1);
  gtk_table_attach(GTK_TABLE(table1), (GtkWidget *)message, 1, 2, 1, 2, GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 5, 5);
  gtk_label_set_line_wrap ((GtkLabel *)message, TRUE);

  table3 = (GtkTable *)gtk_table_new(3, 2, FALSE);
  gtk_table_set_row_spacings(table3,0.5);
  gtk_table_attach(GTK_TABLE(table1), (GtkWidget *)table3, 1, 2, 2, 3, GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 5, 5);

  labeloutdir = (GtkLabel *)gtk_label_new("Output directory:");
  gtk_misc_set_alignment((GtkMisc *)labeloutdir,0.,0.5);
  gtk_table_attach(GTK_TABLE(table3), (GtkWidget *)labeloutdir, 0, 1, 0, 1, GTK_FILL, 0, 5, 5);

  entryoutdir = (GtkEntry *)gtk_entry_new();
  gtk_entry_set_text(entryoutdir,homedir);
  gtk_entry_append_text(entryoutdir,"/cwallet");
  gtk_table_attach(GTK_TABLE(table3), (GtkWidget *)entryoutdir, 1, 2, 0, 1, GTK_EXPAND | GTK_FILL, 0, 5, 5);
  data->entryoutdir = entryoutdir;

  labelprivkey = (GtkLabel *)gtk_label_new("Private key:");
  gtk_misc_set_alignment((GtkMisc *)labelprivkey,0,0);
  gtk_table_attach(GTK_TABLE(table3), (GtkWidget *)labelprivkey, 0, 1, 1, 2, GTK_FILL, 0, 5, 5);

  entryprivkey = (GtkEntry *)gtk_entry_new();
  gtk_table_attach(GTK_TABLE(table3), (GtkWidget *)entryprivkey, 1, 2, 1, 2, GTK_EXPAND | GTK_FILL, 0, 5, 5);
  data->entryprivkey = entryprivkey;

  labelpassphrase = (GtkLabel *)gtk_label_new("Passphrase:");
  gtk_misc_set_alignment((GtkMisc *)labelpassphrase,0,0);
  gtk_table_attach(GTK_TABLE(table3), (GtkWidget *)labelpassphrase, 0, 1, 2, 3, GTK_FILL, 0, 5, 5);

  entrypassphrase = (GtkEntry *)gtk_entry_new();
  gtk_table_attach(GTK_TABLE(table3), (GtkWidget *)entrypassphrase, 1, 2, 2, 3, GTK_EXPAND | GTK_FILL, 0, 5, 5);
  data->entrypassphrase = entrypassphrase;

  button_radio = gtk_radio_button_new_with_label (0, "encrypt");
  gtk_toggle_button_set_active((GtkToggleButton *)(button_radio), TRUE);
  gtk_table_attach(GTK_TABLE(table3), (GtkWidget *)button_radio, 0, 1, 3, 4, GTK_FILL, 0, 5, 5);
  group_radio = gtk_radio_button_group (GTK_RADIO_BUTTON (button_radio));
  button_radio = gtk_radio_button_new_with_label(group_radio, "decrypt");
  gtk_table_attach(GTK_TABLE(table3), (GtkWidget *)button_radio, 1, 2, 3, 4, GTK_EXPAND | GTK_FILL, 0, 5, 5);
  data->button_decrypt = button_radio;

  progress = (GtkProgressBar *)gtk_progress_bar_new();
  gtk_table_attach(GTK_TABLE(table1),(GtkWidget *)progress, 1, 2, 3, 4, GTK_FILL, GTK_SHRINK | GTK_FILL, 5, 0);
  hbuttonbox = (GtkHButtonBox *)gtk_hbutton_box_new();
  
  button_enable = (GtkButton *)gtk_button_new_with_label("Generate");
  gtk_widget_set_size_request((GtkWidget *)button_enable,120,-1);
  g_signal_connect(G_OBJECT(button_enable), "clicked", G_CALLBACK(cb_execute_install), data);
  gtk_container_add((GtkContainer *)hbuttonbox,(GtkWidget *)button_enable);
  data->button_enable = button_enable;
  
  gtk_table_attach(GTK_TABLE(table1), (GtkWidget *)hbuttonbox, 1, 2, 4, 5, 0, 0, 5, 5);
  data->message = message;
  data->progress = progress;
  data->hbuttonbox = hbuttonbox;
  data->window = window;
  
  g_free(str);

  gtk_widget_show_all((GtkWidget *)window);

  gtk_widget_grab_focus((GtkWidget *)button_enable);

  gtk_main();
  return(0);
}

/* Show the GUI or perform the install (qrencode) action 
   Return 0 on success */
int main(int argc, char ** argv) {

  int ret = 0;

  int addpathi = getargi("--addpath",argc,argv);
  if (addpathi != -1) {
    char * addpath = argv[addpathi+1];
    char * oldpath = getenv("PATH");
    char * newpath = g_malloc(strlen(addpath)+strlen(oldpath)+10);
    strcpy(newpath,addpath);
    strcat(newpath,":");
    strcat(newpath,oldpath);
    ret = setenv("PATH",newpath,1);
    g_free(newpath);
  }

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
    else {
      ret = gui_status(argc,argv,data);
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
