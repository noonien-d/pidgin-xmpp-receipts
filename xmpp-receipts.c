/*
 * Implements XEP-0184: Message Delivery Receipts
 *
 * For more information about the XEP visit:
 * 		http://xmpp.org/extensions/xep-0184.html
 *
 * Copyright (C) 2012, Ferdinand Stehle, development[at]kondorgulasch.de
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

#define DISPLAY_VERSION "0.8"

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

typedef struct
{
	GtkTextBuffer	*textbuffer;
	gint			offset;
	gint			lines;
}message_info;

void 		*xmpp_console_handle 	= NULL;
static GHashTable *ht_locations 	= NULL;

/**
 * \fn add_message_iter
 * \brief Stores the current position in the chat window
 */
void add_message_iter(PurpleConnection *gc, const char* to, const gchar* messageid, int newlines)
{
	#ifdef DEBUG
	printf("to: %s \n", to);
	#endif

	PurpleAccount *acct 		= purple_connection_get_account (gc);

	if(!acct) return;

	PurpleConversation *conv 	= purple_find_conversation_with_account(PURPLE_CONV_TYPE_ANY, to, acct);

	if(!conv) return;

	PidginConversation *gtkconv = PIDGIN_CONVERSATION(conv);
	GtkIMHtml *imhtml = GTK_IMHTML(gtkconv->imhtml);

	message_info *info 	= g_new(message_info, 1);

	info->textbuffer	= imhtml->text_buffer;
	GtkTextIter		location;
	gtk_text_buffer_get_end_iter(imhtml->text_buffer, &location);

	info->offset		= gtk_text_iter_get_offset (&location);
	info->lines			= newlines;

	//Insert the location to the table, use messageid as key
	g_hash_table_insert(ht_locations, strdup(messageid), info);

	#ifdef DEBUG
	printf("attached key: %s, table size now %d \n", messageid, g_hash_table_size(ht_locations));
	#endif

}

/**
 * \fn display_message_receipt
 * \brief Displays the received receipt at the correct position
 */
void display_message_receipt(const char* strId){

	#ifdef DEBUG
	if(strId == NULL)
		printf("NO ID\n");
	else
		printf("id is: %s \n", strId);
	#endif

	if(strId == NULL)
		return;

	message_info* info = (message_info*) g_hash_table_lookup(ht_locations, strId);

	#ifdef DEBUG
	printf("insert...");
	#endif

	if(info != NULL)
	{
		GtkTextIter		location;
		gtk_text_buffer_get_iter_at_offset (info->textbuffer,
											&location,
											info->offset);
		gtk_text_iter_forward_lines (&location, info->lines);
		gtk_text_iter_forward_to_line_end (&location);

		gtk_text_buffer_insert (info->textbuffer,
							&location,
							" ✓",
							-1);

		g_hash_table_remove (ht_locations, strId);
	}
	#ifdef DEBUG
	else
		printf("failed: id not found\n");
	#endif
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

			//Answer to an request and verify namespace
			if(nodeRequest)
			{
				#ifdef DEBUG
				printf("got request\n");
				#endif

				const char* strId 	= xmlnode_get_attrib(*packet , "id");

				const char* strNS = xmlnode_get_namespace(nodeRequest);

				if(strcmp(strNS, "urn:xmpp:receipts") == 0)
				{
					xmlnode *message = xmlnode_new("message");
					xmlnode_set_attrib(message, "to", strFrom);
					
					xmlnode *received = xmlnode_new_child(message, "received");
					xmlnode_set_namespace(received, "urn:xmpp:receipts");
					
					xmlnode_set_attrib(received, "id", strId);

					purple_signal_emit(purple_connection_get_prpl(gc), "jabber-sending-xmlnode", gc, &message);
					
					if (message != NULL)
						xmlnode_free(message);
				}
			}
			//Find incoming receipt and call the display-function
			xmlnode* nodeReceived = xmlnode_get_child (*packet, "received");
			if(nodeReceived)
			{
				#ifdef DEBUG
				printf("got received\n");
				#endif

				const char* strNS 	= xmlnode_get_namespace(nodeReceived);
				const char* strId 	= xmlnode_get_attrib(nodeReceived , "id");

				if (strcmp(strNS, "urn:xmpp:receipts") == 0)
				{
					display_message_receipt(strId);
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
 * \fn deleting_conversation_remove_items
 * \brief Checks if the entry belongs to a given textbuffer
 * \param user_data a pointer to the searched textbuffer.
 */
static gboolean
deleting_conversation_remove_items(gpointer key, gpointer value,
									gpointer user_data)
{
	return (((message_info*)value)->textbuffer == user_data) ? TRUE : FALSE;
}

/**
 * \fn deleting_conversation_cb
 * \brief Deletes all references to this conversation from the hashtable
 */
static void
deleting_conversation_cb(PurpleConversation *conv)
{
	g_hash_table_foreach_remove(ht_locations,
								deleting_conversation_remove_items,
								GTK_IMHTML(PIDGIN_CONVERSATION(conv)->imhtml)->text_buffer);
            
	#ifdef DEBUG
	printf("conversation closed, table size now %d \n", g_hash_table_size(ht_locations));
	#endif
}

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
				xmlnode *nodeBody	= xmlnode_get_child (*packet, "body");

				int newlines 	= 1;

				//Only for messages containing text (no status messages etc)
				if(nodeBody)
				{
					char*	text = xmlnode_get_data(nodeBody);
					if(text)
					{
						int	sz			= strlen(text);
						int i;
						for (i = 0; i < sz; i++) {
							if (text[i] == '\n')
								newlines++;
						}

						#ifdef DEBUG
						printf("lines: %d\n", newlines);
						#endif

						g_free(text);
					}
					xmlnode *child	= xmlnode_new_child (*packet, "request");
					xmlnode_set_attrib (child, "xmlns", "urn:xmpp:receipts");

					const char* strTo 	= xmlnode_get_attrib(*packet , "to");
					const char* strId 	= xmlnode_get_attrib(*packet , "id");

					add_message_iter(gc, strTo, strId, newlines);
				}
			}
		}
	}
}

