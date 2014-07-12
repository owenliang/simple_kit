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

#include <stdio.h>
#include <assert.h>
#include "sdeque.h"

void check_back_api(struct sdeque *sdeque)
{
    long i;
    for (i = 0; i < 1024; ++i)
        sdeque_push_back(sdeque, (void *)i);
    assert(sdeque->head->value == (void *)0);
    assert(sdeque->tail->value == (void *)1023);
    assert(sdeque_size(sdeque) == 1024);

    void *value;
    while (sdeque_back(sdeque, &value) != -1) {
        printf("value=%ld\n", (long)value);
        assert(sdeque_pop_back(sdeque) == 0);
    }
    assert(sdeque_size(sdeque) == 0);
    assert(sdeque_pop_back(sdeque) == -1);
    assert(sdeque->head == sdeque->tail && sdeque->head == NULL);
}

void check_front_api(struct sdeque *sdeque)
{
    long i;
    for (i = 0; i < 1024; ++i)
        sdeque_push_front(sdeque, (void *)i);
    assert(sdeque->head->value == (void *)1023);
    assert(sdeque->tail->value == (void *)0);
    assert(sdeque_size(sdeque) == 1024);

    void *value;
    while (sdeque_front(sdeque, &value) != -1) {
        printf("value=%ld\n", (long)value);
        assert(sdeque_pop_front(sdeque) == 0);
    }
    assert(sdeque_size(sdeque) == 0);
    assert(sdeque_pop_front(sdeque) == -1);
    assert(sdeque->head == sdeque->tail && sdeque->head == NULL);
}

void check_mixed_api(struct sdeque *sdeque)
{
    sdeque_push_front(sdeque, (void *)1);
    assert(sdeque_size(sdeque) == 1);
    assert(sdeque->head == sdeque->tail && sdeque->head->value == (void *)1);
    void *value = NULL;
    assert(sdeque_front(sdeque, &value) == 0 && value == (void *)1);
    value = NULL;
    assert(sdeque_back(sdeque, &value) == 0 && value == (void *)1);
    assert(sdeque_pop_back(sdeque) == 0);
    assert(sdeque->head == sdeque->tail  && sdeque->head == NULL);
    assert(sdeque_size(sdeque) == 0);

    sdeque_push_back(sdeque, (void *)1);
    assert(sdeque_size(sdeque) == 1);
    assert(sdeque->head == sdeque->tail && sdeque->head->value == (void *)1);
    value = NULL;
    assert(sdeque_front(sdeque, &value) == 0 && value == (void *)1);
    value = NULL;
    assert(sdeque_back(sdeque, &value) == 0 && value == (void *)1);
    assert(sdeque_pop_front(sdeque) == 0);
    assert(sdeque->head == sdeque->tail  && sdeque->head == NULL);
    assert(sdeque_size(sdeque) == 0);
}

int main(int argc, char **argv)
{
    struct sdeque *sdeque = sdeque_new();

    check_back_api(sdeque);
    check_front_api(sdeque);
    check_mixed_api(sdeque);

    sdeque_free(sdeque);
    return 0;
}

/* vim: set ts=4 sw=4 sts=4 tw=100 */
