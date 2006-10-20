#ifndef __LXA_SLIST_H__
#define __LXA_SLIST_H__

typedef struct _LXASList LXASList;

struct _LXASList {
	LXAEntry *entry;
	LXASList *next;
};

LXASList *
lxa_slist_insert_sorted_single(LXASList *list, LXAEntry *entry, GCompareFunc);

guint
lxa_slist_length(LXASList *list);

void
lxa_slist_free(LXASList *list);

#endif /* __LXA_SLIST_H__ */
