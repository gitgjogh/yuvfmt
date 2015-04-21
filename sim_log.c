/*****************************************************************************
 * Copyright 2014 Jeff <ggjogh@gmail.com>
 *****************************************************************************
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
*****************************************************************************/

#include <stdint.h>
#include <string.h>
#include <malloc.h>
#include <stdarg.h>
#include <stdio.h>

#include "sim_log.h"

slog_printf_t   xlog    =   printf;
slog_printf_t   xlog__  =   printf;
slog_printf_t   xerr    =   printf;
slog_printf_t   xerr__  =   printf;

#ifdef _XLOG_
static slog_head_t xlog_hash_tbl[XLOG_HASH_TBLSZ] = {};
static slog_node_t xlog_node_tbl[XLOG_NODE_TBLSZ] = {};

static slog_t  xlog_obj = {
    /* hash table */
    XLOG_HASH_TBLSZ_LOG2,
    XLOG_HASH_TBLSZ,
    xlog_hash_tbl,
    
    /* storage array*/
    xlog_node_tbl,
    XLOG_NODE_TBLSZ,
    0 /*used*/,
    
    /* configuration */
    0 /*stdout*/,
    SLOG_L_ADD,
    
    /* runtime var */
    0 /*hash_ret*/
};
slog_t *xlog_ptr = &xlog_obj;

void xinit(int level)
{
    slog_init(&xlog_obj,
            xlog_hash_tbl, XLOG_HASH_TBLSZ_LOG2, 
            xlog_node_tbl, XLOG_NODE_TBLSZ);

    xlog_obj.level  = level;
    xlog_obj.fp     = stdout;
    xlog_ptr    = &xlog_obj;

    xlog    = xlog_head;
    xlog__  = xlog_tail;
    xerr    = xerr_head;
    xerr__  = xerr_tail;
}
void xlevel(int level)
{
    xlog_obj.level = level;
}
void xdir(FILE *fp)
{
    xlog_obj.fp = fp;
}
slog_node_t* xkey(const char *key)
{
    return slog_bind(&xlog_obj, key, 0, SLOG_L_KEY);
}
slog_node_t* xbind(const char *key, int level)
{
    return slog_bind(&xlog_obj, key, 0, level);
}
int xbinds(int level, const char *group)
{
    return slog_binds(&xlog_obj, level, group);
}
static int xlogv_head(const char *fmt, va_list ap)
{
    return slogv_head(&xlog_obj, fmt, ap);
}
static int xlogv_tail(const char *fmt, va_list ap)
{
    return slogv_tail(&xlog_obj, fmt, ap);
}
int xlog_head(const char *fmt, ...)
{
    int r = 0;
    va_list ap;
    va_start(ap, fmt);
    r = slogv_head(&xlog_obj, fmt, ap);
    va_end(ap);
    return r;
}
int xlog_tail(const char *fmt, ...)
{
    int r = 0;
    va_list ap;
    va_start(ap, fmt);
    r = slogv_tail(&xlog_obj, fmt, ap);
    va_end(ap);
    return r;
}
int xerr_head(const char *fmt, ...)
{
    int r = 0;
    va_list ap;
    va_start(ap, fmt);
    r = fprintf(stderr, "@err>> ");
    r = vfprintf(stderr, fmt, ap);
    va_end(ap);
    return r;
}
int xerr_tail(const char *fmt, ...)
{
    int r = 0;
    va_list ap;
    va_start(ap, fmt);
    r = vfprintf(stderr, fmt, ap);
    va_end(ap);
    return r;
}
#endif  // _XLOG_

void slog_cfg(slog_t *kl, FILE *fp, int level)
{
    kl->fp = fp;
    kl->level = level;
}

void slog_clear(slog_t *kl)
{
    int hash_bufsz = sizeof(slog_head_t) * (1 << kl->hash_depth_log2);
    int node_bufsz = sizeof(slog_node_t) * kl->depth;
    
    kl->used = 0;
    kl->hash_ret = 0;
    memset(kl->hash_tbl, 0, hash_bufsz);
    memset(kl->node_tbl, 0, node_bufsz);
}

int slog_init(slog_t *kl, slog_head_t* hash_tbl, int hash_depth_log2, 
                          slog_node_t* node_tbl, int depth)
{
    memset(kl, 0, sizeof(slog_t));
    
    kl->hash_depth_log2 = hash_depth_log2;
    kl->hash_depth  = (1 << hash_depth_log2) - 1;
    kl->hash_tbl    = hash_tbl;
    
    kl->node_tbl    = node_tbl;
    kl->depth       = depth;
    
    slog_clear(kl);
    
    return 1;
}

int slog_open(slog_t *kl, int hash_depth_log2, int depth)
{
    int hash_tblsz = sizeof(slog_head_t) * (1 << hash_depth_log2);
    int node_tblsz = sizeof(slog_node_t) * depth;
    
    slog_head_t *hash_tbl = malloc(hash_tblsz);
    if (!hash_tbl) {
        xerr("malloc hash_tbl falied\n");
        return 0;
    }
    
    slog_node_t *node_tbl = malloc(node_tblsz);
    if (!hash_tbl) {
        xerr("malloc node_tbl falied\n");
        free(hash_tbl);
        return 0;
    }
    
    slog_init(kl, hash_tbl, hash_depth_log2, node_tbl, depth);
    
    return 1;
}

void slog_close(slog_t *kl) 
{
    if (kl->hash_tbl) {
        free (kl->hash_tbl);
    }
    
    if (kl->node_tbl) {
        free (kl->node_tbl);
    }
}    

static 
uint32_t slog_nstr_time33(const uint8_t *key, uint32_t keylen)
{
    uint32_t c = 0;
    uint32_t i, h;
    for (i=h=0; (c=key[i]) && (i<keylen); ++i) {
        h = h * 33u + c; 
    } 

    return h; 
}

