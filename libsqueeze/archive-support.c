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
lsq_archive_support_init(LSQArchiveSupport *support);
void
lsq_archive_support_class_init(LSQArchiveSupportClass *supportclass);

/*
 *
 */
GType
lsq_archive_support_get_type ()
{
	static GType lsq_archive_support_type = 0;

 	if (!lsq_archive_support_type)
	{
 		static const GTypeInfo lsq_archive_support_info = 
		{
			sizeof (LSQArchiveSupportClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) lsq_archive_support_class_init,
			(GClassFinalizeFunc) NULL,
			NULL,
			sizeof (LSQArchiveSupport),
			0,
			(GInstanceInitFunc) lsq_archive_support_init,
		};

		lsq_archive_support_type = g_type_register_static (G_TYPE_OBJECT, "LSQArchiveSupport", &lsq_archive_support_info, 0);
	}
	return lsq_archive_support_type;
}

/*
 *
 */
void
lsq_archive_support_init(LSQArchiveSupport *support)
{
	support->add = NULL;
	support->extract = NULL;
	support->remove = NULL;
	support->refresh = NULL;
	support->custom_action = NULL;
}

/*
 *
 */
void
lsq_archive_support_class_init(LSQArchiveSupportClass *supportclass)
{
	/*
	GObjectClass *gobject_class = G_OBJECT_CLASS (supportclass);
	LSQArchiveSupportClass *klass = LSQ_ARCHIVE_SUPPORT_CLASS (supportclass);
	*/
}

/*
 *
 */
LSQArchiveSupport*
lsq_archive_support_new()
{
	LSQArchiveSupport*support;

	support = g_object_new(LSQ_TYPE_ARCHIVE_SUPPORT, NULL);
	
	return support;
}

/*
 *
 */
void
lsq_archive_support_add_mime(LSQArchiveSupport *support, gchar *mime)
{
	support->mime = g_slist_prepend(support->mime, mime);	
}

/*
 *
 */
gboolean
lsq_archive_support_mime_supported(LSQArchiveSupport *support, const gchar *mime)
{
	GSList *result = g_slist_find_custom(support->mime, mime, lsq_archive_support_lookup_mime);
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
lsq_register_support(LSQArchiveSupport *support)
{ 
	if(!LSQ_IS_ARCHIVE_SUPPORT(support))
		return FALSE;

	lsq_archive_support_list = g_slist_prepend(lsq_archive_support_list, support);	
	g_object_ref(support);

	return TRUE;
}

/*
 *
 */
LSQArchiveSupport *
lsq_get_support_for_mime(const gchar *mime)
{
	return lsq_get_support_for_mime_from_slist(lsq_archive_support_list, mime);
}

/*
 *
 */
LSQArchiveSupport *
lsq_get_support_for_mime_from_slist(GSList *list, const gchar *mime)
{
	GSList *result = g_slist_find_custom(list, mime, lsq_archive_support_lookup_support);
	if(result)
		return result->data;
	return NULL;
}

/*
 *
 */
gint
lsq_archive_support_lookup_mime(gconstpointer support_mime, gconstpointer mime)
{
	return strcmp((gchar *)support_mime, (gchar *)mime);
}

/*
 *
 */
gint
lsq_archive_support_lookup_support(gconstpointer support, gconstpointer mime)
{
	if(lsq_archive_support_mime_supported(LSQ_ARCHIVE_SUPPORT(support), (gchar *)mime))
		return 0;
	else
		return 1;
}		

gint
lsq_archive_support_add(LSQArchiveSupport *support, LSQArchive *archive, GSList *files)
{
	if(support->add)
	{
		lsq_archive_set_status(archive, LSQ_ARCHIVESTATUS_ADD);
		archive->support = support;
		return support->add(archive, files);
	}
	else
		g_critical("ADD NOT IMPLEMENTED BY SUPPORT OBJECT '%s'", support->id);
	return -1;
}

gint
lsq_archive_support_extract(LSQArchiveSupport *support, LSQArchive *archive, gchar *dest_path, GSList *files)
{
	if(support->extract)
	{
		lsq_archive_set_status(archive, LSQ_ARCHIVESTATUS_EXTRACT);
		archive->support = support;
		return support->extract(archive, dest_path, files);
	}
	else
		g_critical("EXTRACT NOT IMPLEMENTED BY SUPPORT OBJECT '%s'", support->id);
	return -1;
}

gint
lsq_archive_support_remove(LSQArchiveSupport *support, LSQArchive *archive, GSList *files)
{
	if(support->remove)
	{
		lsq_archive_set_status(archive, LSQ_ARCHIVESTATUS_REMOVE);
		archive->support = support;
		return support->remove(archive, files);
	}
	else
		g_critical("REMOVE NOT IMPLEMENTED BY SUPPORT OBJECT '%s'", support->id);
	return -1;
}

gint
lsq_archive_support_refresh(LSQArchiveSupport *support, LSQArchive *archive)
{
	if(support->refresh)
	{
		lsq_archive_set_status(archive, LSQ_ARCHIVESTATUS_REFRESH);
		archive->support = support;
		return support->refresh(archive);
	}
	else
		g_critical("VIEW NOT IMPLEMENTED BY SUPPORT OBJECT '%s'", support->id);
	return -1;
}

GSList *
lsq_archive_support_list_properties(LSQArchiveSupport *support, gchar *prefix)
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
lsq_archive_support_install_action(LSQArchiveSupport *support, LSQCustomAction *action)
{
	support->custom_action = g_slist_append(support->custom_action, action);
}

LSQCustomAction*
lsq_archive_support_find_action(LSQArchiveSupport *support, const gchar *name)
{
	GSList *actions = support->custom_action;
	while(actions)
	{
		if(strcmp(((LSQCustomAction*)actions->data)->name, name) == 0)
			return (LSQCustomAction*)actions->data;
		actions = actions->next;
	}
	return NULL;
}

LSQCustomAction**
lsq_archive_support_list_actions(LSQArchiveSupport *support, guint *n_actions)
{
	LSQCustomAction** list;
	guint i = 0;
	GSList *actions = support->custom_action;
	(*n_actions) = g_slist_length(actions);
	list = g_new(LSQCustomAction*, *n_actions);
	while(actions)
	{
		list[i++] = (LSQCustomAction*)actions->data;
		actions = actions->next;
	}
	return list;
}

LSQCustomAction*
lsq_custom_action_new(const gchar *name, const gchar *nick, const gchar *blurb, const gchar *icon, LSQCustomActionFunc func, LSQArchiveSupport *support, gpointer user_data)
{
	LSQCustomAction *action = g_new(LSQCustomAction, 1);
	action->name = g_strdup(name);
	action->nick = g_strdup(nick);
	action->blurb = g_strdup(blurb);
	action->icon = g_strdup(icon);
	action->func = func;
	action->support = support;
	action->user_data = user_data;
	return action;
}

const gchar*
lsq_custom_action_get_name(LSQCustomAction *action)
{
	return action->name;
}

const gchar*
lsq_custom_action_get_nick(LSQCustomAction *action)
{
	return action->nick;
}

const gchar*
lsq_custom_action_get_blurb(LSQCustomAction *action)
{
	return action->blurb;
}

void
lsq_custom_action_execute(LSQCustomAction *action, LSQArchive *archive, LSQCustomActionPreFunc pre_func, LSQCustomActionPostFunc post_func)
{
	if(pre_func)
	{
		if(pre_func(action) == FALSE)
			return;
	}
	/* set post-condition-callback */
	action->post_func = post_func; 	
	action->func(action->support, archive, action->user_data);
}