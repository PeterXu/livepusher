#ifndef _QUEUE_H_
#define _QUEUE_H_

extern int create_queue();
extern int destroy_queue();

extern int queue_delete(int index);
extern int queue_delete_first();
extern int queue_delete_last();

extern int queue_size();
extern void* queue_get_first();
extern void* queue_get_last();
extern int queue_append_last(void *pval);
extern int queue_insert_first(void *pval);

#endif//_QUEUE_H_
