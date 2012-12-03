/*
 * Implements XEP-0184: Message Delivery Receipts
 *
 * For more information about the XEP visit:
 * 		http://xmpp.org/extensions/xep-0184.html
 *
 * Copyright (C) 2012, Ferdinand Stehle
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02111-1301  USA
 *
 */



#define PURPLE_PLUGINS

#define DISPLAY_VERSION "0.3"

#include "gtkplugin.h"
#include "version.h"
#include "prpl.h"
#include "xmlnode.h"

#include <glib.h>
#include <string.h>
#include <stdio.h>

#include "gtkimhtml.h"
#include "gtkutils.h"

#include "util.h"

//#define DEBUG	1

void 		*xmpp_console_handle 	= NULL;

/**
 * \fn display_message_receipt
 * \brief Displays the incoming receipt
 */
void display_message_receipt(PurpleConnection *gc, const char* from, const char* to){

	#ifdef DEBUG
	printf("from: %s to: %s \n", from, to);
	#endif

	PurpleAccount *acct 		= purple_connection_get_account (gc);

	if(!acct) return;

	PurpleConversation *conv 	= purple_find_conversation_with_account(PURPLE_CONV_TYPE_ANY, from, acct);

	if(!conv) return;

	PidginConversation *gtkconv = PIDGIN_CONVERSATION(conv);
	GtkIMHtml *imhtml = GTK_IMHTML(gtkconv->imhtml);

	GtkTextIter location;
	gtk_text_buffer_get_end_iter(imhtml->text_buffer, &location);

	gtk_text_buffer_insert (imhtml->text_buffer,
								&location,
								" ✓",
								-1);

	//purple_conv_im_write(PURPLE_CONV_IM(conv), NULL, "message delivered", PURPLE_MESSAGE_SYSTEM | PURPLE_MESSAGE_NO_LOG, 						time(NULL));
}

/**
 * \fn xmpp_send_string
 * \brief Sends an xmpp-string with the given connection
 */
void
xmpp_send_string(PurpleConnection *gc, char* str)
{
	PurplePluginProtocolInfo *prpl_info = NULL;

	if (gc)
		prpl_info = PURPLE_PLUGIN_PROTOCOL_INFO(gc->prpl);

	//XMPP senden
	if (prpl_info && prpl_info->send_raw != NULL)
		prpl_info->send_raw(gc, str, strlen(str));
}

/**
 * \fn xmlnode_received_cb
 * \brief Reacts to requests and receipts
 */
static void
xmlnode_received_cb(PurpleConnection *gc, xmlnode **packet, gpointer null)
{
	if(*packet != NULL)
	{
		if(strcmp((*packet)->name, "message") == 0)
		{
			#ifdef DEBUG
			printf("got message\n");
			#endif

			xmlnode* nodeRequest = xmlnode_get_child (*packet, "request");

			const char* strFrom	= xmlnode_get_attrib(*packet , "from");
			const char* strTo 	= xmlnode_get_attrib(*packet , "to");
			const char* strId 	= xmlnode_get_attrib(*packet , "id");

			//Answer to an request and verify namespace
			if(nodeRequest)
			{
				#ifdef DEBUG
				printf("got request\n");
				#endif

				const char* strNS = xmlnode_get_namespace(nodeRequest);

				if(strcmp(strNS, "urn:xmpp:receipts") == 0)
				{
					GString*	sendpacket 	= g_string_new("");
					g_string_printf(sendpacket, "<message from='%s' to='%s'>\n<received xmlns='urn:xmpp:receipts' id='%s'/></message>",
					strTo, strFrom, strId);

					#ifdef DEBUG
					printf("\n%s\n", strNS);
					#endif

					xmpp_send_string(gc, sendpacket->str);

					g_string_free(sendpacket, TRUE);
				}
			}
			//Find incoming receipt and call the display-function
			xmlnode* nodeReceived = xmlnode_get_child (*packet, "received");
			if(nodeReceived)
			{
				#ifdef DEBUG
				printf("got received\n");
				#endif

				const char* strNS = xmlnode_get_namespace(nodeReceived);

				if(strcmp(strNS, "urn:xmpp:receipts") == 0)
				{
					display_message_receipt(gc, strFrom, strTo);
				}
			}
		}
	}
}

/*
<message from='noonien@jabber.ccc.de/vaio' id='richard7' to='noonien2@jabber.ccc.de/eee'>
  		<body>My lord, dispatch; read o'er these articles.</body>

  		<request xmlns='urn:xmpp:receipts'/>

</message>
* */


/**
 * \fn xmlnode_sending_cb
 * \brief Extends messages with an receipt request
 */

static void
xmlnode_sending_cb(PurpleConnection *gc, xmlnode **packet, gpointer null)
{
	if(*packet != NULL)
	{
		if((*packet)->name)
		{
			if(strcmp((*packet)->name, "message") == 0)
			{
				//Only for messages containing text (no status messages etc)
				if(xmlnode_get_child (*packet, "body"))
				{
					xmlnode *child	= xmlnode_new_child (*packet, "request");
					xmlnode_set_attrib (child, "xmlns", "urn:xmpp:receipts");
				}
			}
		}
	}
}

static gboolean
plugin_load(PurplePlugin *plugin)
{
	PurplePlugin *jabber = purple_find_prpl("prpl-jabber");

	if (!jabber)
		return FALSE;

	xmpp_console_handle = plugin;


	purple_signal_connect(jabber, "jabber-receiving-xmlnode", xmpp_console_handle,
			    PURPLE_CALLBACK(xmlnode_received_cb), NULL);
	purple_signal_connect(jabber, "jabber-sending-xmlnode", xmpp_console_handle,
			    PURPLE_CALLBACK(xmlnode_sending_cb), NULL);

	return TRUE;
}

static gboolean
plugin_unload(PurplePlugin *plugin)
{



	return TRUE;
}

static GList dep = {"prpl-jabber",NULL,NULL};

static PurplePluginInfo info =
{
	PURPLE_PLUGIN_MAGIC,
	PURPLE_MAJOR_VERSION,
	PURPLE_MINOR_VERSION,
	PURPLE_PLUGIN_STANDARD,                       	/**< type           */
	PIDGIN_PLUGIN_TYPE,                           	/**< ui_requirement */
	0,                                            	/**< flags          */
	&dep,                                         	/**< dependencies   */
	PURPLE_PRIORITY_LOWEST,                      	/**< priority       */

	"gtk-xmpp-receipts",                            /**< id             */
	"XMPP Receipts",                         		/**< name           */
	DISPLAY_VERSION,                              	/**< version        */
													/**  summary        */
	"Send and receive xmpp receipts.",
													/**  description    */
	"Implements XEP-0184: Message Delivery Receipts within pidgin.",
	"Noonien (fsmail@kondorgulasch.de)",          	/**< author         */
	"http//pidgin.im",                            	/**< homepage       */

	plugin_load,                                  	/**< load           */
	plugin_unload,                                	/**< unload         */
	NULL,                                         	/**< destroy        */

	NULL,                                         	/**< ui_info        */
	NULL,                                         	/**< extra_info     */
	NULL,
	NULL,

	/* padding */
	NULL,
	NULL,
	NULL,
	NULL
};

static void
init_plugin(PurplePlugin *plugin)
{


}

PURPLE_INIT_PLUGIN(xmppconsole, init_plugin, info)
