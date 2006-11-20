/*
 *  Copyright (c) 2006 Stephan Arts <psyBSD@gmail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include <string.h>
#include <glib.h>
#include <glib/gstdio.h>
#include <glib-object.h>

#include "mime.h"
#include "archive.h"
#include "archive-support.h"

#include "internals.h"

void
lxa_archive_support_init(LXAArchiveSupport *support);
void
lxa_archive_support_class_init(LXAArchiveSupportClass *supportclass);

/*
 *
 */
GType
lxa_archive_support_get_type ()
{
	static GType lxa_archive_support_type = 0;

 	if (!lxa_archive_support_type)
	{
 		static const GTypeInfo lxa_archive_support_info = 
		{
			sizeof (LXAArchiveSupportClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) lxa_archive_support_class_init,
			(GClassFinalizeFunc) NULL,
			NULL,
			sizeof (LXAArchiveSupport),
			0,
			(GInstanceInitFunc) lxa_archive_support_init,
		};

		lxa_archive_support_type = g_type_register_static (G_TYPE_OBJECT, "LXAArchiveSupport", &lxa_archive_support_info, 0);
	}
	return lxa_archive_support_type;
}

/*
 *
 */
void
lxa_archive_support_init(LXAArchiveSupport *support)
{
	support->add = NULL;
	support->extract = NULL;
	support->remove = NULL;
	support->refresh = NULL;
	support->user_action = NULL;
}

/*
 *
 */
void
lxa_archive_support_class_init(LXAArchiveSupportClass *supportclass)
{
	/*
	GObjectClass *gobject_class = G_OBJECT_CLASS (supportclass);
	LXAArchiveSupportClass *klass = LXA_ARCHIVE_SUPPORT_CLASS (supportclass);
	*/
}

/*
 *
 */
LXAArchiveSupport*
lxa_archive_support_new()
{
	LXAArchiveSupport*support;

	support = g_object_new(LXA_TYPE_ARCHIVE_SUPPORT, NULL);
	
	return support;
}

/*
 *
 */
void
lxa_archive_support_add_mime(LXAArchiveSupport *support, gchar *mime)
{
	support->mime = g_slist_prepend(support->mime, mime);	
}

/*
 *
 */
gboolean
lxa_archive_support_mime_supported(LXAArchiveSupport *support, const gchar *mime)
{
	GSList *result = g_slist_find_custom(support->mime, mime, lxa_archive_support_lookup_mime);
	if(!result)
		return FALSE;
	if(!result->data)
		return FALSE;
	return TRUE;
}

/*
 *
 */
gboolean
lxa_register_support(LXAArchiveSupport *support)
{ 
	if(!LXA_IS_ARCHIVE_SUPPORT(support))
		return FALSE;

	lxa_archive_support_list = g_slist_prepend(lxa_archive_support_list, support);	
	g_object_ref(support);

	return TRUE;
}

/*
 *
 */
LXAArchiveSupport *
lxa_get_support_for_mime(const gchar *mime)
{
	return lxa_get_support_for_mime_from_slist(lxa_archive_support_list, mime);
}

/*
 *
 */
LXAArchiveSupport *
lxa_get_support_for_mime_from_slist(GSList *list, const gchar *mime)
{
	GSList *result = g_slist_find_custom(list, mime, lxa_archive_support_lookup_support);
	if(result)
		return result->data;
	return NULL;
}

/*
 *
 */
gint
lxa_archive_support_lookup_mime(gconstpointer support_mime, gconstpointer mime)
{
	return strcmp((gchar *)support_mime, (gchar *)mime);
}

/*
 *
 */
gint
lxa_archive_support_lookup_support(gconstpointer support, gconstpointer mime)
{
	if(lxa_archive_support_mime_supported(LXA_ARCHIVE_SUPPORT(support), (gchar *)mime))
		return 0;
	else
		return 1;
}		

gint
lxa_archive_support_add(LXAArchiveSupport *support, LXAArchive *archive, GSList *files)
{
	if(support->add)
	{
		lxa_archive_set_status(archive, LXA_ARCHIVESTATUS_ADD);
		archive->support = support;
		return support->add(archive, files);
	}
	else
		g_critical("ADD NOT IMPLEMENTED BY SUPPORT OBJECT '%s'", support->id);
	return -1;
}

