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
#include <stdio.h>
#include "yuvdef.h"


int sat_div(int num, int den)
{
    return (num + den - 1)/den;
}


int bit_sat(int nbit, int val)
{
    int pad = (1<<nbit) - 1;
    return (val+pad) & (~pad);
}


int is_bit_aligned(int nbit, int val)
{
    int pad = (1<<nbit) - 1;
    return (val&pad)==0;
}


int is_mch_420(int fmt)
{
    return (fmt == YUVFMT_420P
            || fmt == YUVFMT_420SP
            || fmt == YUVFMT_420SPA) ? 1 : 0;
}


int is_mch_422(int fmt)
{
    return (fmt == YUVFMT_422P
            || fmt == YUVFMT_422SP
            || fmt == YUVFMT_422SPA
            || fmt == YUVFMT_UYVY
            || fmt == YUVFMT_YUYV) ? 1 : 0;
}

 
int is_mch_mixed(int fmt)
{
    return (fmt == YUVFMT_UYVY 
            || fmt == YUVFMT_YUYV) ? 1 : 0;
}


int is_mch_planar(int fmt)
{
    return (fmt == YUVFMT_420P 
            || fmt == YUVFMT_422P) ? 1 : 0;
}


int is_semi_planar(int fmt)
{
    return (fmt == YUVFMT_420SP
            || fmt == YUVFMT_420SPA
            || fmt == YUVFMT_422SP
            || fmt == YUVFMT_422SPA) ? 1 : 0;
}


int is_mono_planar(int fmt)
{
    return (fmt == YUVFMT_400P) ? 1 : 0;
}

int get_spl_fmt(int fmt)
{
    if (fmt == YUVFMT_400P)     return YUVFMT_400P; 
    else if (is_mch_420(fmt))   return YUVFMT_420P;
    else if (is_mch_422(fmt))   return YUVFMT_422P;
    else                        return YUVFMT_UNSUPPORT;
}

void swap_uv(uint8_t **u, uint8_t **v)
{
    uint8_t *m = *u;
    *u = *v;
    *v = m;
}

int get_uv_width(yuv_seq_t *yuv)
{
    int fmt = yuv->yuvfmt;
    
    if       (is_semi_planar(fmt)) {
        return yuv->width;
    } else if (is_mch_planar(fmt)) {
        return yuv->width / 2;
    }
    
    return 0;
}

int get_uv_height(yuv_seq_t *yuv)
{
    int fmt = yuv->yuvfmt;

    if       (is_mch_422(fmt)) {
        return yuv->height;
    } else if (is_mch_420(fmt)) {
        return yuv->height / 2;
    }

    return 0;
}


void set_yuv_prop(yuv_seq_t *yuv, int w, int h, int fmt, 
                    int nbit, int nlsb, int btile, 
                    int stride, int io_size)
{
    yuv->width      = w;
    yuv->height     = h;
    yuv->yuvfmt     = fmt;
    yuv->nbit       = nbit;
    yuv->nlsb       = nlsb;
    yuv->btile      = btile;

    if (btile) 
    {
        tile_t *t = &yuv->tile; 
        if (yuv->nbit == 10) { 
            t->tw = 3; t->th = 4; t->tsz = 16; 
        } else 
        if (yuv->nbit == 8) { 
            t->tw = 8; t->th = 4; t->tsz = 32;
        } else {
            printf("@err>> not support bitdepth (%d) for tile mode\n", yuv->nbit);
            t->tw = 8; t->th = 4; t->tsz = 64;
        }
        
        yuv->y_stride = sat_div(w, t->tw) * t->tsz;
        yuv->y_stride = max(stride,  yuv->y_stride);
        
        yuv->y_size = yuv->y_stride * sat_div(h, t->th);
    } 
    else 
    {
        yuv->y_stride = sat_div(yuv->width * yuv->nbit, 8);
        yuv->y_stride = max(stride,  yuv->y_stride);
        
        yuv->y_size = yuv->y_stride * yuv->height;
    }
    
    if      (fmt == YUVFMT_400P)
    {
        yuv->uv_stride  = 0;
        yuv->uv_size    = 0;
        yuv->io_size    = yuv->y_size;
    }
    else if (fmt == YUVFMT_420P)
    {
        yuv->uv_stride  = yuv->y_stride / 2;
        yuv->uv_size    = yuv->y_size   / 4;
        yuv->io_size    = yuv->y_size + 2 * yuv->uv_size;
    }
    else if (fmt == YUVFMT_420SP)
    {
        assert( is_bit_aligned(1, yuv->height) );
        
        yuv->uv_stride  = yuv->y_stride;
        yuv->uv_size    = yuv->y_size   / 2;
        yuv->io_size    = yuv->y_size + yuv->uv_size;
    }
    else if (fmt == YUVFMT_422P)
    {
        yuv->uv_stride  = yuv->y_stride / 2;
        yuv->uv_size    = yuv->y_size   / 2;
        yuv->io_size    = yuv->y_size + 2 * yuv->uv_size;
    }
    else if (fmt == YUVFMT_UYVY || fmt == YUVFMT_YUYV)
    {
        yuv->y_stride   = yuv->y_stride * 2; 
        yuv->y_size     = yuv->y_size   * 2;
        yuv->uv_stride  = 0;
        yuv->uv_size    = 0;
        yuv->io_size    = yuv->y_size;
    }
    
    yuv->io_size = max(io_size, yuv->io_size);
    assert(yuv->buf_size >= yuv->io_size);
    
    void show_yuv_prop(yuv_seq_t *yuv);
    //show_yuv_prop(yuv);

    return;
}

void set_yuv_prop_by_copy(yuv_seq_t *dst, yuv_seq_t *src)
{
    return set_yuv_prop(dst, 
            src->width, src->height, src->yuvfmt, 
            src->nbit,  src->nlsb,   src->btile, 
            src->y_stride, src->io_size);
}

void show_yuv_prop(yuv_seq_t *yuv)
{
    printf("\n");
    printf("width       = %d\n" , yuv->width     );
    printf("height      = %d\n" , yuv->height    );
    printf("yuvfmt      = %s\n" , yuv->yuvfmt    );
    printf("nlsb        = %d\n" , yuv->nlsb      );
    printf("nbit        = %d\n" , yuv->nbit      );
    printf("btile       = %d\n" , yuv->btile     );
    printf("y_stride    = %d\n" , yuv->y_stride  );
    printf("uv_stride   = %d\n" , yuv->uv_stride );
    printf("y_size      = %d\n" , yuv->y_size    );
    printf("uv_size     = %d\n" , yuv->uv_size   );
    printf("io_size     = %d\n" , yuv->io_size   );
    printf("buf_size    = %d\n" , yuv->buf_size  );
    
    printf("tile.tw     = %d\n" , yuv->tile.tw   );
    printf("tile.th     = %d\n" , yuv->tile.th   );
    printf("tile.tsz    = %d\n" , yuv->tile.tsz  );
    printf("\n");
}