/* 
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

#include <config.h>
#include <string.h>
#include <glib.h>
#include <glib/gstdio.h>
#include <glib-object.h> 
#include <signal.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <thunar-vfs/thunar-vfs.h>

#include "libsqueeze.h"
#include "archive-iter.h"
#include "archive-tempfs.h"
#include "support-factory.h"
#include "archive.h"
#include "internals.h"

static void
lsq_support_factory_class_init(LSQSupportFactoryClass *);
static void
lsq_support_factory_init(LSQSupportFactory *);
static void
lsq_support_factory_dispose(GObject *object);
static void
lsq_support_factory_finalize(GObject *object);

static GObjectClass *parent_class;

GType
lsq_support_factory_get_type ()
{
	static GType lsq_support_factory_type = 0;

	if (!lsq_support_factory_type)
	{
		static const GTypeInfo lsq_support_factory_info = 
		{
			sizeof (LSQSupportFactoryClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) lsq_support_factory_class_init,
			(GClassFinalizeFunc) NULL,
			NULL,
			sizeof (LSQSupportFactory),
			0,
			(GInstanceInitFunc) lsq_support_factory_init,
			NULL
		};

		lsq_support_factory_type = g_type_register_static (G_TYPE_OBJECT, "LSQSupportFactory", &lsq_support_factory_info, 0);
	}
	return lsq_support_factory_type;
}

static void
lsq_support_factory_class_init(LSQSupportFactoryClass *support_factory_class)
{
	GObjectClass *object_class = G_OBJECT_CLASS(support_factory_class);

	object_class->dispose = lsq_support_factory_dispose;
	object_class->finalize = lsq_support_factory_finalize;

	parent_class = g_type_class_peek(G_TYPE_OBJECT); 

}

static void
lsq_support_factory_init(LSQSupportFactory *support_factory)
{
}

/**
 * lsq_support_factory_dispose:
 *
 * @object: LSQSupportFactory object
 *
 */
static void
lsq_support_factory_dispose(GObject *object)
{

	parent_class->dispose(object);
}

/**
 * lsq_support_factory_finalize:
 *
 * @object: LSQSupportFactory object
 *
 */
static void
lsq_support_factory_finalize(GObject *object)
{
	parent_class->finalize(object);
}

static gint
lsq_lookup_mime_support(gconstpointer a, gconstpointer b)
{
    return 0;
}

void
lsq_support_factory_add_template(LSQSupportFactory *factory, LSQSupportTemplate *s_template)
{
	GSList *result = g_slist_find_custom(lsq_mime_support_list, s_template, lsq_lookup_mime_support);
	if(!result)
	{
		factory->mime_support = g_slist_prepend(factory->mime_support, s_template);
		lsq_mime_support_list = g_slist_prepend(lsq_mime_support_list, s_template);
	}

}

