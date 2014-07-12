/*
 * Copyright (C) 2014-2015  liangdong <liangdong01@baidu.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

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
