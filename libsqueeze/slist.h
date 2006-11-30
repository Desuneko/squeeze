#ifndef __LSQ_SLIST_H__
#define __LSQ_SLIST_H__

typedef struct _LSQSList LSQSList;

struct _LSQSList {
	LSQEntry *entry;
	LSQSList *next;
};

LSQSList *
lsq_slist_insert_sorted_single(LSQSList *list, LSQEntry *entry, GCompareFunc);

guint
lsq_slist_length(LSQSList *list);

void
lsq_slist_free(LSQSList *list);

#endif /* __LSQ_SLIST_H__ */
