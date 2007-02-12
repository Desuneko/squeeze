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
#include <thunar-vfs/thunar-vfs.h>

#include "libsqueeze-archive.h"
#include "archive-iter.h"
#include "archive-command.h"
#include "archive.h"
#include "internals.h"
#include "slist.h"

LSQSList *
lsq_slist_insert_sorted_single(LSQSList *list, LSQArchiveEntry *entry, GCompareFunc cmp_func)
{
	gint cmp = 1;
	LSQSList *iter = list;
	LSQSList *prev_entry = NULL;
	LSQSList *new_entry = NULL;

	for(; iter; iter = iter->next)
	{
		/* archive can be NULL */
		cmp = cmp_func(entry, (LSQArchiveEntry*)iter->entry);

		if(!cmp)
		{
			g_critical("THIS SHOULD NOT HAPPEN!!! (the universe has just collapsed)");
			return NULL;
		}
		if(cmp < 0)
			break;

		prev_entry = iter;
	}

	new_entry = g_new0(LSQSList, 1);
	new_entry->next = iter;
	new_entry->entry = entry;

	if(!prev_entry)
		return new_entry;
	
	prev_entry->next = new_entry;
	return list;
}

guint
lsq_slist_length(LSQSList *list)
{
	guint size = 0;
	for(; list; list = list->next)
		size++;
	return size;
}

void
lsq_slist_free(LSQSList *list)
{
	LSQSList *next;
	for(; list; list = next)
	{
		next = list->next;
		g_free(list);
	}
}

