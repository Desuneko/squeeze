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
#include "support-factory.h"
#include "internals.h"
#include "btree.h"

#ifndef LSQ_BTREE_MAX_DEPTH
#define LSQ_BTREE_MAX_DEPTH 500
#endif

LSQBTree *
lsq_btree_insert_sorted_single (
        LSQBTree *list,
        LSQArchiveEntry *entry,
        GCompareFunc cmp_func )
{
    gint cmp;
    LSQBTree *iter;
    LSQBTree **next = NULL;
    LSQBTree *new_entry = NULL;
    LSQBTree *stack[LSQ_BTREE_MAX_DEPTH];
    guint stack_i = 0;

    if ( NULL != list && NULL != list->next )
    {
        g_critical("Cannot insert into a flattened tree");
        return NULL;
    }

    for ( iter = list; NULL != iter; iter = *next )
    {
        /* archive can be NULL */
        cmp = cmp_func( entry, iter->entry );

        if ( 0 > cmp )
        {
            next = &iter->left;
        }
        else if ( 0 < cmp )
        {
            next = &iter->right;
        }
        else
        {
            g_critical("THIS SHOULD NOT HAPPEN!!! (the universe has just collapsed)");
            return NULL;
        }

        stack[stack_i++] = iter;
	g_return_val_if_fail(stack_i < LSQ_BTREE_MAX_DEPTH, NULL);
    }

    new_entry = g_new0(LSQBTree, 1);
    new_entry->entry = entry;

    if ( NULL == next )
    {
        return new_entry;
    }

    *next = new_entry;

    return list;
}

guint
lsq_btree_length ( LSQBTree *list )
{
    guint size = 0;
    LSQBTree *iter;
    LSQBTree *stack[LSQ_BTREE_MAX_DEPTH];
    guint stack_i = 0;

    /* The tree is flattened */
    if ( NULL == list || NULL != list->next )
    {
        for ( iter = list; NULL != iter; iter = iter->next)
        {
            size++;
        }
    }
    else
    {
        do
        {
            for ( iter = list; NULL != iter; iter = iter->left )
            {
                stack[stack_i++] = iter;
		g_return_val_if_fail(stack_i < LSQ_BTREE_MAX_DEPTH, 0);

                size++;
            }

            list = stack[--stack_i]->right;
        }
        while ( 0 != stack_i || NULL != list );
    }

    return size;
}

void
lsq_btree_free ( LSQBTree *list )
{
    LSQBTree *iter, *next;
    LSQBTree *stack[LSQ_BTREE_MAX_DEPTH];
    guint stack_i = 0;

    /* The tree is flattened */
    if ( NULL == list || NULL != list->next )
    {
        for ( iter = list; NULL != iter; iter = next)
        {
            next = iter->next;
            g_free( iter );
        }
    }
    else
    {
        do
        {
            for ( iter = list; NULL != iter; iter = iter->left )
            {
                stack[stack_i++] = iter;
		g_return_if_fail(stack_i < LSQ_BTREE_MAX_DEPTH);
            }

            list = stack[--stack_i]->right;

            g_free( stack[stack_i] );
        }
        while ( 0 != stack_i || NULL != list );
    }
}

LSQBTree *
lsq_btree_flatten ( LSQBTree *list )
{
    LSQBTree *iter, *next = list;
    LSQBTree **prev = &list;
    LSQBTree *stack[LSQ_BTREE_MAX_DEPTH];
    guint stack_i = 0;

    /* The tree is flattened */
    if ( NULL == list || NULL != list->next )
    {
        return list;
    }

    do
    {
        for ( iter = next; NULL != iter; iter = iter->left )
        {
            stack[stack_i++] = iter;
	    g_return_val_if_fail(stack_i < LSQ_BTREE_MAX_DEPTH, NULL);
        }

        next = stack[--stack_i];

        *prev = next;
        prev = &next->next;

        next = next->right;
    }
    while ( 0 != stack_i || NULL != next );

    return list;
}

