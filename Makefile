GTK_PIDGIN_INCLUDES= `pkg-config --cflags gtk+-2.0 pidgin`

CFLAGS= -O2 -Wall -fpic -g
LDFLAGS= -shared

INCLUDES = \
      $(GTK_PIDGIN_INCLUDES)

xmpp-receipts.so: xmpp-receipts.c
	gcc xmpp-receipts.c $(CFLAGS) $(INCLUDES) $(LDFLAGS) -o xmpp-receipts.so

install: xmpp-receipts.so
	mkdir -p ~/.purple/plugins
	cp xmpp-receipts.so ~/.purple/plugins/

uninstall:
	rm -f ~/.purple/plugins/xmpp-receipts.so

clean:
	rm -f xmpp-receipts.so
