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
#include "yuvdef.h"
#include "yuvcvt.h"


void b8_linear_2_rect_align_0(uint8_t* line, uint8_t* rect, int w, int h, int s)
{
    int i, j;
    for (j=0; j<h; ++j) {
        for (i=0; i<w; ++i) {
            *((uint8_t*)(rect+i)) = *((uint8_t*)(line+i));
        }
        line += w;
        rect += s;
    }
}

void b8_linear_2_rect_align_4(uint8_t* line, uint8_t* rect, int w, int h, int s)
{
    int i, j;
    for (j=0; j<h; ++j) {
        for (i=0; i<w; i+=4) {
            *((uint32_t*)(rect+i)) = *((uint32_t*)(line+i));
        }
        line += w;
        rect += s;
    }
}

void b8_linear_2_rect_align_8(uint8_t* line, uint8_t* rect, int w, int h, int s)
{
    int i, j;
    for (j=0; j<h; ++j) {
        for (i=0; i<w; i+=8) {
            *((uint64_t*)(rect+i)) = *((uint64_t*)(line+i));
        }
        line += w;
        rect += s;
    }
}

void b8_linear_2_rect_width_8(uint8_t* line, uint8_t* rect, int w, int h, int s)
{
    int i, j;
    for (j=0; j<h; ++j) {
        *((uint64_t*)(rect)) = *((uint64_t*)(line));
        line += w;
        rect += s;
    }
}


void b8_rect_2_linear_align_0(uint8_t* line, uint8_t* rect, int w, int h, int s)
{
    int i, j;
    for (j=0; j<h; ++j) {
        for (i=0; i<w; ++i) {
            *((uint8_t*)(line+i)) = *((uint8_t*)(rect+i));
        }
        line += w;
        rect += s;
    }
}

void b8_rect_2_linear_align_4(uint8_t* line, uint8_t* rect, int w, int h, int s)
{
    int i, j;
    for (j=0; j<h; ++j) {
        for (i=0; i<w; i+=4) {
            *((uint32_t*)(line+i)) = *((uint32_t*)(rect+i));
        }
        line += w;
        rect += s;
    }
}

void b8_rect_2_linear_align_8(uint8_t* line, uint8_t* rect, int w, int h, int s)
{
    int i, j;
    for (j=0; j<h; ++j) {
        for (i=0; i<w; i+=8) {
            *((uint64_t*)(line+i)) = *((uint64_t*)(rect+i));
        }
        line += w;
        rect += s;
    }
}

void b8_rect_2_linear_width_8(uint8_t* line, uint8_t* rect, int w, int h, int s)
{
    int i, j;
    for (j=0; j<h; ++j) {
        *((uint64_t*)(line)) = *((uint64_t*)(rect));
        line += w;
        rect += s;
    }
}

void b8_linear_2_rect(int dir, uint8_t* line, uint8_t* rect, int w, int h, int s)
{
    void (*map_func_p)(uint8_t* line, uint8_t* rect, int w, int h, int s);
    int b_l2r = (dir==LINE2RECT);
    
    if        ( ((int)line&3) || (w&3) || ((int)rect&3) || (s&3) ) {
        map_func_p = b_l2r ? b8_linear_2_rect_align_0 : b8_rect_2_linear_align_0;
    } else if ( ((int)line&7) || (w&7) || ((int)rect&7) || (s&7) ) {
        map_func_p = b_l2r ? b8_linear_2_rect_align_4 : b8_rect_2_linear_align_4;
    } else if ( w == 8 )  {
        map_func_p = b_l2r ? b8_linear_2_rect_width_8 : b8_rect_2_linear_width_8; 
    } else {
        map_func_p = b_l2r ? b8_linear_2_rect_align_8 : b8_rect_2_linear_align_8;
    }
    
    map_func_p(line, rect, w, h, s);
}

