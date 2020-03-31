pidgin-xmpp-receipts
====================


This pidgin-plugin implements xmpp message delivery receipts ([XEP-0184 v1.2](https://xmpp.org/extensions/xep-0184.html)).


When no delivering confirmation is displayed, it is also possible that the 
receiver doesn't support the extension.


Compiling
---------

To compile the plugin, run

>	$ make

You will need pidgin development packages
(link in ubuntu: libpurple-dev and pidgin-dev).


Installation
------------

To copy the extension to your personal plugin folder (~/.purple/plugins)
run:

>	$ make install

Now you may activate the extension within the pidgin settings.
