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
#include "compression-support.h"

#define _(String) gettext(String)

void
lxa_compressionsupport_init(LXACompressionSupport *support);
void
lxa_compressionsupport_class_init(LXACompressionSupportClass *supportclass);

GType
lxa_compressionsupport_get_type ()
{
	static GType lxa_compressionsupport_type = 0;

 	if (!lxa_compressionsupport_type)
	{
 		static const GTypeInfo lxa_compressionsupport_info = 
		{
			sizeof (LXACompressionSupportClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) lxa_compressionsupport_class_init,
			(GClassFinalizeFunc) NULL,
			NULL,
			sizeof (LXACompressionSupport),
			0,
			(GInstanceInitFunc) lxa_compressionsupport_init,
		};

		lxa_compressionsupport_type = g_type_register_static (G_TYPE_OBJECT, "LXACompressionSupport", &lxa_compressionsupport_info, 0);
	}
	return lxa_compressionsupport_type;
}

void
lxa_compressionsupport_init(LXACompressionSupport *support)
{
}

void
lxa_compressionsupport_class_init(LXACompressionSupportClass *supportclass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (supportclass);
	LXACompressionSupportClass *klass = LXA_COMPRESSIONSUPPORT_CLASS (supportclass);
}

LXACompressionSupport*
lxa_compressionsupport_new()
{
	LXACompressionSupport*support;

	support = g_object_new(LXA_TYPE_COMPRESSIONSUPPORT, NULL);
	
	return support;
}
