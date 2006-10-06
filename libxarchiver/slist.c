
#include <glib.h>
#include <glib-object.h>

#include "archive.h"

LXASList *
lxa_slist_insert_sorted_single(LXASList *list, LXAEntry *entry)
{
	gint cmp = 1;
	LXASList *iter = list;
	LXASList *prev_entry = NULL;
	LXASList *new_entry = NULL;

	for(; iter; iter = iter->next)
	{
		cmp = strcmp(entry->filename, ((LXAEntry*)iter->entry)->filename);

		if(!cmp)
		{
			/* TODO: merge same as in flush */
			return;
		}
		if(cmp < 0)
			break;

		prev_entry = iter;
	}

	new_entry = g_new(LXASList, 1);
	new_entry->next = iter;
	new_entry->entry = entry;

	if(!prev_entry)
		return new_entry;
	
	prev_entry->next = new_entry;
	return list;
}

guint
lxa_slist_length(LXASList *list)
{
	guint size = 0;
	for(; list; list = list->next)
		size++;
	return size;
}

void
lxa_slist_free(LXASList *list)
{
	LXASList *next;
	for(; list; list = next)
	{
		next = list->next;
		g_free(list);
	}
}
