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

#define TM_SEC_SIZE     (6)
#define TM_MIN_SIZE     (6)
#define TM_HOUR_SIZE    (5)
#define TM_MDAY_SIZE    (5)
#define TM_MON_SIZE     (4)
#define TM_YEAR_SIZE    (64 - TM_SEC_SIZE - TM_MIN_SIZE - TM_HOUR_SIZE - TM_MDAY_SIZE - TM_MON_SIZE - TM_WDAY_SIZE - TM_YDAY_SIZE - TM_ISDST_SIZE)
#define TM_WDAY_SIZE    (3)
#define TM_YDAY_SIZE    (9)
#define TM_ISDST_SIZE   (2)

#define TM_SEC_OFFSET   (0)
#define TM_MIN_OFFSET   (TM_SEC_OFFSET + TM_SEC_SIZE)
#define TM_HOUR_OFFSET  (TM_MIN_OFFSET + TM_MIN_SIZE)
#define TM_MDAY_OFFSET  (TM_HOUR_OFFSET + TM_HOUR_SIZE)
#define TM_MON_OFFSET   (TM_MDAY_OFFSET + TM_MDAY_SIZE)
#define TM_YEAR_OFFSET  (TM_MON_OFFSET + TM_MON_SIZE)
#define TM_WDAY_OFFSET  (TM_YEAR_OFFSET + TM_YEAR_SIZE)
#define TM_YDAY_OFFSET  (TM_WDAY_OFFSET + TM_WDAY_SIZE)
#define TM_ISDST_OFFSET (TM_YDAY_OFFSET + TM_YDAY_SIZE)

#define TM_X_MAKE(x,v) (((guint64)((v) & ((1<<(TM_##x##_SIZE)) - 1))) << (TM_##x##_OFFSET))
#define TM_X_GET(x,v) (((v) >> (TM_##x##_OFFSET)) & ((1<<(TM_##x##_SIZE)) - 1))
#define TM_X_MASK(x) (((guint64)((1<<(TM_##x##_SIZE)) - 1)) << (TM_##x##_OFFSET))
#define LSQ_DATETIME_CMP_MASK(v) ((v)&(TM_X_MASK(YEAR)|TM_X_MASK(MON)|TM_X_MASK(MDAY)|TM_X_MASK(HOUR)|TM_X_MASK(MIN)|TM_X_MASK(SEC)))

static void
value_init_datetime ( GValue *value )
{
    value->data[0].v_int64 = LSQ_DATETIME_NULL;
}

static void
value_copy_datetime (
        const GValue *src_value,
        GValue *dest_value
    )
{
    dest_value->data[0].v_int64 = src_value->data[0].v_int64;
}

static gchar *
value_collect_datetime (
        GValue *value,
        guint n_collect_values,
        GTypeCValue *collect_values,
        guint collect_flags
    )
{
    value->data[0].v_int64 = collect_values[0].v_int64;

    return NULL;
}

static gchar *
value_lcopy_datetime (
        const GValue *value,
        guint n_collect_values,
        GTypeCValue *collect_values,
        guint collect_flags
    )
{
    gint64 *int64_p = collect_values[0].v_pointer;

    if (!int64_p)
        return g_strdup_printf ("value location for `%s' passed as NULL", G_VALUE_TYPE_NAME (value));

    *int64_p = value->data[0].v_int64;

    return NULL;
}

static void
value_datetime_to_string (
        const GValue *src_value,
        GValue *dest_value
    )
{
    gchar buffer[80]; /* An abitrary size to fit the time string in */
    struct tm timeval;
    LSQDateTime dt = g_value_get_datetime( src_value );

    if ( LSQ_DATETIME_NULL != dt )
    {
        lsq_datetime_to_tm( dt, &timeval );
        strftime( buffer, sizeof(buffer), "%c", &timeval );
        g_value_set_string( dest_value, buffer );
    }
}

