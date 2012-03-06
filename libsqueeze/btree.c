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
#define LSQ_BTREE_MAX_DEPTH 20
#endif

LSQBTree *
lsq_btree_insert_sorted_single (
        LSQBTree *list,
        LSQArchiveEntry *entry,
        GCompareFunc cmp_func
    )
{
    gint cmp;
    LSQBTree *iter;
    LSQBTree **next = NULL;
    LSQBTree *new_entry = NULL;
    LSQBTree *stack[LSQ_BTREE_MAX_DEPTH];
    guint stack_i = 0;
    LSQArchiveEntry *swap_entry;
    LSQBTree *swap_iter;
    gint swap_balance;
    gboolean short_side;

#if DEBUG
    g_return_val_if_fail( NULL != entry, NULL );
    g_return_val_if_fail( NULL != cmp_func, NULL );
#endif

    /* Check for a flat list */
    if ( G_UNLIKELY ( NULL != list && NULL != list->next ) )
    {
        g_error( "Cannot insert into a flattened tree" );
        return NULL;
    }

    /* Walk the btree */
    for ( iter = list; NULL != iter; iter = *next )
    {
        /* archive can be NULL */
        cmp = cmp_func( entry, iter->entry );

        if ( 0 > cmp )
        {
            /* Go the the left */
            next = &iter->left;
        }
        else if ( 0 < cmp )
        {
            /* Go to the right */
            next = &iter->right;
        }
        else
        {
            /* Logic outside this routine dictates we should never find a match */
            g_error( "THIS SHOULD NOT HAPPEN!!! (the universe has just collapsed)" );
            return NULL;
        }

        /* Keep a stack of the path we followed */
        g_return_val_if_fail( LSQ_BTREE_MAX_DEPTH > stack_i, NULL );
        stack[stack_i++] = iter;
    }

    /* Create a new tree element */
    new_entry = g_new0( LSQBTree, 1 );
    new_entry->entry = entry;

    /* Check it this is a new tree */
    if ( NULL == next )
    {
        return new_entry;
    }

    /* Store ourself in the parent */
    *next = new_entry;

    /* Balance the tree */
    while ( 0 < stack_i )
    {
        iter = stack[--stack_i];

        /* Calculate new the balance for the parent */
        if ( iter->left == new_entry )
        {
            short_side = iter->balance > 0;
            --iter->balance;
        }
        else
        {
            short_side = iter->balance < 0;
            ++iter->balance;
        }

        /* The balance in the higher parents doesn't change when the short side changed */
        if ( FALSE != short_side )
        {
            break;
        }

        if ( 1 < iter->balance )
        {
            /* Rotate left */
            /* The code could be easier if we would just overwrite our parent left or right value.
             * But instead we move that data from our right to our self and use the right tree link to be placed in the tree as if it was ourself.
             */
            /* Letters are tree nodes, numbers are the data. This illustrates a rotate right.
             *
             *        A:4     |     A:2
             *       /   \    |    /   \
             *     B:2   C:5  |  D:1   B:4
             *    /   \       |       /   \
             *  D:1   E:3     |     E:3   C:5
             */
            swap_iter = iter->right;
            swap_balance = swap_iter->balance;
            /* Only balance if it helps */
            if ( 0 < swap_balance )
            {
                /* Swap the data */
                swap_entry = iter->entry;
                iter->entry = swap_iter->entry;
                swap_iter->entry = swap_entry;

                /* Reformat the tree links */
                iter->right = swap_iter->right;
                swap_iter->right = swap_iter->left;
                swap_iter->left = iter->left;
                iter->left = swap_iter;

                /* Fix the balance values
                 *
                 * if B > 0
                 *   A = A - 1 - B
                 * else
                 *   A = A - 1
                 *
                 * diff = A - 1 - B
                 * if diff < 0
                 *   B = B - 1 + diff
                 * else
                 *   B = B - 1
                 */
                swap_iter->balance = iter->balance - 1;
                if ( 0 < swap_balance )
                {
                    swap_iter->balance -= swap_balance;
                }
                iter->balance = iter->balance - 1 - swap_balance;
                if ( 0 < iter->balance )
                {
                    iter->balance = 0;
                }
                iter->balance += swap_balance - 1;

                /* We added a child so our depth was increased, but we also saved depth by rotation so our parents depth stays the same */
                break;
            }
        }
        else if ( -1 > iter->balance )
        {
            /* Rotate right */
            /* The code could be easier if we would just overwrite our parent left or right value.
             * But instead we move that data from our left to our self and use the left tree link to be placed in the tree as if it was ourself.
             */
            swap_iter = iter->left;
            swap_balance = swap_iter->balance;
            /* Only balance if it helps */
            if ( 0 > swap_balance )
            {
                /* Swap the data */
                swap_entry = iter->entry;
                iter->entry = swap_iter->entry;
                swap_iter->entry = swap_entry;

                /* Reformat the tree links */
                iter->left = swap_iter->left;
                swap_iter->left = swap_iter->right;
                swap_iter->right = iter->right;
                iter->right = swap_iter;

                /* Fix the balance values
                 *
                 * if B < 0
                 *   A = A + 1 - B
                 * else
                 *   A = A + 1
                 *
                 * diff = A + 1 - B
                 * if diff > 0
                 *   B = B + 1 + diff
                 * else
                 *   B = B + 1
                 */
                swap_iter->balance = iter->balance + 1;
                if ( 0 > swap_balance )
                {
                    swap_iter->balance -= swap_balance;
                }
                iter->balance = iter->balance + 1 - swap_balance;
                if ( 0 > iter->balance )
                {
                    iter->balance = 0;
                }
                iter->balance += swap_balance + 1;

                /* We added a child so our depth was increased, but we also saved depth by rotation so our parents depth stays the same */
                break;
            }
        }

        /* Store ourself in new_entry for the check in the next parent */
        new_entry = iter;
    }

    return list;
}

