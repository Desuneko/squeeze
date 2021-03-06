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
#include <glib-object.h> 
#include <gio/gio.h>

#include <libxfce4util/libxfce4util.h>

#include "libsqueeze.h"
#include "libsqueeze-view.h"




struct _LSQArchiveIterPool
{
    LSQArchiveIter **pool;
    guint size;
    guint reserved;
};

LSQArchiveIterPool *
lsq_archive_iter_pool_new ( void )
{
    return g_new0( LSQArchiveIterPool, 1 );
}

void
lsq_archive_iter_pool_free ( LSQArchiveIterPool *pool )
{
    /* lsq_archive_iter_pool_print();*/
    /* free the pool of iters */
    guint i;

    g_return_if_fail( NULL != pool );

    for ( i = 0; i < pool->size; ++i )
    {
        lsq_archive_iter_unref( pool->pool[i] );
    }
    for ( i = 0; i < pool->size; ++i )
    {
#ifdef USE_LSQITER_SLICES
        /* Cleaning up the whole pool */
        /* Now we can free the iters  */
#ifdef USE_GSLICES
        g_slice_free( LSQArchiveIter, pool->pool[i] );
#else
        g_free( pool->pool[i] );
#endif
#elif defined(USE_GSLICES)
        g_slice_free( LSQArchiveIter, pool->pool[i] );
#else
        g_free( pool->pool[i] );
#endif
    }
#ifdef USE_LSQITER_SLICES
    for ( ; i < pool->reserved; ++i )
    {
        /* Cleaning up the whole pool */
        /* Now we can free the iters  */
        if ( NULL == pool->pool[i] )
        {
            break;
        }
#ifdef USE_GSLICES
        g_slice_free( LSQArchiveIter, pool->pool[i] );
#else
        g_free( pool->pool[i] );
#endif
    }
#endif
    g_free( pool->pool );
    g_free( pool );
}

gboolean
lsq_archive_iter_pool_find_iter ( LSQArchiveIterPool *ipool, LSQArchiveEntry *entry, LSQArchiveIter **ret_iter, guint *ret_pos )
{
    /* binary search */
    LSQArchiveIter **pool = ipool->pool;
    guint size = ipool->size;
    guint pos;
    guint off = 0;
    gintptr cmp;

#if DEBUG
    g_return_val_if_fail( NULL != ipool, FALSE );
    g_return_val_if_fail( NULL != entry, FALSE );
#endif

    while ( 0 < size )
    {
        pos = size / 2;
        cmp = (guintptr)entry - (guintptr)pool[off+pos]->entry;
        if ( cmp == 0 )
        {
            if ( NULL != ret_iter )
            {
                (*ret_iter) = pool[off+pos];
            }
            if ( NULL != ret_pos )
            {
                (*ret_pos) = off+pos;
            }
            return TRUE;
        }
        if ( 0 < cmp )
        {
            size -= ++pos;
            off += pos;
        }
        if ( 0 > cmp )
        {
            size = pos;
        }
    }
    if ( NULL != ret_pos )
    {
        (*ret_pos) = off;
    }
    return FALSE;
}

void
lsq_archive_iter_pool_insert_iter ( LSQArchiveIterPool *ipool, LSQArchiveIter *iter, guint pos )
{
    LSQArchiveIter **pool, **old_pool = pool = ipool->pool;
    guint i;

#if DEBUG
    g_return_if_fail( NULL != ipool );
    g_return_if_fail( NULL != iter );
    g_return_if_fail( ipool->size >= pos );
#endif

    /* make space for new iter */
    if ( ipool->size >= ipool->reserved )
    {
        pool = g_new( LSQArchiveIter *, ipool->reserved + ipool->size + 1 );
        for ( i = 0; i < pos; ++i )
        {
            pool[i] = old_pool[i];
        }
    }

#ifdef USE_LSQITER_SLICES
    /* If we USE_LSQITER_SLICES we overwrite the first reserved pointer.
     * But we just picked that one out of the pool and use that as our current iter.
     */
#ifdef PADENTIC
    g_return_if_fail( NULL == old_pool[ipool->size] || old_pool[ipool->size] == iter );
#endif
#endif
    /* move all behind the iter */
    for ( i = ipool->size; i > pos; --i )
    {
        pool[i] = old_pool[i-1];
    }

    /* finish up the new pool */
    ipool->size++;
    if ( ipool->size > ipool->reserved )
    {
        ipool->reserved += ipool->size;
        ipool->pool = pool;
        g_free( old_pool );
#ifdef USE_LSQITER_SLICES
        /* We need to know if there are still allocations left */
        /* Make all unallocated NULL                           */
        for ( i = ipool->size; i < ipool->reserved; ++i )
        {
            pool[i] = NULL;
        }
#endif
    }

    /* insert the iter */
    pool[pos] = iter;
}

void
lsq_archive_iter_pool_remove_iter ( LSQArchiveIterPool *ipool, LSQArchiveIter *iter )
{
    LSQArchiveIter **pool = ipool->pool;
    guint pos;

#if DEBUG
    g_return_if_fail( NULL != ipool );
    g_return_if_fail( NULL != iter );
#endif

    /* iter has been found (should allways) */
    if ( G_LIKELY ( TRUE == lsq_archive_iter_pool_find_iter(ipool, iter->entry, NULL, &pos ) ) )
    {
        ipool->size--;

        for ( ; pos < ipool->size; ++pos )
        {
            pool[pos] = pool[pos+1];
        }
#ifdef USE_LSQITER_SLICES
        /* We don't free the pointer so move it */
        /* Place it at the end om the pool	  */
        pool[ipool->size] = iter;
#endif
    }
#if DEBUG
    else
    {
        g_return_if_reached();
    }
#endif
}

guint
lsq_archive_iter_pool_get_size ( LSQArchiveIterPool *pool )
{
#if DEBUG
    g_return_val_if_fail( NULL != pool, 0 );
#endif
    return pool->size;
}

guint
lsq_archive_iter_pool_get_reserved ( LSQArchiveIterPool *pool )
{
#if DEBUG
    g_return_val_if_fail( NULL != pool, 0 );
#endif
    return pool->reserved;
}

LSQArchiveIter *
lsq_archive_iter_pool_get_iter ( LSQArchiveIterPool *pool, guint index_ )
{
#if DEBUG
    g_return_val_if_fail( NULL != pool, NULL );
    g_return_val_if_fail( pool->reserved > index_, NULL );
#endif
    return pool->pool[index_];
}