GType
lsq_datetime_get_type ( void )
{
    static GType type = G_TYPE_INVALID;

    if ( G_UNLIKELY ( G_TYPE_INVALID == type ) )
    {
        GTypeValueTable value_table = {
            value_init_datetime,
            NULL,
            value_copy_datetime,
            NULL,
            "q",
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
    type = lsq_datetime_get_type();
    type;
}

LSQDateTime
lsq_datetime_from_tm ( const struct tm *timeval )
{
#ifdef DEBUG
    g_return_val_if_fail( NULL != timeval, LSQ_DATETIME_NULL );
#endif

    return (
            TM_X_MAKE( SEC, timeval->tm_sec ) |
            TM_X_MAKE( MIN, timeval->tm_min ) |
            TM_X_MAKE( HOUR, timeval->tm_hour ) |
            TM_X_MAKE( MDAY, timeval->tm_mday ) |
            TM_X_MAKE( MON, timeval->tm_mon ) |
            TM_X_MAKE( YEAR, timeval->tm_year ) |
            TM_X_MAKE( WDAY, timeval->tm_wday ) |
            TM_X_MAKE( YDAY, timeval->tm_yday ) |
            TM_X_MAKE( ISDST, timeval->tm_isdst )
        );
}

LSQDateTime
lsq_datetime_from_string (
        const gchar *str,
        const gchar *format,
        gchar **endp
    )
{
    struct tm timeval;
    LSQDateTime dt = LSQ_DATETIME_NULL;

#ifdef DEBUG
    g_return_val_if_fail( NULL != str, LSQ_DATETIME_NULL );
    g_return_val_if_fail( NULL != format, LSQ_DATETIME_NULL );
#endif

    str = strptime( str, format, &timeval );

    if ( G_LIKELY( NULL != str ) )
    {
        if ( NULL != endp )
        {
            *endp = (gchar *)str;
        }

        dt = lsq_datetime_from_tm( &timeval );
    }

    return dt;
}

void
lsq_datetime_to_tm ( LSQDateTime dt, struct tm *timeval )
{
#ifdef DEBUG
    g_return_if_fail( LSQ_DATETIME_NULL!= dt );
    g_return_if_fail( NULL != timeval );
#endif

    memset( timeval, 0, sizeof(struct tm) );

    timeval->tm_sec = TM_X_GET( SEC, dt );
    timeval->tm_min = TM_X_GET( MIN, dt );
    timeval->tm_hour = TM_X_GET( HOUR, dt );
    timeval->tm_mday = TM_X_GET( MDAY, dt );
    timeval->tm_mon = TM_X_GET( MON, dt );
    timeval->tm_year = TM_X_GET( YEAR, dt );
    timeval->tm_wday = TM_X_GET( WDAY, dt );
    timeval->tm_yday = TM_X_GET( YDAY, dt );
    timeval->tm_isdst = TM_X_GET( ISDST, dt );
}

gint
lsq_datetime_cmp (
        LSQDateTime a,
        LSQDateTime b )
{
    gint cmp;

    if ( a == b )
        return 0;

    if ( LSQ_DATETIME_NULL == a )
        return -1;
    if ( LSQ_DATETIME_NULL == b )
        return 1;

    /* Ignoring daylight saveing */
    cmp = LSQ_DATETIME_CMP_MASK( a ) - LSQ_DATETIME_CMP_MASK( b );
    return cmp;
}

LSQDateTime
g_value_get_datetime ( const GValue *value )
{
#ifdef DEBUG
    g_return_val_if_fail( G_VALUE_HOLDS_DATETIME( value ), LSQ_DATETIME_NULL );
#endif

    return value->data[0].v_int64;
}

void
g_value_set_datetime (
        GValue *value,
        LSQDateTime v_dt )
{
#ifdef DEBUG
    g_return_if_fail( G_VALUE_HOLDS_DATETIME( value ) );
#endif

    value->data[0].v_int64 = v_dt;
}