static 
uint32_t slog_cstr_time33(const uint8_t *key, uint32_t *keylen)
{
    uint32_t c = 0;
    uint32_t i, h;
    for (i=h=0; (c=key[i]); ++i) {
        h = h * 33 + c; 
    } 

    if (keylen) { 
        *keylen = i;
    }

    return h; 
}

static uint32_t slog_get_keylen(const char *str) 
{ 
    int c = 0, i = 0;
    if (str && (c=str[0]) && c == '@' ) {
        for (++str, i = 0; (c=str[i]) && c != '>'; ++i) {
        }
    }
    return i; 
}

slog_node_t* slog_touch_node(slog_t* kl, const char *key, uint32_t keylen,
                        int b_insert, int *b_found)
{
    uint32_t hash = keylen ? slog_nstr_time33(key, keylen)
                           : slog_cstr_time33(key, &keylen);

    slog_node_t* head = kl->hash_tbl[hash & kl->hash_depth];
    slog_node_t *node = 0;

    for (node = head; node; node = node->next) {
        if (node->hash_val==hash/* && strncmp(node->key, key, keylen)==0 */) {
            b_found ? (*b_found = 1) : 0;
            return node;
        }
    }

    if (node==0 && b_insert) 
    {
        b_found ? (*b_found = 0) : 0;
        
        if (kl->used >= kl->depth) {
            xerr("slog table overflow!\n");
            return 0;
        }
        
        int i;
        /* search one empty entry (hash_val==0) */
        for (i=kl->used; i<kl->depth; ++i) {
            if (!kl->node_tbl[i].hash_val) {
                break;
            }
        }
        if (i >= kl->depth) {
            for (i=0; i<kl->used; ++i) {
                if (!kl->node_tbl[i].hash_val) {
                    break;
                }
            }
        }
        
        slog_node_t* tail = &kl->node_tbl[i];
        tail->hash_val = hash;
        //tail->next = 0;
        kl->used += 1;
        
        if (node) {
            node->next = tail;
        } else {
            kl->hash_tbl[hash & kl->hash_depth] = tail;
        }
        return tail;
    }

    return 0;
}

slog_node_t* slog_get_node(slog_t *kl, const char *key, uint32_t keylen)
{
    kl->hash_ret = slog_touch_node(kl, key, keylen, 0, 0);
    return kl->hash_ret;
}

slog_node_t* slog_bind(slog_t *kl, const char *key, uint32_t keylen, int level)
{
    slog_node_t* node = slog_touch_node(kl, key, keylen, 1, 0);
    node ? (node->level = level) : 0;
    return node;
}

uint32_t get_token_pos(const char* str, uint32_t search_from,
                       const char* prejumpset,
                       const char* delemiters,
                       uint32_t *stoken_start)
{
    uint32_t c, start, end;

    if (prejumpset) 
    {
        for (start = search_from; (c = str[start]); ++start) {
            if ( !strchr(prejumpset, c) ) {
                break;
            }
        }
    }

    if (stoken_start) {
        *stoken_start = start;
    }

    for (end = start; (c = str[end]); ++end) {
        if ( delemiters && strchr(delemiters, c) ) {
            break;
        }
    }

    return end - start;
}

int slog_binds(slog_t *kl, int level, const char *group)
{
    slog_node_t* node = 0;
    int nkey=0, keylen=0, pos=0;
    keylen = get_token_pos(group, 0, " ", " ,", &pos);
    while (keylen) {
        node = slog_bind(kl, &group[pos], keylen, level);
        if (node) {
            ++ nkey;
        } else {
            break;
        }
        keylen = get_token_pos(group, pos+keylen, " ,", " ,", &pos);
    }
    return nkey;
}

static int slog_group_assert(slog_t *kl, int b_head, const char *key, uint32_t keylen)
{
    if (kl->level >= SLOG_L_KEY) {
        return 1;
    }
    
    slog_node_t* node = b_head  ? (keylen>0 ? slog_get_node(kl, key, keylen) : 0) 
                            : (kl->hash_ret);
    
    return node ? (node->level + kl->level) > SLOG_L_KEY : 0;
}

static int slog_head_assert(slog_t *kl, const char *key, uint32_t keylen)
{
    slog_group_assert(kl, 1, key, keylen);
}

static int slog_tail_assert(slog_t *kl)
{
    slog_group_assert(kl, 0, 0, 0);
}

static int slogv_group(slog_t *kl, int b_head, const char *fmt, va_list ap)
{
    if (kl->level == SLOG_L_NON) {
        return 0;
    } else
    if (kl->level >= SLOG_L_ALL) {
        return vfprintf(kl->fp, fmt, ap);
    }

    int keylen = b_head ? slog_get_keylen(fmt) : 0;
    if (slog_group_assert(kl, b_head, fmt+1, keylen)) {
        return vfprintf(kl->fp, fmt, ap);
    }

    return 0;
}

int slogv_head(slog_t *kl, const char *fmt, va_list ap)
{
    return slogv_group(kl, 1, fmt, ap);
}
int slogv_tail(slog_t *kl, const char *fmt, va_list ap)
{
    return slogv_group(kl, 0, fmt, ap);
}
int slog_head(slog_t *kl, const char *fmt, ...)
{
    int r = 0;
    va_list ap;
    va_start(ap, fmt);
    r = slogv_head(kl, fmt, ap);
    va_end(ap);
    return r;
}
int slog_tail(slog_t *kl, const char *fmt, ...)
{
    int r = 0;
    va_list ap;
    va_start(ap, fmt);
    r = slogv_tail(kl, fmt, ap);
    va_end(ap);
    return r;
}
