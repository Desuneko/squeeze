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
#include <gio/gio.h>

#include <libxfce4util/libxfce4util.h>

#include "libsqueeze.h"
#include "parser-context.h"
#include "parser.h"
#include "support-template.h"

GType
lsq_support_template_get_property_type (
        LSQSupportTemplate *templ,
        guint nr
    )
{
#ifdef DEBUG
    g_return_val_if_fail( NULL != templ, G_TYPE_NONE );
    g_return_val_if_fail( templ->n_properties > nr, G_TYPE_NONE );
    g_return_val_if_fail( LSQ_IS_PARSER( templ->parser ), G_TYPE_NONE);
#endif
    return lsq_parser_get_property_type( templ->parser, nr );
}

guint
lsq_support_template_get_property_offset (
        LSQSupportTemplate *templ,
        guint nr
    )
{
#ifdef DEBUG
    g_return_val_if_fail( NULL != templ, 0 );
    g_return_val_if_fail( templ->n_properties > nr, 0 );
    g_return_val_if_fail( LSQ_IS_PARSER( templ->parser ), 0 );
#endif
    return lsq_parser_get_property_offset( templ->parser, nr );
}

const gchar *
lsq_support_template_get_property_name (
        LSQSupportTemplate *templ,
        guint nr
    )
{
    g_return_val_if_fail( NULL != templ, NULL );
    g_return_val_if_fail( templ->n_properties > nr, NULL );

    return templ->property_names[nr];
}

guint
lsq_support_template_get_n_properties ( LSQSupportTemplate *templ )
{
    guint n_props;

#ifdef DEBUG
    g_return_val_if_fail( NULL != templ, 0 );
    g_return_val_if_fail( LSQ_IS_PARSER( templ->parser ), 0 );
#endif

    n_props = lsq_parser_n_properties( templ->parser );

    if ( templ->n_properties > n_props )
    {
        n_props = templ->n_properties;
    }
    return n_props;
}

guint
lsq_support_template_get_properties_size ( LSQSupportTemplate *templ )
{
#ifdef DEBUG
    g_return_val_if_fail( NULL != templ, 0 );
    g_return_val_if_fail( LSQ_IS_PARSER( templ->parser ), 0 );
#endif
    return lsq_parser_get_properties_size( templ->parser );
}
