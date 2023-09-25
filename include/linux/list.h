#ifndef _LINUX_LIST_H
#define _LINUX_LIST_H

/*
 * Simple doubly linked list implementation.
 *
 * Some of the internal functions ("__xxx") are useful when
 * manipulating whole lists rather than single entries, as
 * sometimes we already know the next/prev entries and we can
 * generate better code by using them directly rather than
 * using the generic single-entry routines.
 */

struct list_head {
	struct list_head *next, *prev;
};
// 定义一个链表头结点的初始化器，将头结点的next和prev指针都指向自身
#define LIST_HEAD_INIT(name) { &(name), &(name) }
// 定义一个链表头结点变量，并使用LIST_HEAD_INIT宏进行初始化
#define LIST_HEAD(name) \
	struct list_head name = LIST_HEAD_INIT(name)

// 对list_head进行初始化，将其next和prev指针都指向自身
#define INIT_LIST_HEAD(ptr) do { \
	(ptr)->next = (ptr); (ptr)->prev = (ptr); \
} while (0)

/*
 * Insert a new entry between two known consecutive entries. 
 *
 * This is only for internal list manipulation where we know
 * the prev/next entries already!
 */
// 在已知两个相邻节点(prev和next)的情况下，将一个新节点(new)插入到它们之间
// 注意，这个函数是供内部使用的，用于在处理整个链表而不是单个节点时提高效率。
static inline void __list_add(struct list_head *new,
			      struct list_head *prev,
			      struct list_head *next)		//添加
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
static inline void list_add(struct list_head *new, struct list_head *head)
{
	//在list_head中添加
	__list_add(new, head, head->next);
}

/**
 * list_add_tail - add a new entry
 * @new: new entry to be added
 * @head: list head to add it before
 *
 * Insert a new entry before the specified head.
 * This is useful for implementing queues.
 */
// 将一个新节点(new)添加到链表的尾部。
static inline void list_add_tail(struct list_head *new, struct list_head *head)
{
	__list_add(new, head->prev, head);
}

/*
 * Delete a list entry by making the prev/next entries
 * point to each other.
 *
 * This is only for internal list manipulation where we know
 * the prev/next entries already!
 */
// 通过将前一个节点(prev)的next指针和下一个节点(next)的prev指针指向彼此，从链表中删除一个节点。
// 注意，这个函数是供内部使用的，用于在处理整个链表而不是单个节点时提高效率。
static inline void __list_del(struct list_head *prev, struct list_head *next)
{
	next->prev = prev;
	prev->next = next;
}

/**
 * list_del - deletes entry from list.
 * @entry: the element to delete from the list.
 * Note: list_empty on entry does not return true after this, the entry is in an undefined state.
 */
// list_del(entry): 从链表中删除指定的节点(entry)。
// 它调用了__list_del函数来删除节点，并将节点的next和prev指针设置为NULL。
// 注意，删除节点后，该节点处于未定义状态，不能再使用list_empty函数来判断链表是否为空。
static inline void list_del(struct list_head *entry)
{	//删除
	__list_del(entry->prev, entry->next);
	entry->next = (void *) 0;
	entry->prev = (void *) 0;
}

/**
 * list_del_init - deletes entry from list and reinitialize it.
 * @entry: the element to delete from the list.
 */
// 从链表中删除指定的节点(entry)并重新初始化它。
// 它调用了__list_del函数来删除节点，并使用INIT_LIST_HEAD宏重新初始化节点。
static inline void list_del_init(struct list_head *entry)
{
	__list_del(entry->prev, entry->next);
	INIT_LIST_HEAD(entry); 
}

/**
 * list_move - delete from one list and add as another's head
 * @list: the entry to move
 * @head: the head that will precede our entry
 */
// 将一个节点(list)从原链表中删除，并添加到另一个链表的头部(head)。
// 它调用了__list_del函数将节点从原链表中删除，然后使用list_add将节点添加到指定链表的头部。
static inline void list_move(struct list_head *list, struct list_head *head)
{
        __list_del(list->prev, list->next);
        list_add(list, head);
}

/**
 * list_move_tail - delete from one list and add as another's tail
 * @list: the entry to move
 * @head: the head that will follow our entry
 */
