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

#ifndef YUVFMT_H_
#define YUVFMT_H_

#include<stdint.h>
#include<stdio.h>

#ifndef MAX_PATH
#define MAX_PATH 256
#endif

#define W_ALIGN_BIT     3
#define H_ALIGN_BIT     3
#define W_ALIGN_SIZE    (1<<W_ALIGN_BIT)
#define H_ALIGN_SIZE    (1<<H_ALIGN_BIT)

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(a)   (sizeof(a)/sizeof(a[0]))
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

typedef struct _yuv_seq 
{
    int     width;
    int     height;
    int     w_align;
    int     h_align;
    int     yuvfmt;
    int     nlsb;
    int     btile;
    
    tile_t  tile;
    int     y_stride;
    int     uv_stride;
    
    int     y_size;
    int     uv_size;
    int     io_size;
    int     buf_size;
    uint8_t *pbuf;

} yuv_seq_t;

typedef struct _yuv_cfg
{
    int     frame_range[2];

    char*       src_path;
    FILE*       src_fp;
    yuv_seq_t   src;

    char*       dst_path;
    FILE*       dst_fp;
    yuv_seq_t   dst;
    
} yuv_cfg_t;

#endif