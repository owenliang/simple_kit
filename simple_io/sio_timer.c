/***************************************************************************
 * 
 * Copyright (c) 2014 Baidu.com, Inc. All Rights Reserved
 * $Id$ 
 * 
 **************************************************************************/
 
 /**
 * @file sio_timer.c
 * @author liangdong(liangdong01@baidu.com)
 * @date 2014/03/31 09:54:39
 * @version $Revision$ 
 * @brief 
 *  
 **/

#include <stdlib.h>
#include "sio_timer.h"

#define PARENT(timer) (timer->index  >> 1)
#define LEFT(timer)   (timer->index  << 1)
#define RIGHT(timer)  ((timer->index << 1) + 1)

struct sio_timer_manager *sio_timer_new()
{
    return calloc(1, sizeof(struct sio_timer_manager));
}

void sio_timer_free(struct sio_timer_manager *st_mgr)
{
    free(st_mgr->heap_nodes);
    free(st_mgr);
}

static void _sio_timer_upheap(struct sio_timer_manager *st_mgr, struct sio_timer *timer)
{
    uint64_t parent = PARENT(timer);

    while (parent) {
        if (timer->expire < st_mgr->heap_nodes[parent]->expire) {
            st_mgr->heap_nodes[parent]->index = timer->index;
            st_mgr->heap_nodes[timer->index] = st_mgr->heap_nodes[parent];
            timer->index = parent;
        } else {
            break;
        }
        parent = PARENT(timer);
    }
    
    st_mgr->heap_nodes[timer->index] = timer;
}

static void _sio_timer_downheap(struct sio_timer_manager *st_mgr, struct sio_timer *timer)
{
    for (;;) {
        uint64_t left = LEFT(timer);
        uint64_t right = RIGHT(timer);
        uint64_t min_expire = timer->expire;
        uint64_t min_index = timer->index;

        if (left <= st_mgr->heap_length && st_mgr->heap_nodes[left]->expire < min_expire) { 
            min_index = left;
            min_expire = st_mgr->heap_nodes[left]->expire;
        }
        if (right <= st_mgr->heap_length && st_mgr->heap_nodes[right]->expire < min_expire) {
            min_index = right;
        }
        if (min_index != timer->index) {
            st_mgr->heap_nodes[min_index]->index = timer->index;
            st_mgr->heap_nodes[timer->index] = st_mgr->heap_nodes[min_index];
            timer->index = min_index;
        } else {
            break;
        }
    }
    st_mgr->heap_nodes[timer->index] = timer;
}

static void _sio_timer_adjust_heap(struct sio_timer_manager *st_mgr, struct sio_timer *timer)
{
    uint64_t parent = PARENT(timer);
    
    if (parent && timer->expire < st_mgr->heap_nodes[parent]->expire) 
        _sio_timer_upheap(st_mgr, timer);
    else
        _sio_timer_downheap(st_mgr, timer);
}

void sio_timer_insert(struct sio_timer_manager *st_mgr, struct sio_timer *timer)
{
    if (st_mgr->heap_length >= st_mgr->heap_size) {
        uint64_t new_size = st_mgr->heap_size + 1;
        
        struct sio_timer **new_nodes = (struct sio_timer **)realloc(st_mgr->heap_nodes, 
                (new_size + 1) * sizeof(*new_nodes));
        st_mgr->heap_nodes = new_nodes;
        st_mgr->heap_size++;
    }
    timer->index = ++st_mgr->heap_length;
    _sio_timer_adjust_heap(st_mgr, timer);    
}

void sio_timer_remove(struct sio_timer_manager *st_mgr, struct sio_timer *timer)
{
    if (timer->index == st_mgr->heap_length)
        st_mgr->heap_length--;
    else {
        st_mgr->heap_nodes[timer->index] = st_mgr->heap_nodes[st_mgr->heap_length--];
        st_mgr->heap_nodes[timer->index]->index = timer->index;
        _sio_timer_adjust_heap(st_mgr, st_mgr->heap_nodes[timer->index]);
    }
}

void sio_timer_modify(struct sio_timer_manager *st_mgr, struct sio_timer *timer)
{
    _sio_timer_adjust_heap(st_mgr, timer);
}

struct sio_timer *sio_timer_pop(struct sio_timer_manager *st_mgr)
{
    if (!st_mgr->heap_length)
        return NULL;

    struct sio_timer *top = st_mgr->heap_nodes[1];
    
    if (st_mgr->heap_length == 1) {
        st_mgr->heap_length = 0;
    } else {
        st_mgr->heap_nodes[1] = st_mgr->heap_nodes[st_mgr->heap_length--];
        st_mgr->heap_nodes[1]->index = 1;
        _sio_timer_adjust_heap(st_mgr, st_mgr->heap_nodes[1]);
    }
    return top;
}

struct sio_timer *sio_timer_top(struct sio_timer_manager *st_mgr)
{
    if (!st_mgr->heap_length)
        return NULL;
    return st_mgr->heap_nodes[1];
}

uint64_t sio_timer_size(struct sio_timer_manager *st_mgr)
{
    return st_mgr->heap_length;
}

/* vim: set ts=4 sw=4 sts=4 tw=100 */
