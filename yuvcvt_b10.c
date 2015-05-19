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

#include <assert.h>
#include <malloc.h>
#include "yuvdef.h"
#include "yuvcvt.h"


/**
 * unpack 10-bit compact pixels to 16 (low 10-bit)
 *      - i32_0 is used for bitop
 *      - i32_1 is used for reading 
 *
 *   high (10-rbit)|00000000000000|- rbit -| 
 *    bit      high------------------------> low
 *    byte         |  3  |  2  |  1  |  0  | 
 *  
 *                  high (10-rbit)|00000000|- (32-rbit) --|
 *    bit      high------------------------> low
 *  
 */
void b10_linear_unpack_lte(void* b10_base, int n_byte, void* b16_base, int n16)
{
    register i64_pack_t bp;
    
    bp.i64   = 0;
    uint16_t v16;
    int rbit = 0;
    int i32  = 0;
    int n32  = n_byte >> 2;
    int i16  = 0;
    uint32_t* pb10 = (uint32_t*)b10_base;
    uint16_t* pb16 = (uint16_t*)b16_base;

    //assert( (n_byte & 0x08) == 0 );
    //assert( n_byte > (n16*5+3)/4 );
    
    while (1) 
    {
        if (rbit<10) {
            if (i32 < n32 && i16 < n16) {
                bp.i32_1 = pb10[i32++];
            } else {
                break;
            }
            v16  = (bp.i32_0 & 0x3ff);  bp.i64 >>= (32-rbit);   // low   rbit
            v16 += (bp.i32_0 & 0x3ff);  bp.i64 >>= 10;          // high (10-rbit)
            rbit += 22;                                         // rbit=32-(10-rbit)
        } else {
            v16 = (bp.i32_0 & 0x3ff);   bp.i64 >>= 10; 
            rbit -= 10;
        }
        pb16[i16++] = v16;
    }
}

/**
 * pack low 10-bit in 16 to 10-bit compact format
 *
 *                 |xxxxxxxxxxxxxx|- rbit -| 
 *    bit      high------------------------> low
 *    byte         |  3  |  2  |  1  |  0  | 
 *  
 */
void b10_linear_pack_lte(void* b10_base, int n_byte, void* b16_base, int n16)
{
    uint32_t v16 = 0;
    uint32_t v32 = 0;
    int rbit = 0;
    int i32  = 0;
    int n32  = n_byte >> 2;
    int i16  = 0;
    uint32_t* pb10 = (uint32_t*)b10_base;
    uint16_t* pb16 = (uint16_t*)b16_base;
    
    //assert( (n_byte & 0x08) == 0 );
    //assert( n_byte > (n16*5+3)/4 );
    
    while (i32 < n32 && i16 < n16) 
    {
        v16 = pb16[i16++];
        
        if (rbit>=22) {
            v32  += (v16 & 0x3ff) << rbit;                      // low (32-rbit)
            pb10[i32++] = v32;
            v32   = (v16 & 0x3ff) >> (32-rbit);                 // high 10-(32-rbit)
            rbit -= 22;                                         // rbit=10-(32-rbit)
        } else {
            v32  += (v16 & 0x3ff) << rbit;
            rbit += 10;
        }
    }
}

void b10_rect_unpack
(
    int   b_pack,
    void* b10_base, int b10_stride,
    void* b16_base, int b16_stride,
    int   rect_w,   int rect_h
)
{
    int x, y;
    void (*b10_pack_unpack_fp)(void* b10_base, int n_byte, void* b16_base, int n16);
    
    b10_pack_unpack_fp = (b_pack == B16_2_B10) ? b10_linear_pack_lte : b10_linear_unpack_lte;
    
    for (y=0; y<rect_h; ++y) 
    {
        b10_pack_unpack_fp(b10_base, b10_stride, b16_base, rect_w);
        b10_base += b10_stride;
        b16_base += b16_stride;
    }
    
    return;
}

int b10_rect_unpack_mch(yuv_seq_t *rect10, yuv_seq_t *rect16, int b_pack)
{
    int fmt = rect10->yuvfmt;
    
    uint8_t* b10_base   = rect10->pbuf; 
    uint8_t* b16_base   = rect16->pbuf; 
    int b10_stride  = rect10->y_stride; 
    int b16_stride  = rect16->y_stride;
    int w   = rect10->width; 
    int h   = rect10->height; 

    ENTER_FUNC;
    
    assert (rect10->nbit == 10);
    assert (rect16->nbit == 16);
    assert (rect16->nlsb == 10);
    assert (rect10->width == rect16->width);
    assert (rect10->height == rect16->height);
    assert (rect10->yuvfmt == rect16->yuvfmt);
    
    if      (fmt == YUVFMT_400P)
    {
        b10_rect_unpack(b_pack, b10_base, b10_stride, b16_base, b16_stride, w, h);
    }
    else if (fmt == YUVFMT_420P || fmt == YUVFMT_422P)
    {
        b10_rect_unpack(b_pack, b10_base, b10_stride, b16_base, b16_stride, w, h);
        
        b10_base   += rect10->y_size;
        b16_base   += rect16->y_size; 
        b10_stride  = rect10->uv_stride;
        b16_stride  = rect16->uv_stride;
        
        w   = w/2;
        h   = is_mch_422(fmt) ? h : h/2;
        
        b10_rect_unpack(b_pack, b10_base, b10_stride, b16_base, b16_stride, w, h);
        
        b10_base   += rect10->uv_size;
        b16_base   += rect16->uv_size;
        
        b10_rect_unpack(b_pack, b10_base, b10_stride, b16_base, b16_stride, w, h);
    }
    else if (is_semi_planar(fmt))
    {
        b10_rect_unpack(b_pack, b10_base, b10_stride, b16_base, b16_stride, w, h);
        
        b10_base   += rect10->y_size;
        b16_base   += rect16->y_size; 
        b10_stride  = rect10->uv_stride;
        b16_stride  = rect16->uv_stride;
        
        h   = is_mch_422(fmt) ? h : h/2;
        
        b10_rect_unpack(b_pack, b10_base, b10_stride, b16_base, b16_stride, w, h);
    }
    else if (fmt == YUVFMT_UYVY || fmt == YUVFMT_YUYV)
    {
        w   = w*2;
        b10_rect_unpack(b_pack, b10_base, b10_stride, b16_base, b16_stride, w, h);
    }
    
    LEAVE_FUNC;

    return;
}

