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

#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <glib.h>
#include <glib-object.h>
#include <libintl.h>
#include "archive.h"
#include "archive-support.h"

#define _(String) gettext(String)

gint
lxa_archive_support_dummy(LXAArchive *archive);


void
lxa_archive_support_init(LXAArchiveSupport *support);
void
lxa_archive_support_class_init(LXAArchiveSupportClass *supportclass);

static guint lxa_archive_support_signals[2];

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

gint
lxa_archive_support_dummy(LXAArchive *archive)
{
}

void
lxa_archive_support_init(LXAArchiveSupport *support)
{
	support->add = lxa_archive_support_dummy;
	support->extract = lxa_archive_support_dummy;
}

void
lxa_archive_support_class_init(LXAArchiveSupportClass *supportclass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (supportclass);
	LXAArchiveSupportClass *klass = LXA_ARCHIVE_SUPPORT_CLASS (supportclass);

	lxa_archive_support_signals[0] = g_signal_new("lxa_add_complete",
			G_TYPE_FROM_CLASS(supportclass),
			G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
			0,
			NULL,
			NULL,
			g_cclosure_marshal_VOID__POINTER,
			G_TYPE_NONE,
			1,
			G_TYPE_POINTER,
			NULL);
	lxa_archive_support_signals[1] = g_signal_new("lxa_extract_complete",
			G_TYPE_FROM_CLASS(supportclass),
			G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
			0,
			NULL,
			NULL,
			g_cclosure_marshal_VOID__POINTER,
			G_TYPE_NONE,
			1,
			G_TYPE_POINTER,
			NULL);
}

LXAArchiveSupport*
lxa_archive_support_new()
{
	LXAArchiveSupport*support;

	support = g_object_new(LXA_TYPE_ARCHIVE_SUPPORT, NULL);
	
	return support;
}

void
lxa_archive_support_emit_signal(LXAArchiveSupport *support, guint signal_id, LXAArchive *archive)
{
	g_signal_emit(G_OBJECT(support), lxa_archive_support_signals[signal_id], 0, archive);
}
