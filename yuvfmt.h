

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

typedef union _i64_pack {
    uint64_t i64;
    struct {
        uint32_t i32_0;
        uint32_t i32_1;
    };
} i64_pack_t;

enum {
    YUVFMT_420P = 0,
    YUVFMT_420SP,
    YUVFMT_420SPA,
    YUVFMT_422P,
    YUVFMT_422SP,
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
    //char    name[MAX_PATH];
    FILE*   fp;
    int     width;
    int     height;
    int     w_align;
    int     h_align;
    int     yuvfmt;
    int     b10;
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


#endif