gint
lxa_archive_support_extract(LXAArchiveSupport *support, LXAArchive *archive, gchar *dest_path, GSList *files)
{
	if(support->extract)
	{
		lxa_archive_set_status(archive, LXA_ARCHIVESTATUS_EXTRACT);
		archive->support = support;
		return support->extract(archive, dest_path, files);
	}
	else
		g_critical("EXTRACT NOT IMPLEMENTED BY SUPPORT OBJECT '%s'", support->id);
	return -1;
}

gint
lxa_archive_support_remove(LXAArchiveSupport *support, LXAArchive *archive, GSList *files)
{
	if(support->remove)
	{
		lxa_archive_set_status(archive, LXA_ARCHIVESTATUS_REMOVE);
		archive->support = support;
		return support->remove(archive, files);
	}
	else
		g_critical("REMOVE NOT IMPLEMENTED BY SUPPORT OBJECT '%s'", support->id);
	return -1;
}

gint
lxa_archive_support_refresh(LXAArchiveSupport *support, LXAArchive *archive)
{
	if(support->refresh)
	{
		lxa_archive_set_status(archive, LXA_ARCHIVESTATUS_REFRESH);
		archive->support = support;
		return support->refresh(archive);
	}
	else
		g_critical("VIEW NOT IMPLEMENTED BY SUPPORT OBJECT '%s'", support->id);
	return -1;
}

GSList *
lxa_archive_support_list_properties(LXAArchiveSupport *support, gchar *prefix)
{
	guint _nprops, i;
	GObjectClass *support_class = G_OBJECT_GET_CLASS(support);
	GParamSpec **pspecs = g_object_class_list_properties(support_class, &_nprops);
	GSList *pspec_list = NULL;

/* Reverse the array, because the list gets prepended
 * Otherwise the option-list gets reversed
 */
	for(i = _nprops; i > 0; i--)
	{
		if(!strncmp(prefix, g_param_spec_get_name(pspecs[i-1]), strlen(prefix)))
		{
			pspec_list = g_slist_prepend(pspec_list, pspecs[i-1]);
		}
	}
	return pspec_list;
}

void
lxa_archive_support_install_action(LXAArchiveSupport *support, LXAUserAction *action)
{
	support->user_action = g_slist_append(support->user_action, action);
}

LXAUserAction*
lxa_archive_support_find_action(LXAArchiveSupport *support, const gchar *name)
{
	GSList *actions = support->user_action;
	while(actions)
	{
		if(strcmp(((LXAUserAction*)actions->data)->name, name) == 0)
			return (LXAUserAction*)actions->data;
		actions = actions->next;
	}
	return NULL;
}

LXAUserAction**
lxa_archive_support_list_actions(LXAArchiveSupport *support, guint *n_actions)
{
	LXAUserAction** list;
	guint i = 0;
	GSList *actions = support->user_action;
	(*n_actions) = g_slist_length(actions);
	list = g_new(LXAUserAction*, *n_actions);
	while(actions)
	{
		list[i++] = (LXAUserAction*)actions->data;
		actions = actions->next;
	}
	return list;
}

LXAUserAction*
lxa_user_action_new(const gchar *name, const gchar *nick, const gchar *blurb, const gchar *icon, LXAUserActionFunc func, LXAArchive *archive, LXAArchiveSupport *support, gpointer user_data)
{
	LXAUserAction *action = g_new(LXAUserAction, 1);
	action->name = g_strdup(name);
	action->nick = g_strdup(nick);
	action->blurb = g_strdup(blurb);
	action->icon = g_strdup(icon);
	action->func = func;
	action->archive = archive;
	action->support = support;
	action->user_data = user_data;
	return action;
}

const gchar*
lxa_user_action_get_name(LXAUserAction *action)
{
	return action->name;
}

const gchar*
lxa_user_action_get_nick(LXAUserAction *action)
{
	return action->nick;
}

const gchar*
lxa_user_action_get_blurb(LXAUserAction *action)
{
	return action->blurb;
}

void
lxa_user_action_execute(LXAUserAction *action)
{
	action->func(action->archive, action->support, action->user_data);
}
