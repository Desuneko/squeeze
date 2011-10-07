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
#define _XOPEN_SOURCE
#include <time.h>
#include <string.h>
#include <glib.h>
#include <glib-object.h>
#include <gobject/gvaluecollector.h>
#include <gio/gio.h>

#include <libxfce4util/libxfce4util.h>

#include "libsqueeze.h"
#include "support-factory.h"
#include "support-reader.h"
#include "archive-iter.h"
#include "archive.h"

#include "internals.h"

static void
value_init_datetime ( GValue *value )
{
    value->data[0].v_pointer = NULL;
}

static void
value_free_datetime ( GValue *value )
{
    if ( ! ( value->data[1].v_uint & G_VALUE_NOCOPY_CONTENTS ) )
    {
        g_free( value->data[0].v_pointer );
    }
}

static void
value_copy_datetime (
        const GValue *src_value,
        GValue *dest_value )
{
    LSQDateTime *copy = NULL;
    if ( NULL != src_value->data[0].v_pointer )
    {
        copy = g_new( LSQDateTime, 1 );
        *copy = *LSQ_DATETIME(src_value->data[0].v_pointer);
    }
    dest_value->data[0].v_pointer = copy;
}

static gpointer
value_peek_datetime ( const GValue *value )
{
    return value->data[0].v_pointer;
}

static gchar *
value_collect_datetime (
        GValue *value,
        guint n_collect_values,
        GTypeCValue *collect_values,
        guint collect_flags )
{
    if ( NULL == collect_values[0].v_pointer )
    {
        value->data[0].v_pointer = NULL;
    }
    else if ( collect_flags & G_VALUE_NOCOPY_CONTENTS )
    {
        value->data[0].v_pointer = collect_values[0].v_pointer;
        value->data[1].v_uint = G_VALUE_NOCOPY_CONTENTS;
    }
    else
    {
        LSQDateTime *copy = NULL;
        if ( NULL != collect_values[0].v_pointer )
        {
            copy = g_new( LSQDateTime, 1 );
            *copy = *LSQ_DATETIME(collect_values[0].v_pointer);
        }
        value->data[0].v_pointer = copy;
    }

    return NULL;
}

static gchar *
value_lcopy_datetime (
        const GValue *value,
        guint n_collect_values,
        GTypeCValue *collect_values,
        guint collect_flags )
{
    LSQDateTime **dt_p = collect_values[0].v_pointer;

    if ( NULL == dt_p )
    {
        return g_strdup_printf ("value location for `%s' passed as NULL", G_VALUE_TYPE_NAME (value));
    }

    if ( NULL == value->data[0].v_pointer)
    {
        *dt_p = NULL;
    }
    else if ( collect_flags & G_VALUE_NOCOPY_CONTENTS )
    {
        *dt_p = value->data[0].v_pointer;
    }
    else
    {
        LSQDateTime *copy = NULL;
        if ( NULL != value->data[0].v_pointer )
        {
            copy = g_new( LSQDateTime, 1 );
            *copy = *LSQ_DATETIME(value->data[0].v_pointer);
        }
        *dt_p = copy;
    }

    return NULL;
}

static void
value_datetime_to_string (
        const GValue *src_value,
        GValue *dest_value )
{
    gchar buffer[200]; /* An abitrary size to fit the time string in */
    const LSQDateTime *dt = g_value_get_datetime( src_value );
    if ( NULL != dt )
    {
        strftime( buffer, sizeof(buffer), "%c", dt );
        g_value_set_string( dest_value, buffer );
    }
}

GType
lsq_datetime_get_type ( void )
{
    static GType type = G_TYPE_INVALID;

    if ( G_UNLIKELY( G_TYPE_INVALID == type ) )
    {
        GTypeValueTable value_table = {
            value_init_datetime,
            value_free_datetime,
            value_copy_datetime,
            value_peek_datetime,
            "p",
            value_collect_datetime,
            "p",
            value_lcopy_datetime
        };
        GTypeInfo type_info = {
            0,
            NULL,
            NULL,
            NULL,
            NULL,
            NULL,
            0,
            0,
            NULL,
            &value_table
        };
        GTypeFundamentalInfo fundamental_info = { 0 };

        type = g_type_register_fundamental(
                g_type_fundamental_next(),
                "LSQDateTime",
                &type_info,
                &fundamental_info,
                0
            );

	g_value_register_transform_func(
                type,
                G_TYPE_STRING,
                value_datetime_to_string
            );
    }

    return type;
}

