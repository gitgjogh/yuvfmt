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

#ifndef __SIM_LOG_H__
#define __SIM_LOG_H__

#include <stdint.h>
#include <stdarg.h>
#include <stdio.h>

typedef enum log_level {
    SLOG_L_NON  = 0,
    SLOG_L_ADD  = 1,
    SLOG_L_KEY  = 127,
    SLOG_L_ALL  = 128,
} log_level_i;

typedef struct slog_hash_node { 
    struct slog_hash_node *next;
    uint32_t    hash_val;
    int         level;
} slog_node_t;

typedef slog_node_t* slog_head_t;

typedef struct simple_hash_log
{
    /* hash table */
    uint32_t    hash_depth_log2;
    uint32_t    hash_depth;
    slog_head_t*    hash_tbl;
    
    /* storage array*/
    slog_node_t*    node_tbl;
    uint32_t    depth;
    uint32_t    used;
    
    /* configuration */
    FILE        *fp;
    int         level;
    
    /* runtime var */
    slog_node_t*    hash_ret;
} slog_t;

void    slog_cfg(slog_t *kl, FILE *fp, int level);
int     slog_init(slog_t *kl, slog_head_t* hash_tbl, int hash_depth_log2, 
                              slog_node_t* node_tlb, int depth);
void    slog_clear(slog_t *kl);
int     slog_open(slog_t *kl, int hash_depth_log2, int depth);
void    slog_close(slog_t *kl);

slog_node_t* slog_touch_node(slog_t* kl, const char *key, uint32_t key_len,
                        int b_insert, int *b_found);
slog_node_t* slog_get_node(slog_t *kl, const char *key, uint32_t keylen);
slog_node_t* slog_bind(slog_t *kl, const char *key, uint32_t keylen, int level);
int     slog_binds(slog_t *kl, int level, const char *groups);

int     slogv_head(slog_t *kl, const char *fmt, va_list ap);
int     slogv_tail(slog_t *kl, const char *fmt, va_list ap);
int     slog_head(slog_t *kl, const char *fmt, ...);
int     slog_tail(slog_t *kl, const char *fmt, ...);

typedef int (*slog_printf_t)(const char *fmt, ...);
extern  slog_printf_t   xlog;
extern  slog_printf_t   xlog__;
extern  slog_printf_t   xerr;
extern  slog_printf_t   xerr__;

#ifdef _XLOG_
#define XLOG_HASH_TBLSZ_LOG2  (10)
#define XLOG_HASH_TBLSZ       (1<<XLOG_HASH_TBLSZ_LOG2)
#define XLOG_NODE_TBLSZ       (2 *XLOG_HASH_TBLSZ)

extern slog_t *xlog_ptr;

void xinit(int level);
void xlevel(int level);
void xdir(FILE *fp);
slog_node_t* xkey(const char *key);
slog_node_t* xbind(const char *key, int level);
int xbinds(int level, const char *group);

int xlog_head(const char *fmt, ...);
int xlog_tail(const char *fmt, ...);
int xerr_head(const char *fmt, ...);
int xerr_tail(const char *fmt, ...);
#endif  // _XLOG_

#endif  // __SIM_LOG_H__