/*
 * Insert a new entry between two known consecutive entries.
 *
 * This is only for internal list manipulation where we know
 * the prev/next entries already!
 */
DVBCORE_INTERFACE void __DVBCoreListAdd(struct DVBCoreListNodeS *new,
                        struct DVBCoreListNodeS *prev, struct DVBCoreListNodeS *next)
{
	next->prev = new;
	new->next = next;
	new->prev = prev;
	prev->next = new;
}

/**
 * list_add - add a new entry
 * @new: new entry to be added
 * @head: list head to add it after
 *
 * Insert a new entry after the specified head.
 * This is good for implementing stacks.
 */
DVBCORE_INTERFACE void DVBCoreListAdd(struct DVBCoreListNodeS *new, struct DVBCoreListS *list)
{
	__DVBCoreListAdd(new, (struct DVBCoreListNodeS *)list, list->head);
}

DVBCORE_INTERFACE void DVBCoreListAddBefore(struct DVBCoreListNodeS *new, 
                                    struct DVBCoreListNodeS *pos)
{
	__DVBCoreListAdd(new, pos->prev, pos);
}

DVBCORE_INTERFACE void DVBCoreListAddAfter(struct DVBCoreListNodeS *new, 
                                    struct DVBCoreListNodeS *pos)
{
	__DVBCoreListAdd(new, pos, pos->next);
}

DVBCORE_INTERFACE void DVBCoreListAddTail(struct DVBCoreListNodeS *new, struct DVBCoreListS *list)
{
	__DVBCoreListAdd(new, list->tail, (struct DVBCoreListNodeS *)list);
}

/*
 * Delete a list entry by making the prev/next entries
 * point to each other.
 *
 * This is only for internal list manipulation where we know
 * the prev/next entries already!
 */
DVBCORE_INTERFACE void __DVBCoreListDel(struct DVBCoreListNodeS *prev, struct DVBCoreListNodeS *next)
{
	next->prev = prev;
	prev->next = next;
}

/**
 * list_del - deletes entry from list.
 * @entry: the element to delete from the list.
 * Note: list_empty() on entry does not return true after this, the entry is
 * in an undefined state.
 */
DVBCORE_INTERFACE void __DVBCoreListDelEntry(struct DVBCoreListNodeS *node)
{
	__DVBCoreListDel(node->prev, node->next);
}

DVBCORE_INTERFACE void DVBCoreListDel(struct DVBCoreListNodeS *node)
{
	__DVBCoreListDel(node->prev, node->next);
	node->next = DVBCORE_LIST_POISON1;
	node->prev = DVBCORE_LIST_POISON2;
}

/**
 * list_replace - replace old entry by new one
 * @old : the element to be replaced
 * @new : the new element to insert
 *
 * If @old was empty, it will be overwritten.
 */
DVBCORE_INTERFACE void DVBCoreListReplace(struct DVBCoreListNodeS *old,
				                struct DVBCoreListNodeS *new)
{
	new->next = old->next;
	new->next->prev = new;
	new->prev = old->prev;
	new->prev->next = new;
}

DVBCORE_INTERFACE void DVBCoreListReplaceInit(struct DVBCoreListNodeS *old,
					            struct DVBCoreListNodeS *new)
{
	DVBCoreListReplace(old, new);
	DVBCoreListNodeInit(old);
}

/**
 * list_del_init - deletes entry from list and reinitialize it.
 * @entry: the element to delete from the list.
 */
DVBCORE_INTERFACE void DVBCoreListDelInit(struct DVBCoreListNodeS *node)
{
	__DVBCoreListDelEntry(node);
	DVBCoreListNodeInit(node);
}

/**
 * list_move - delete from one list and add as another's head
 * @list: the entry to move
 * @head: the head that will precede our entry
 */
DVBCORE_INTERFACE void DVBCoreListMove(struct DVBCoreListNodeS *node, struct DVBCoreListS *list)
{
	__DVBCoreListDelEntry(node);
	DVBCoreListAdd(node, list);
}

/**
 * list_move_tail - delete from one list and add as another's tail
 * @list: the entry to move
 * @head: the head that will follow our entry
 */
DVBCORE_INTERFACE void DVBCoreListMoveTail(struct DVBCoreListNodeS *node, 
                                struct DVBCoreListS *list)
{
	__DVBCoreListDelEntry(node);
	DVBCoreListAddTail(node, list);
}

/**
 * list_is_last - tests whether @list is the last entry in list @head
 * @list: the entry to test
 * @head: the head of the list
 */
DVBCORE_INTERFACE int DVBCoreListIsLast(const struct DVBCoreListNodeS *node,
				            const struct DVBCoreListS *list)
{
	return node->next == (struct DVBCoreListNodeS *)list;
}

/**
 * list_empty - tests whether a list is empty
 * @head: the list to test.
 */
DVBCORE_INTERFACE int DVBCoreListEmpty(const struct DVBCoreListS *list)
{
	return (list->head == (struct DVBCoreListNodeS *)list) 
	       && (list->tail == (struct DVBCoreListNodeS *)list);
}

/**
 * list_rotate_left - rotate the list to the left
 * @head: the head of the list
 */
DVBCORE_INTERFACE void DVBCoreListRotateLeft(struct DVBCoreListS *list)
{
	struct DVBCoreListNodeS *first;

	if (!DVBCoreListEmpty(list)) {
		first = list->head;
		DVBCoreListMoveTail(first, list);
	}
}

/**
 * list_is_singular - tests whether a list has just one entry.
 * @head: the list to test.
 */
DVBCORE_INTERFACE int DVBCoreListIsSingular(const struct DVBCoreListS *list)
{
	return !DVBCoreListEmpty(list) && (list->head == list->tail);
}


