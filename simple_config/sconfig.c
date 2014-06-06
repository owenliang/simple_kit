/***************************************************************************
 * 
 * Copyright (c) 2014 Baidu.com, Inc. All Rights Reserved
 * $Id$ 
 * 
 **************************************************************************/
 
 /**
 * @file sconfig.c
 * @author liangdong(liangdong01@baidu.com)
 * @date 2014/05/31 09:44:33
 * @version $Revision$ 
 * @brief 
 *  
 **/

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "slist.h"
#include "sconfig.h"

static void _sconfig_free_items(struct slist *items)
{
    void *value;
    while (slist_front(items, NULL, NULL, &value) != -1) {
        free(value);
        slist_pop_front(items);
    }
    slist_free(items);
}

static int _sconfig_parse_line(char *buffer, int len, char **name, char **value)
{
    int i;
    /* find name's begin */
    for (i = 0; i < len; ++i) {
        if (buffer[i] == '=' || buffer[i] == '_' || isdigit(buffer[i]))
            return -1; /* key is empty or key starts with _ or 0-9 */
        if (!isspace(buffer[i]))
            break;
    }
    if (i == len || buffer[i] == '#')
        return 0;
    *name = buffer + i;

    /* find name's end */
    int has_alpha = 0;
    for ( ; i < len; ++i) {
        if (buffer[i] == '=')
            break;
        /* only 0-9, a-z, A-Z, _ is allowed */
        else if (isupper(buffer[i]) || islower(buffer[i])) {
            has_alpha = 1;
            continue;
        } else if (isdigit(buffer[i]) || buffer[i] == '_')
            continue;
        else
            return -1;
    }
    if (i == len || !has_alpha)
        return -1;    /* = is not found or no alpha */
    buffer[i++] = '\0';
    if (i == len) {
        *value = buffer + i;
        return 1; /* name with empty value */
    }

    /* find value's end */
    *value = buffer + i;
    int last_char = -1;
    for ( ; i < len; ++i) {
        if (buffer[i] == '#' && buffer + i != *value && isspace(buffer[i - 1]))
            break; /* annotation occurs */
        else if (!isspace(buffer[i]))
            last_char = i;
    }
    if (last_char == -1) {
        **value = '\0';
        return 1; /* name with empty value */
    }
    if (isspace(**value))
        return -1; /* blank chars after = */
    buffer[last_char + 1] = '\0';
    return 2; /* name with value */
}

static int _sconfig_load(const char *filename, struct slist *items)
{
    FILE *fp = fopen(filename, "r");
    if (!fp)
        return -1;

    char *name, *value;
    char buffer[4096];
    while (fgets(buffer, sizeof(buffer), fp)) {
        int len = strlen(buffer);
        if (buffer[len - 1] != '\n') /* line too long */
            break;
        buffer[--len] = '\0';
        int ret = _sconfig_parse_line(buffer, len, &name, &value);
        if (ret == -1)
            break; /* parse fail */
        else if (ret == 0)
            continue; /* no token found */
        char *dup_value = strdup(value);
        if (slist_insert(items, name, strlen(name) + 1, dup_value) == -1) {
            /* duplacate name occurs */
            free(dup_value);
            break;
        }
    }
    int eof = feof(fp);
    fclose(fp);
    return eof ? 0 : -1;
}

struct sconfig *sconfig_new(const char *filename)
{
    struct slist *items = slist_new(32);
    if (_sconfig_load(filename, items) == -1) {
        _sconfig_free_items(items);
        return NULL;
    }
    struct sconfig *sconf = calloc(1, sizeof(*sconf));
    sconf->filename = strdup(filename);
    sconf->items = items;
    return sconf;
}

void sconfig_free(struct sconfig *sconf)
{
    free(sconf->filename);
    _sconfig_free_items(sconf->items);
    free(sconf);
}

int sconfig_reload(struct sconfig *sconf)
{
    struct slist *items = slist_new(32);
    if (_sconfig_load(sconf->filename, items) == -1) {
        _sconfig_free_items(items);
        return -1;
    }
    _sconfig_free_items(sconf->items);
    sconf->items = items;
    return 0;
}

int sconfig_read_str(struct sconfig *sconf, const char *name, const char **value)
{
    void *item;
    if (slist_find(sconf->items, name, strlen(name) + 1, &item) == -1)
        return -1;
    *value = (const char *)item;
    return 0;
}

int sconfig_read_int32(struct sconfig *sconf, const char *name, int32_t *value)
{
    void *item;
    if (slist_find(sconf->items, name, strlen(name) + 1, &item) == -1)
        return -1;
    *value = strtol((const char *)item, NULL, 10);
    return 0;
}

int sconfig_read_uint32(struct sconfig *sconf, const char *name, uint32_t *value)
{
    void *item;
    if (slist_find(sconf->items, name, strlen(name) + 1, &item) == -1)
        return -1;
    *value = strtoul((const char *)item, NULL, 10);
    return 0;
}

int sconfig_read_int64(struct sconfig *sconf, const char *name, int64_t *value)
{
    void *item;
    if (slist_find(sconf->items, name, strlen(name) + 1, &item) == -1)
        return -1;
    *value = strtoll((const char *)item, NULL, 10);
    return 0;
}

int sconfig_read_uint64(struct sconfig *sconf, const char *name, uint64_t *value)
{
    void *item;
    if (slist_find(sconf->items, name, strlen(name) + 1, &item) == -1)
        return -1;
    *value = strtoull((const char *)item, NULL, 10);
    return 0;
}

int sconfig_read_double(struct sconfig *sconf, const char *name, double *value)
{
    void *item;
    if (slist_find(sconf->items, name, strlen(name) + 1, &item) == -1)
        return -1;
    *value = strtod((const char *)item, NULL);
    return 0;
}

void sconfig_begin_iterate(struct sconfig *sconf)
{
    slist_begin_iterate(sconf->items);
}

int sconfig_iterate(struct sconfig *sconf, const char **name, const char **value)
{
    void *v;
    if (slist_iterate(sconf->items, name, NULL, &v) == -1)
        return -1;
    if (value)
        *value = (const char *)v;
    return 0;
}

void sconfig_end_iterate(struct sconfig *sconf)
{
    slist_end_iterate(sconf->items);
}

/* vim: set ts=4 sw=4 sts=4 tw=100 */
