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

#ifndef __YUVDEF_H__
#define __YUVDEF_H__


#include <stdint.h>
#include "sim_opt.h"


#ifndef ENTER_FUNC
#define ENTER_FUNC()    do{} while(0)
#endif

#ifndef LEAVE_FUNC
#define LEAVE_FUNC()    do{} while(0)
#endif

typedef union _i64_pack {
    uint64_t i64;
    struct {
        uint32_t i32_0;
        uint32_t i32_1;
    };
} i64_pack_t;

enum {
    YUVFMT_UNKNOWN,
    YUVFMT_400P=0,
    YUVFMT_420P,
    YUVFMT_420SP,
    YUVFMT_420SPA,
    YUVFMT_422P,
    YUVFMT_422SP,
    YUVFMT_422SPA,
    YUVFMT_UYVY,
    YUVFMT_YUYV,
    YUVFMT_UNSUPPORT
};

/**
 *     tw
 *  ---^---
 *  x x x x  |
 *  x x x x  > th
 *  x x x x  |
 *  
 */
typedef struct _tile 
{
    int     tw;
    int     th;
    int     tsz;        //!< tsz >= (tw*th); 
    
    /**
     *                                tw
     *                             ---^---
     *  x x x x  x x x x  x x x x  x x x x  |
     *  x x x x  x x x x  x x x x  x x x x  > th
     *  x x x x  x x x x  x x x x  x x x x  |
     *  x x x x    <== padding_bytes
     *  
     */
    //int     tile_row_size;         //!< tile_row_size = tsz * N + padding_bytes;

} tile_t;

typedef struct _rect
{
    union {
        struct  {int x,y,w,h;};
        int     v[4];
    };
} rect_t;

typedef struct _yuv_seq 
{
    int     width;
    int     height;
    int     yuvfmt;
    int     nbit;
    int     nlsb;
    int     btile;
    
    tile_t  tile;
    int     y_stride;       //!< cfg-able
    int     uv_stride;      //!< un-cfg-able, y_stride or y_stride/2
    
    int     y_size;         //!< un-cfg-able, y_stride * y_height
    int     uv_size;        //!< un-cfg-able, uv_stride * uv_height
    int     io_size;        //!< cfg-able
    int     buf_size;
    uint8_t *pbuf;

    rect_t  roi;

} yuv_seq_t;


int is_mch_420(int fmt);
int is_mch_422(int fmt);
int is_mch_mixed(int fmt);
int is_mch_planar(int fmt);
int is_semi_planar(int fmt);
int is_mono_planar(int fmt);
int get_spl_fmt(int fmt);

void swap_uv(uint8_t **u, uint8_t **v);
int get_uv_width(yuv_seq_t *yuv);
int get_uv_height(yuv_seq_t *yuv);
int get_uv_ds_ratio_w(int fmt);
int get_uv_ds_ratio_h(int fmt);

void set_yuv_prop(yuv_seq_t *yuv, int b_realloc, int w, int h, int fmt, 
                    int nbit, int nlsb, int btile, 
                    int stride, int io_size);
                    
void set_yuv_prop_by_copy(yuv_seq_t *dst, int b_realloc, yuv_seq_t *src);
void show_yuv_prop(yuv_seq_t *yuv, int level, const char *prompt);
int  yuv_buf_realloc(yuv_seq_t *yuv, int buf_size);
void yuv_buf_free(yuv_seq_t *yuv);


#endif  // __YUVDEF_H__