LSQBTree *
lsq_btree_remove_sorted_single (
        LSQBTree *list,
        gchar *filename,
        GCompareFunc cmp_func,
        LSQArchiveEntry **found )
{
    gint cmp;
    LSQBTree *iter;
    LSQBTree **next = &list;
    LSQBTree *del_entry;
    LSQBTree *stack[LSQ_BTREE_MAX_DEPTH];
    guint stack_i = 0;
    gboolean short_side;
    LSQBTree *swap_iter;
#ifdef BALANCE_ON_REMOVE
    LSQArchiveEntry *swap_entry;
    gint swap_balance;
#endif

#if DEBUG
    g_return_val_if_fail( NULL != filename, NULL );
    g_return_val_if_fail( NULL != cmp_func, NULL );
#endif

    if ( NULL != found )
    {
        *found = NULL;
    }

    /* The tree is flattened */
    if ( NULL == list || NULL != list->next )
    {
        for ( iter = list; NULL != iter; iter = iter->next)
        {
            cmp = cmp_func( filename, iter->entry );

            if ( 0 == cmp )
            {
                if ( NULL != found )
                {
                    *found = iter->entry;
                }

                /* remove it */
                *next = iter->next;
                g_free ( iter );
                return list;
            }
            else if ( 0 > cmp )
            {
                return list;
            }

            next = &iter->next;
        }
    }
    else
    {
        /* Walk the btree */
        for ( iter = list; NULL != iter; iter = *next )
        {
            /* archive can be NULL */
            cmp = cmp_func( filename, iter->entry );

            if ( 0 > cmp )
            {
                /* Go the the left */
                next = &iter->left;
            }
            else if ( 0 < cmp )
            {
                /* Go to the right */
                next = &iter->right;
            }
            else
            {
                if ( NULL != found )
                {
                    *found = iter->entry;
                }
                break;
            }

            /* Keep a stack of the path we followed */
            g_return_val_if_fail( LSQ_BTREE_MAX_DEPTH > stack_i, NULL );
            stack[stack_i++] = iter;
        }

        /* Not found */
        if ( NULL == iter )
        {
            return list;
        }

        if ( NULL == iter->left )
        {
            *next = iter->right;
            g_free ( iter );
        }
        else if ( NULL == iter->right )
        {
            *next = iter->left;
            g_free ( iter );
        }
        else
        {
            /* Find either the most right or most left element in the tree and replace iter with it.
             * We do this by replaceing the content of the iter, not the iter itself.
             *
             * We that the longest of the two paths, i the hope to be better balanced. */
            
            /* Keep a stack of the path we followed */
            g_return_val_if_fail( LSQ_BTREE_MAX_DEPTH > stack_i, NULL );
            stack[stack_i++] = iter;

            /* Right it the longest */
            if ( 0 < iter->balance )
            {
                /* Go right */
                for ( swap_iter = iter->right; NULL != swap_iter->left; swap_iter = *next )
                {
                    next = &swap_iter->left;

                    /* Keep a stack of the path we followed */
                    g_return_val_if_fail( LSQ_BTREE_MAX_DEPTH > stack_i, NULL );
                    stack[stack_i++] = iter;
                }

                /* remove the iter from the list */
                *next = swap_iter->right;

                /* copy the iter to our current location */
                iter->entry = swap_iter->entry;

                g_free ( swap_iter );
            }
            else
            {
                /* Go left */
                for ( swap_iter = iter->right; NULL != iter->left; iter = iter->left )
                {
                    next = &swap_iter->right;

                    /* Keep a stack of the path we followed */
                    g_return_val_if_fail( LSQ_BTREE_MAX_DEPTH > stack_i, NULL );
                    stack[stack_i++] = iter;
                }

                /* remove the iter from the list */
                *next = swap_iter->left;

                /* copy the iter to our current location */
                iter->entry = swap_iter->entry;

                g_free ( swap_iter );
            }
        }

        /* Get the new pointer for balancing */
        del_entry = *next;

        /* Balance the tree */
        while ( 0 < stack_i )
        {
            iter = stack[--stack_i];

            /* Calculate new the balance for the parent */
            if ( iter->left == del_entry )
            {
                short_side = 0 <= iter->balance;
                ++iter->balance;
            }
            else
            {
                short_side = 0 >= iter->balance;
                --iter->balance;
            }

            /* The balance in the higher parents doesn't change when the short side changed */
            if ( FALSE != short_side )
            {
#ifdef BALANCE_ON_REMOVE
                if ( 1 < iter->balance )
                {
                    /* Rotate left */
                    /* The code could be easier if we would just overwrite our parent left or right value.
                     * But instead we move that data from our right to our self and use the right tree link to be placed in the tree as if it was ourself.
                     */
                    /* Letters are tree nodes, numbers are the data. This illustrates a rotate right.
                     *
                     *        A:4     |     A:2
                     *       /   \    |    /   \
                     *     B:2   C:5  |  D:1   B:4
                     *    /   \       |       /   \
                     *  D:1   E:3     |     E:3   C:5
                     */
                    /* Swap the data */
                    swap_iter = iter->right;
                    swap_entry = iter->entry;
                    iter->entry = swap_iter->entry;
                    swap_iter->entry = swap_entry;

                    /* Reformat the tree links */
                    iter->right = swap_iter->right;
                    swap_iter->right = swap_iter->left;
                    swap_iter->left = iter->left;
                    iter->left = swap_iter;

                    /* Fix the balance values
                     *
                     * if B > 0
                     *   A = A - 1 - B
                     * else
                     *   A = A - 1
                     *
                     * diff = A - 1 - B
                     * if diff < 0
                     *   B = B - 1 + diff
                     * else
                     *   B = B - 1
                     */
                    swap_balance = swap_iter->balance;
                    swap_iter->balance = iter->balance - 1;
                    if ( 0 < swap_balance )
                    {
                        swap_iter->balance -= swap_balance;
                    }
                    iter->balance = iter->balance - 1 - swap_balance;
                    if ( 0 < iter->balance )
                    {
                        iter->balance = 0;
                    }
                    iter->balance += swap_balance - 1;

                    /* Saved depth by rotation so our parents depth also changes */
                    if ( 0 >= swap_balance )
                    {
                        break;
                    }
                }
                else if ( -1 > iter->balance )
                {
                    /* Rotate right */
                    /* The code could be easier if we would just overwrite our parent left or right value.
                     * But instead we move that data from our left to our self and use the left tree link to be placed in the tree as if it was ourself.
                     */
                    /* Swap the data */
                    swap_iter = iter->left;
                    swap_entry = iter->entry;
                    iter->entry = swap_iter->entry;
                    swap_iter->entry = swap_entry;

                    /* Reformat the tree links */
                    iter->left = swap_iter->left;
                    swap_iter->left = swap_iter->right;
                    swap_iter->right = iter->right;
                    iter->right = swap_iter;

                    /* Fix the balance values
                     *
                     * if B < 0
                     *   A = A + 1 - B
                     * else
                     *   A = A + 1
                     *
                     * diff = A + 1 - B
                     * if diff > 0
                     *   B = B + 1 + diff
                     * else
                     *   B = B + 1
                     */
                    swap_balance = swap_iter->balance;
                    swap_iter->balance = iter->balance + 1;
                    if ( 0 > swap_balance )
                    {
                        swap_iter->balance -= swap_balance;
                    }
                    iter->balance = iter->balance + 1 - swap_balance;
                    if ( 0 > iter->balance )
                    {
                        iter->balance = 0;
                    }
                    iter->balance += swap_balance + 1;

                    /* Saved depth by rotation so our parents depth also changes */
                    if ( 0 <= swap_balance )
                    {
                        break;
                    }
                }
                else
                {
                    break;
                }
#else
                break;
#endif
            }

            /* Store ourself in new_entry for the check in the next parent */
            del_entry = iter;
        }
    }

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
                g_return_val_if_fail( LSQ_BTREE_MAX_DEPTH > stack_i, 0 );

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
		g_return_if_fail( LSQ_BTREE_MAX_DEPTH > stack_i );
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
            g_return_val_if_fail( LSQ_BTREE_MAX_DEPTH > stack_i, NULL );
        }

        next = stack[--stack_i];

        *prev = next;
        prev = &next->next;

        next = next->right;
    }
    while ( 0 != stack_i || NULL != next );

    return list;
}

