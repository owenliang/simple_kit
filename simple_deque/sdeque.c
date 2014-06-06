/***************************************************************************
 * 
 * Copyright (c) 2014 Baidu.com, Inc. All Rights Reserved
 * $Id$ 
 * 
 **************************************************************************/
 
 /**
 * @file sdeque.c
 * @author liangdong(liangdong01@baidu.com)
 * @date 2014/05/27 20:33:03
 * @version $Revision$ 
 * @brief 
 *  
 **/

#include <stdlib.h>
#include <string.h>
#include "sdeque.h"

struct sdeque *sdeque_new()
{
    struct sdeque *sdeque = calloc(1, sizeof(*sdeque));
    return sdeque;
}

void sdeque_free(struct sdeque *sdeque)
{
    struct sdeque_node *node = sdeque->head;
    while (node) {
        struct sdeque_node *next = node->next;
        free(node);
        node = next;
    }
    free(sdeque);
}

void sdeque_push_front(struct sdeque *sdeque, void *value)
{
    struct sdeque_node *node = calloc(1, sizeof(*node));
    node->value = value;
    node->next = sdeque->head;
    if (sdeque->head)
        sdeque->head->prev = node;
    else
        sdeque->tail = node;
    sdeque->head = node;
    ++sdeque->num_nodes;
}

int sdeque_pop_front(struct sdeque *sdeque)
{
    if (!sdeque->head)
        return -1;
    struct sdeque_node *node = sdeque->head;
    sdeque->head = sdeque->head->next;
    if (sdeque->head)
        sdeque->head->prev = NULL;
    else
        sdeque->tail = NULL;
    free(node);
    --sdeque->num_nodes;
    return 0;
}

int sdeque_front(struct sdeque *sdeque, void **value)
{
    if (!sdeque->head)
        return -1;
    if (value)
        *value = sdeque->head->value;
    return 0;
}

void sdeque_push_back(struct sdeque *sdeque, void *value)
{
    struct sdeque_node *node = calloc(1, sizeof(*node));
    node->value = value;
    node->prev = sdeque->tail;
    if (!sdeque->tail) {
        sdeque->head = node;
    } else {
        sdeque->tail->next = node;
    }
    sdeque->tail = node;
    ++sdeque->num_nodes;
}

int sdeque_pop_back(struct sdeque *sdeque)
{
    if (!sdeque->tail)
        return -1;
    struct sdeque_node *node = sdeque->tail;
    sdeque->tail = node->prev;
    if (sdeque->tail) {
        sdeque->tail->next = NULL;
    } else {
        sdeque->head = NULL;
    }
    free(node);
    --sdeque->num_nodes;
    return 0;
}

int sdeque_back(struct sdeque *sdeque, void **value)
{
    if (!sdeque->tail)
        return -1;
    if (value)
        *value = sdeque->tail->value;
    return 0;
}

uint64_t sdeque_size(struct sdeque *sdeque)
{
    return sdeque->num_nodes;
}

/* vim: set ts=4 sw=4 sts=4 tw=100 */
