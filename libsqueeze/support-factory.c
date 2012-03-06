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
#include <gio/gio.h>

#include <libxfce4util/libxfce4util.h>

#include "libsqueeze.h"
#include "archive-iter.h"
#include "archive-tempfs.h"
#include "support-factory.h"
#include "archive.h"
#include "internals.h"

static void
lsq_support_factory_finalize ( GObject *object );

G_DEFINE_TYPE ( LSQSupportFactory, lsq_support_factory, G_TYPE_OBJECT );

static void
lsq_support_factory_class_init ( LSQSupportFactoryClass *support_factory_class )
{
    GObjectClass *object_class = G_OBJECT_CLASS(support_factory_class);

    object_class->finalize = lsq_support_factory_finalize;
}

static void
lsq_support_factory_init ( LSQSupportFactory *support_factory )
{
}

/**
 * lsq_support_factory_finalize:
 *
 * @object: LSQSupportFactory object
 *
 */
static void
lsq_support_factory_finalize ( GObject *object )
{
    LSQSupportFactory *factory = LSQ_SUPPORT_FACTORY(object);

    g_free( factory->filename );
    g_free( factory->id );
    g_slist_free( factory->mime_support );

    G_OBJECT_CLASS(lsq_support_factory_parent_class)->finalize( object );
}

static gint
lsq_lookup_mime_support ( gconstpointer a, gconstpointer b )
{
    return 1;
}

void
lsq_support_factory_add_template ( LSQSupportFactory *factory, LSQSupportTemplate *s_template )
{
    GSList *result;
   
    g_return_if_fail( LSQ_IS_SUPPORT_FACTORY( factory ) );
    g_return_if_fail( NULL != s_template );

    result = g_slist_find_custom( lsq_mime_support_list, s_template, lsq_lookup_mime_support );
    if ( NULL == result )
    {
        factory->mime_support = g_slist_prepend( factory->mime_support, s_template );
        lsq_mime_support_list = g_slist_prepend( lsq_mime_support_list, s_template );
    }
}

gint
lsq_suport_factory_compare_filename ( const LSQSupportFactory *factory, const gchar *filename )
{
    g_return_val_if_fail( LSQ_IS_SUPPORT_FACTORY( factory ), 1 );
    g_return_val_if_fail( NULL != filename, 1 );

    return strcmp( factory->filename, filename );
}

