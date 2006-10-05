
typedef struct _LXASList LXASList;

struct _LXASList {
	LXAEntry *entry;
	LXASList *next;
};

LXASList *
lxa_slist_insert_sorted_single(LXASList *list, LXAEntry *entry);

guint
lxa_slist_length(LXASList *list);

void
lxa_slist_free(LXASList *list);

