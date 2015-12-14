GTK_PIDGIN_INCLUDES= `pkg-config --cflags gtk+-2.0 pidgin`

CC ?= gcc
CFLAGS += -O2 -Wall -fpic
LDFLAGS += -shared
DESTDIR =
PLUGINDIR = ~/.purple/plugins/

INCLUDES = \
      $(GTK_PIDGIN_INCLUDES)

xmpp-receipts.so: xmpp-receipts.c
	$(CC) xmpp-receipts.c $(CFLAGS) $(INCLUDES) $(LDFLAGS) -o xmpp-receipts.so

install: xmpp-receipts.so
	install -Dm755 xmpp-receipts.so -t $(DESTDIR)$(PLUGINDIR)

uninstall:
	rm -f $(DESTDIR)$(PLUGINDIR)/xmpp-receipts.so

clean:
	rm -f xmpp-receipts.so
