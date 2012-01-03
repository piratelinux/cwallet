bin_PROGRAMS = cwallet cwallet-gui
cwallet_SOURCES = cwallet.c
cwallet_gui_SOURCES = cwallet-gui.c
cwallet_LDADD = -ldb-4.8 -lcrypto
cwallet_gui_LDADD = $(GTK2_LIBS)
AM_CPPFLAGS = $(GTK2_CFLAGS) -I$(CURDIR)