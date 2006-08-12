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

#define EXO_API_SUBJECT_TO_CHANGE

#include <glib.h>
#include <glib/gstdio.h>
#include <glib-object.h>
#include <thunar-vfs/thunar-vfs.h>

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
lxa_get_support_for_mime(gchar *mime)
{
	return lxa_get_support_for_mime_from_slist(lxa_archive_support_list, mime);
}

/*
 *
 */
LXAArchiveSupport *
lxa_get_support_for_mime_from_slist(GSList *list, gchar *mime)
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
	return g_strcasecmp((gchar *)support_mime, (gchar *)mime);
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
