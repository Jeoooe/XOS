#include <xos/list.h>
#include <xos/assert.h>

void list_init(list_t *list) {
    list->tail.next = NULL;
    list->head.prev = NULL;
    list->head.next = &list->tail;
    list->tail.prev = &list->head;
}

//anchor结点前插入node
void list_insert_before(list_node_t *anchor, list_node_t *node) {
    node->prev = anchor->prev;
    node->next = anchor;
    anchor->prev->next = node;
    anchor->prev = node;
}

void list_insert_after(list_node_t *anchor, list_node_t *node) {
    node->prev = anchor;
    node->next = anchor->next;
    anchor->next->prev = node;
    anchor->next = node;
}

void list_push(list_t *list, list_node_t *node) {
    assert(!list_search(list, node));
    list_insert_after(&list->head, node);
}

//弹出头结点后的节点
list_node_t *list_pop(list_t* list) {
    assert(!list_empty(list));
    list_node_t *node = list->head.next;
    list_remove(node);
    return node;
}

void list_pushback(list_t *list, list_node_t *node) {
    assert(!list_search(list, node));
    list_insert_before(&list->tail, node);
}

list_node_t *list_popback(list_t *list) {
    assert(!list_empty(list));
    list_node_t *node = &list->tail.prev;
    list_remove(node);
    return node;
}

bool list_search(list_t *list, list_node_t *node) {
    list_node_t *next = list->head.next;
    while (next != &list->tail) {
        if (next == node) {
            return true;
        }
        next = next->next;
    }
    return false;
}
void list_remove(list_node_t *node) {
    assert(node->prev != NULL);
    assert(node->next != NULL);

    node->prev->next = node->next;
    node->next->prev = node->prev;

    node->next = NULL;
    node->prev = NULL;
}

bool list_empty(list_t *list) {
    return (list->head.next == &list->tail);
}

u32 list_size(list_t *list) {
    list_node_t *next = list->head.next;
    u32 size = 0;
    while (next != &list->tail) {
        size++;
        next = next->next;
    }
    return size;
}