int b10_tile_unpack
(
    int      b_pack,
    uint8_t* tile10_base, int tw, int th, int tsz, int ts, 
    uint8_t* rect16_base, int w,  int h,  int s
)
{
    int x, y, tx, ty;
    
    assert( (tsz & 7) == 0 );
    assert( ts >= (w*th*5+3)/4 );
    
    #define UNPACK_BUF_SIZE 4094
    uint8_t  unpack_buf[UNPACK_BUF_SIZE];
    uint8_t *unpack_base = unpack_buf;
    int size_needed = tw * th * sizeof(uint16_t);
    if (size_needed > UNPACK_BUF_SIZE)
    {
        unpack_base = (uint8_t*) malloc ( size_needed );
        if (unpack_base==0) {
            printf("%s : malloc fail!\n", __FUNCTION__);
            return 0;
        }
    }
    
    for (ty=0, y=0; y<h; y+=th, ++ty) 
    {
        for (tx=0, x=0; x<w; x+=tw, ++tx) 
        {
            uint8_t* p10 = &tile10_base[ts*ty + tsz*tx];
            uint8_t* p16 = &rect16_base[s * y + sizeof(uint16_t) * x];
            
            if (b_pack==B16_2_B10) {
                b8_linear_2_rect(RECT2LINE, unpack_base, p16, tw*2, th, s);
                b16_rect_transpose(unpack_base, tw, th);
                b10_linear_pack_lte(p10, tsz, unpack_base, tw*th);
            } else {
                b10_linear_unpack_lte(p10, tsz, unpack_base, tw*th);
                b16_rect_transpose(unpack_base, tw, th);
                b8_linear_2_rect(LINE2RECT, unpack_base, p16, tw*2, th, s);
            }
        }
    }
    
    if (size_needed>UNPACK_BUF_SIZE)
    {
        free(unpack_base);
    }
    
    return w*h;
}

int b10_tile_unpack_mch(yuv_seq_t *tile10, yuv_seq_t *rect16, int b_pack)
{
    int fmt = tile10->yuvfmt;

    ENTER_FUNC;
    
    int tw  = tile10->tile.tw;
    int th  = tile10->tile.th;
    int tsz = tile10->tile.tsz;
    int ts  = tile10->y_stride;
    int w   = rect16->width;
    int h   = rect16->height;
    int s   = rect16->y_stride;
    uint8_t *pt = tile10->pbuf;
    uint8_t *pl = rect16->pbuf;
    
    assert (tile10->btile == 1);
    assert (tile10->nbit == 10);
    assert (rect16->nbit == 16);
    assert (rect16->nlsb == 10);
    assert (tile10->width == rect16->width);
    assert (tile10->height == rect16->height);
    assert (tile10->yuvfmt == rect16->yuvfmt);
    
    if      (fmt == YUVFMT_400P)
    {
        b10_tile_unpack(b_pack, pt, tw, th, tsz, ts, pl, w, h, s);
    }
    else if (fmt == YUVFMT_420P || fmt == YUVFMT_422P)
    {
        b10_tile_unpack(b_pack, pt, tw, th, tsz, ts, pl, w, h, s);
        
        ts  = tile10->uv_stride;
        s   = rect16->uv_stride;
        w   = w/2;
        h   = is_mch_422(fmt) ? h : h/2;
        
        pt += tile10->y_size;
        pl += rect16->y_size;
        
        b10_tile_unpack(b_pack, pt, tw, th, tsz, ts, pl, w, h, s);
        
        pt += tile10->uv_size;
        pl += rect16->uv_size;
        
        b10_tile_unpack(b_pack, pt, tw, th, tsz, ts, pl, w, h, s);
    }
    else if (is_semi_planar(fmt))
    {
        b10_tile_unpack(b_pack, pt, tw, th, tsz, ts, pl, w, h, s);
        
        ts  = tile10->uv_stride;
        s   = rect16->uv_stride;
        h   = is_mch_422(fmt) ? h : h/2;

        pt += tile10->y_size;
        pl += rect16->y_size;
        
        b10_tile_unpack(b_pack, pt, tw, th, tsz, ts, pl, w, h, s);
    }
    else if (fmt == YUVFMT_UYVY || fmt == YUVFMT_YUYV)
    {
        w   = w*2;
        b10_tile_unpack(b_pack, pt, tw, th, tsz, ts, pl, w, h, s);
    }
    
    LEAVE_FUNC;

    return 0;
}