void b8_tile_2_rect
(
    int b_t2r, 
    uint8_t* pt, int tw, int th, int tsz, int ts, 
    uint8_t* pl, int w,  int h,  int s
)
{
    int x, y, tx, ty;
    void (*map_func_p)(uint8_t* line, uint8_t* rect, int w, int h, int s);
    int b_l2r = (b_t2r == TILE2RECT);

    if        ( ((int)pt&3) || (tw&3) || (tsz&3) || (ts&3) || ((int)pl&3) || (s&3) ) {
        map_func_p = b_l2r ? b8_linear_2_rect_align_0 : b8_rect_2_linear_align_0;
    } else if ( ((int)pl&7) || (tw&7) || (tsz&7) || (ts&7) || ((int)pl&7) || (s&7) ) {
        map_func_p = b_l2r ? b8_linear_2_rect_align_4 : b8_rect_2_linear_align_4;
    } else if ( tw == 8 ) {
        map_func_p = b_l2r ? b8_linear_2_rect_width_8 : b8_rect_2_linear_width_8;  
    } else {
        map_func_p = b_l2r ? b8_linear_2_rect_align_8 : b8_rect_2_linear_align_8;    
    } 
    
    for (ty=0, y=0; y<h; y+=th, ++ty) 
    {
        for (tx=0, x=0; x<w; x+=tw, ++tx) 
        {
            uint8_t* src = &pt[ts*ty + tsz*tx];
            uint8_t* dst = &pl[s * y + x];

            map_func_p(src, dst, tw, th, s);
        }
    } 
    
    return;
}

void b8_tile_2_mch(yuv_seq_t *tile, yuv_seq_t *rect, int b_t2r)
{
    int fmt = tile->yuvfmt;
    
    int tw  = tile->tile.tw;
    int th  = tile->tile.th;
    int tsz = tile->tile.tsz;
    int ts  = tile->y_stride;
    int w   = rect->width;
    int h   = rect->height;
    int s   = rect->y_stride;
    uint8_t *pt = tile->pbuf;
    uint8_t *pl = rect->pbuf;
    
    ENTER_FUNC();
    
    assert (tile->nbit == 8);
    assert (rect->nbit == 8);
    assert (tile->width == rect->width);
    assert (tile->height == rect->height);
    
    if (fmt == YUVFMT_400P) {
        b8_tile_2_rect(b_t2r, pt, tw, th, tsz, ts, pl, w, h, s);
    }
    else if (fmt == YUVFMT_420P || fmt == YUVFMT_422P)
    {
        b8_tile_2_rect(b_t2r, pt, tw, th, tsz, ts, pl, w, h, s);
        
        ts  = tile->uv_stride;
        s   = rect->uv_stride;
        w   = w/2;
        h   = is_mch_422(fmt) ? h : h/2;
        
        pt += tile->y_size;
        pl += rect->y_size;
        
        b8_tile_2_rect(b_t2r, pt, tw, th, tsz, ts, pl, w, h, s);
        
        pt += tile->uv_size;
        pl += rect->uv_size;
        
        b8_tile_2_rect(b_t2r, pt, tw, th, tsz, ts, pl, w, h, s);
    }
    else if (is_semi_planar(fmt))
    {
        b8_tile_2_rect(b_t2r, pt, tw, th, tsz, ts, pl, w, h, s);
        
        ts  = tile->uv_stride;
        s   = rect->uv_stride;
        h   = is_mch_422(fmt) ? h : h/2;

        pt += tile->y_size;
        pl += rect->y_size;
        
        b8_tile_2_rect(b_t2r, pt, tw, th, tsz, ts, pl, w, h, s);
    }
    else if (fmt == YUVFMT_UYVY || fmt == YUVFMT_YUYV)
    {
        w   = w*2;
        b8_tile_2_rect(b_t2r, pt, tw, th, tsz, ts, pl, w, h, s);
    }
    
    LEAVE_FUNC();

    return;
}