void
lsq_datetime_register_type ( void )
{
    /* Force lsq_datetime_get_type to get called, and not optimized by G_GNUC_CONST */
    volatile GType type;
    type  = lsq_datetime_get_type();
}

LSQDateTime *
lsq_datetime_new_from_string (
        const gchar *str,
        const gchar *format,
        gchar **endp )
{
    LSQDateTime *dt;

    g_return_val_if_fail( NULL != str, NULL );
    g_return_val_if_fail( NULL != format, NULL );

    /* we don't expect it to fail, so the chance of an unnecessary alloc isn't high,
     * if it would fail most of the time, a read to stack and copy to a alloc on success would be better. */
    dt = g_new0( LSQDateTime, 1 );

    str = strptime( str, format, dt );

    if ( G_UNLIKELY( NULL == str ) )
    {
        g_free( dt );
        dt = NULL;
    }
    else if ( NULL != endp )
    {
        *endp = (gchar*)str;
    }

    return dt;
}

gchar *
lsq_datetime_from_string (
        LSQDateTime *dt,
        const gchar *str,
        const gchar *format )
{
    g_return_val_if_fail( NULL != dt, NULL );
    g_return_val_if_fail( NULL != str, NULL );
    g_return_val_if_fail( NULL != format, NULL );

    return strptime( str, format, dt );
}

gint
lsq_datetime_cmp (
        const LSQDateTime *a,
        const LSQDateTime *b )
{
    g_return_val_if_fail( NULL != a, 0 );
    g_return_val_if_fail( NULL != b, 0 );

    return difftime( mktime( (struct tm*)a ), mktime( (struct tm*)b ) );
}

const LSQDateTime *
g_value_get_datetime ( const GValue *value )
{
    g_return_val_if_fail( G_VALUE_HOLDS_DATETIME( value ), NULL );

    return value->data[0].v_pointer;
}

LSQDateTime *
g_value_dup_datetime ( const GValue *value )
{
    LSQDateTime *copy = NULL;

    g_return_val_if_fail( G_VALUE_HOLDS_DATETIME( value ), NULL );

    if ( NULL != value->data[0].v_pointer )
    {
        copy = g_new( LSQDateTime, 1 );
        *copy = *LSQ_DATETIME(value->data[0].v_pointer);
    }

    return copy;
}

void
g_value_set_datetime (
        GValue *value,
        const LSQDateTime *dt )
{
    LSQDateTime *new_val = NULL;

    g_return_if_fail( G_VALUE_HOLDS_DATETIME( value ) );

    if ( NULL != dt )
    {
        new_val = g_new( LSQDateTime, 1 );
        *new_val = *LSQ_DATETIME(dt);
    }

    if ( value->data[1].v_uint & G_VALUE_NOCOPY_CONTENTS )
    {
        value->data[1].v_uint = 0;
    }
    else
    {
        g_free( value->data[0].v_pointer );
    }

    value->data[0].v_pointer = new_val;
}

void
g_value_set_static_datetime (
        GValue *value,
        const gchar *dt )
{
    g_return_if_fail( G_VALUE_HOLDS_STRING( value ) );

    if ( ! ( value->data[1].v_uint & G_VALUE_NOCOPY_CONTENTS ) )
    {
        g_free (value->data[0].v_pointer);
    }
    value->data[1].v_uint = G_VALUE_NOCOPY_CONTENTS;
    value->data[0].v_pointer = LSQ_DATETIME(dt);
}

void
g_value_take_datetime (
        GValue *value,
        LSQDateTime *dt )
{
    g_return_if_fail( G_VALUE_HOLDS_STRING( value ) );

    if ( value->data[1].v_uint & G_VALUE_NOCOPY_CONTENTS) 
    {
        value->data[1].v_uint = 0;
    }
    else
    {
        g_free( value->data[0].v_pointer );
    }
    value->data[0].v_pointer = dt;
}

