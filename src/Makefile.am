bin_PROGRAMS = cwallet cwallet-gui
cwallet_SOURCES = cwallet.c
cwallet_LDFLAGS = -Wl,-Bstatic -ldb-4.8 -lcrypto -Wl,-Bdynamic
cwallet_gui_SOURCES = cwallet-gui.c
cwallet_gui_LDADD = $(GTK2_LIBS)
AM_CPPFLAGS = $(GTK2_CFLAGS) -I$(CURDIR)