// 将一个节点(list)从原链表中删除，并添加到另一个链表的尾部(head)。
// 调用了__list_del函数将节点从原链表中删除，然后使用list_add_tail将节点添加到指定链表的尾部。
static inline void list_move_tail(struct list_head *list,
				  struct list_head *head)
{
        __list_del(list->prev, list->next);
        list_add_tail(list, head);
}

/**
 * list_empty - tests whether a list is empty
 * @head: the list to test.
 */
// 判断链表是否为空。如果链表的头结点的next指针指向自身，即链表为空，返回1；否则，返回0。
static inline int list_empty(struct list_head *head)
{
	return head->next == head;
}

// 将一个链表(list)插入到另一个链表(head)的后面。
static inline void __list_splice(struct list_head *list,
				 struct list_head *head)
{
	struct list_head *first = list->next;
	struct list_head *last = list->prev;
	struct list_head *at = head->next;

	first->prev = head;
	head->next = first;

	last->next = at;
	at->prev = last;
}

/**
 * list_splice - join two lists
 * @list: the new list to add.
 * @head: the place to add it in the first list.
 */
// 将一个链表(list)连接到另一个链表(head)的后面。
static inline void list_splice(struct list_head *list, struct list_head *head)
{
	if (!list_empty(list))
		__list_splice(list, head);
}

/**
 * list_splice_init - join two lists and reinitialise the emptied list.
 * @list: the new list to add.
 * @head: the place to add it in the first list.
 *
 * The list at @list is reinitialised
 */
// 将一个链表(list)连接到另一个链表(head)的后面，并重新初始化被连接的链表。
// list是链表的头节点，这个操作将链表的头结点的next和prev都指向自己
static inline void list_splice_init(struct list_head *list,
				    struct list_head *head)
{
	if (!list_empty(list)) {
		__list_splice(list, head);
		INIT_LIST_HEAD(list);
	}
}

/**
 * list_entry - 获取包含此条目的结构体
 * @ptr: 指向 struct list_head 的指针。
 * @type: 嵌入在其中的结构体的类型。
 * @member: 结构体中 list_head 的名称。
 */
// 通过指向 struct list_head 的指针计算出包含此条目的宿主结构的地址
#define list_entry(ptr, type, member) \
	((type *)((char *)(ptr)-(unsigned long)(&((type *)0)->member)))

/**
 * list_for_each - 遍历链表
 * @pos: 用作循环计数器的 struct list_head。
 * @head: 链表的头节点。
 */
// 遍历链表。通过传递指向链表头节点的指针 head，
// 以及一个用作循环计数器的 struct list_head 指针 pos，可以在循环中访问链表中的每个节点。
#define list_for_each(pos, head) \
	for (pos = (head)->next, prefetch(pos->next); pos != (head); \
        	pos = pos->next, prefetch(pos->next))
/**
 * list_for_each_prev - 反向遍历链表
 * @pos: 用作循环计数器的 struct list_head。
 * @head: 链表的头节点。
 */
#define list_for_each_prev(pos, head) \
	for (pos = (head)->prev, prefetch(pos->prev); pos != (head); \
        	pos = pos->prev, prefetch(pos->prev))
        	
/**
 * list_for_each_safe - 安全遍历链表，支持删除链表节点
 * @pos: 用作循环计数器的 struct list_head。
 * @n: 另一个 struct list_head 用作临时存储
 * @head: 链表的头节点。
 */
// 此宏用于安全遍历链表，支持在遍历过程中删除链表节点。与 list_for_each 类似，
// 但是在循环中使用了额外的临时变量 n，以便在删除节点后仍然能够正确遍历链表。
#define list_for_each_safe(pos, n, head) \
	for (pos = (head)->next, n = pos->next; pos != (head); \
		pos = n, n = pos->next)

/**
 * list_for_each_entry - 遍历给定类型的链表
 * @pos: 用作循环计数器的指向给定类型的指针。
 * @head: 链表的头节点。
 * @member: 结构体中 list_head 的名称。
 */
// 此宏用于遍历给定类型的链表。通过传递指向链表头节点的指针 head、给定类型的指针 pos，
// 以及结构体中 list_head 的成员名 member，可以在循环中访问链表中的每个节点的宿主结构体。
#define list_for_each_entry(pos, head, member)				\
	for (pos = list_entry((head)->next, typeof(*pos), member),	\
		     prefetch(pos->member.next);			\
	     &pos->member != (head); 					\
	     pos = list_entry(pos->member.next, typeof(*pos), member),	\
		     prefetch(pos->member.next))

#endif