void free_message_info(gpointer data)
{
	g_free(data);
}

static gboolean
plugin_load(PurplePlugin *plugin)
{
	PurplePlugin *jabber = purple_find_prpl("prpl-jabber");

	if (!jabber)
		return FALSE;
	
	//Publish support via caps
	gboolean ok;
	purple_plugin_ipc_call (jabber, "add_feature", &ok, "urn:xmpp:receipts");

	#ifdef DEBUG
	if (ok)
		printf("receipt feature added\n");
	else
		printf("receipt feature not added (will work anyway with most clients)\n");
	#endif

	xmpp_console_handle = plugin;

	ht_locations	= g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);

	purple_signal_connect(jabber, "jabber-receiving-xmlnode", xmpp_console_handle,
			    PURPLE_CALLBACK(xmlnode_received_cb), NULL);
	purple_signal_connect_priority(jabber, "jabber-sending-xmlnode", xmpp_console_handle,
			    PURPLE_CALLBACK(xmlnode_sending_cb), NULL, -101);


    //Connect signals for conversations to clean references
    void *conv_handle = purple_conversations_get_handle();

    purple_signal_connect(conv_handle, "deleting-conversation",
							plugin,
							PURPLE_CALLBACK(deleting_conversation_cb),
							NULL);

	return TRUE;
}

static gboolean
plugin_unload(PurplePlugin *plugin)
{
	g_hash_table_destroy(ht_locations);

	return TRUE;
}

static PurplePluginInfo info =
{
	PURPLE_PLUGIN_MAGIC,
	PURPLE_MAJOR_VERSION,
	PURPLE_MINOR_VERSION,
	PURPLE_PLUGIN_STANDARD,                       	/**< type           */
	PIDGIN_PLUGIN_TYPE,                           	/**< ui_requirement */
	0,                                            	/**< flags          */
	NULL,                                         	/**< dependencies   */
	PURPLE_PRIORITY_LOWEST,                      	/**< priority       */

	"gtk-xmpp-receipts",                            /**< id             */
	"XMPP Receipts",                         		/**< name           */
	DISPLAY_VERSION,                              	/**< version        */
													/**  summary        */
	"Send and receive xmpp receipts.",
													/**  description    */
	"Implements XEP-0184: Message Delivery Receipts within pidgin.",
	"Ferdinand Stehle (development@kondorgulasch.de)",          	/**< author         */
	"https://github.com/noonien-d/pidgin-xmpp-receipts/",     /**< homepage       */

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
plugin->info->dependencies = g_list_append(plugin->info->dependencies, "prpl-jabber");
}

PURPLE_INIT_PLUGIN(xmppconsole, init_plugin, info)
