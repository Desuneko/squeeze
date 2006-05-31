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

void
lxa_archivesupport_init(LXAArchiveSupport *support);
void
lxa_archivesupport_class_init(LXAArchiveSupportClass *supportclass);

GType
lxa_archivesupport_get_type ()
{
	static GType lxa_archivesupport_type = 0;

 	if (!lxa_archivesupport_type)
	{
 		static const GTypeInfo lxa_archivesupport_info = 
		{
			sizeof (LXAArchiveSupportClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) lxa_archivesupport_class_init,
			(GClassFinalizeFunc) NULL,
			NULL,
			sizeof (LXAArchiveSupport),
			0,
			(GInstanceInitFunc) lxa_archivesupport_init,
		};

		lxa_archivesupport_type = g_type_register_static (G_TYPE_OBJECT, "LXAArchiveSupport", &lxa_archivesupport_info, 0);
	}
	return lxa_archivesupport_type;
}

void
lxa_archivesupport_init(LXAArchiveSupport *support)
{
}

void
lxa_archivesupport_class_init(LXAArchiveSupportClass *supportclass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (supportclass);
	LXAArchiveSupportClass *klass = LXA_ARCHIVESUPPORT_CLASS (supportclass);
}

LXAArchiveSupport*
lxa_archivesupport_new()
{
	LXAArchiveSupport*support;

	support = g_object_new(LXA_TYPE_ARCHIVESUPPORT, NULL);
	
	return support;
}
