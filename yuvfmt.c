#include<stdio.h>
#include<string.h>
#include<malloc.h>
#include<assert.h>

#include "yuvfmt.h"

static int fcall_layer = 0;
#define ENTER_FUNC  printf("%-2d: %s +++\n", fcall_layer++, __FUNCTION__)
#define LEAVE_FUNC  printf("%-2d: %s ---\n", --fcall_layer, __FUNCTION__)

inline int sat_div(int num, int den)
{
    return (num + den - 1)/den;
}

int bit_saturate(int nbit, int val)
{
    int pad = (1<<nbit) - 1;
    return (val+pad) & (~pad);
}

int is_bit_aligned(int nbit, int val)
{
    int pad = (1<<nbit) - 1;
    return (val&pad)==0;
}

int get_spl_fmt(int fmt)
{
    if (fmt == YUVFMT_420P || fmt == YUVFMT_420SP || fmt == YUVFMT_420SP)
    {
        return YUVFMT_420P;
    }
    else if (fmt == YUVFMT_422P || fmt == YUVFMT_422SP || 
            fmt == YUVFMT_UYVY || fmt == YUVFMT_YUYV)
    {
        return YUVFMT_422P;
    }
    else
    {
        return YUVFMT_UNSUPPORT;
    }
}

void swap_uv(uint8_t **u, uint8_t **v)
{
    uint8_t *m = *u;
    *u = *v;
    *v = m;
}

void b8tile_2_rect_align_0(uint8_t* src, int tw, int th, uint8_t* dst, int s)
{
    int i, j;
    for (j=0; j<th; ++j) {
        for (i=0; i<tw; ++i) {
            *((uint8_t*)(dst+i)) = *((uint8_t*)(src+i));
        }
        src += tw;
        dst += s;
    }
}

void b8tile_2_rect_align_4(uint8_t* src, int tw, int th, uint8_t* dst, int s)
{
    int i, j;
    for (j=0; j<th; ++j) {
        for (i=0; i<tw; i+=4) {
            *((uint32_t*)(dst+i)) = *((uint32_t*)(src+i));
        }
        src += tw;
        dst += s;
    }
}

void b8tile_2_rect_align_8(uint8_t* src, int tw, int th, uint8_t* dst, int s)
{
    int i, j;
    for (j=0; j<th; ++j) {
        for (i=0; i<tw; i+=8) {
            *((uint64_t*)(dst+i)) = *((uint64_t*)(src+i));
        }
        src += tw;
        dst += s;
    }
}

void b8tile_2_rect_width_8(uint8_t* src, int tw, int th, uint8_t* dst, int s)
{
    int i, j;
    for (j=0; j<th; ++j) {
        *((uint64_t*)(dst)) = *((uint64_t*)(src));
        src += tw;
        dst += s;
    }
}

void b8tile_2_rect(uint8_t* src, int tw, int th, uint8_t* dst, int s)
{
    if      ( ((int)src&3) || (tw&3) || ((int)dst&3) || (s&3) )
        b8tile_2_rect_align_0(src, tw, th, dst, s);
    else if ( ((int)src&7) || (tw&7) || ((int)dst&7) || (s&7) )
        b8tile_2_rect_align_4(src, tw, th, dst, s);
    else if ( tw == 8 )
        b8tile_2_rect_width_8(src, tw, th, dst, s); 
    else
        b8tile_2_rect_align_8(src, tw, th, dst, s);   
}

void b8tile_rescan_planar
(
    uint8_t* pt, int tw, int th, int tsz, int ts, 
    uint8_t* pl, int w,  int h,  int s
)
{
    int x, y, tx, ty;
    void (*tile2rect_func_p)(uint8_t* src, int tw, int th, uint8_t* dst, int s);
    
    ENTER_FUNC;

    if      ( ((int)pt&3) || (tw&3) || (tsz&3) || (ts&3) || ((int)pl&3) || (s&3) )
        tile2rect_func_p = b8tile_2_rect_align_0;
    else if ( ((int)pl&7) || (tw&7) || (tsz&7) || (ts&7) || ((int)pl&7) || (s&7) )
        tile2rect_func_p = b8tile_2_rect_align_4;
    else if ( tw == 8 )
        tile2rect_func_p = b8tile_2_rect_width_8; 
    else
        tile2rect_func_p = b8tile_2_rect_align_8;     

    for (ty=0, y=0; y<h; y+=th, ++ty) 
    {
        for (tx=0, x=0; x<w; x+=tw, ++tx) 
        {
            uint8_t* src = &pt[ts*ty + tsz*tx];
            uint8_t* dst = &pl[s * y + x];

            tile2rect_func_p(src, tw, th, dst, s);
        }
    } 
    
    LEAVE_FUNC;
}

void b8tile_rescan_mch(yuv_seq_t *psrc, yuv_seq_t *pdst)
{
    int fmt = psrc->yuvfmt;
    
    int tw  = psrc->tile.tw;
    int th  = psrc->tile.th;
    int tsz = psrc->tile.tsz;
    int ts  = psrc->y_stride;
    int w   = pdst->w_align;
    int h   = pdst->h_align;
    int s   = pdst->y_stride;
    uint8_t *pt = psrc->pbuf;
    uint8_t *pl = pdst->pbuf;
    
    ENTER_FUNC;
    
    assert (psrc->b10     == pdst->b10);
    assert (psrc->w_align == pdst->w_align);
    assert (psrc->h_align == pdst->h_align);
    
    if (fmt == YUVFMT_420P || fmt == YUVFMT_422P)
    {
        b8tile_rescan_planar(pt, tw, th, tsz, ts, pl, w, h, s);
        
        ts  = psrc->uv_stride;
        s   = pdst->uv_stride;
        w   = w/2;
        h   = (fmt == YUVFMT_422P) ? h : h/2;
        
        pt += psrc->y_size;
        pl += pdst->y_size;
        
        b8tile_rescan_planar(pt, tw, th, tsz, ts, pl, w, h, s);
        
        pt += psrc->uv_size;
        pl += pdst->uv_size;
        
        b8tile_rescan_planar(pt, tw, th, tsz, ts, pl, w, h, s);
    }
    else if (fmt == YUVFMT_420SP)
    {
        b8tile_rescan_planar(pt, tw, th, tsz, ts, pl, w, h, s);
        
        ts  = psrc->uv_stride;
        s   = pdst->uv_stride;
        h   = h/2;

        pt += psrc->y_size;
        pl += pdst->y_size;
        
        b8tile_rescan_planar(pt, tw, th, tsz, ts, pl, w, h, s);
    }
    else if (fmt == YUVFMT_UYVY || fmt == YUVFMT_YUYV)
    {
        w   = w*2;
        b8tile_rescan_planar(pt, tw, th, tsz, ts, pl, w, h, s);
    }
    
    LEAVE_FUNC;

    return;
}

//void b10tile_rescan_mch(yuv_seq_t *psrc, yuv_seq_t *pdst)
//{
//    ENTER_FUNC;
//    
//    psrc->tile.tw *= 2;
//    psrc->width   *= 2;
//    b8tile_rescan_mch(psrc, pdst);
//    psrc->tile.tw /= 2;
//    psrc->width   /= 2;
//    
//    LEAVE_FUNC;
//
//    return;
//}

/**
 * little endian
 *                       
 *               |- rbit -|- (32-rbit) --| 
 *               000000000000000|- rbit -| 
 *  bit      high------------------------> low
 *  byte         |  3  |  2  |  1  |  0  | 
 *  
 */
void b10_unpack_linear(uint8_t*  src8, int n_byte, uint8_t* dst8, int n16)
{
    register i64_pack_t bp;
    
    bp.i64 = 0;
    uint16_t v16;
    int rbit = 0;
    int i32  = 0;
    int n32  = n_byte >> 2;
    int i16  = 0;
    uint32_t* src = (uint32_t*)src8;
    uint16_t* dst = (uint16_t*)dst8;

    //assert( (n_byte & 0x08) == 0 );
    //assert( n_byte > (n16*5+3)/4 );
    
    while (1) 
    {
        if (rbit<10) {
            if (i32 < n32 && i16 < n16) {
                bp.i32_1 = src[i32++];
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
        dst[i16++] = v16;
    }
}

void b10_unpack_rect
(
    uint8_t* src, int src_stride,
    uint8_t* dst, int dst_stride,
    int w, int h
)
{
    int x, y;
    
    ENTER_FUNC;
    
    for (y=0; y<h; ++y) 
    {
        b10_unpack_linear(src, src_stride, dst, w);
        src += src_stride;
        dst += dst_stride;
    }
    
    LEAVE_FUNC;
}

int b10_unpack_mch(yuv_seq_t *psrc, yuv_seq_t *pdst)
{
    int fmt = psrc->yuvfmt;
    
    uint8_t* src8   = psrc->pbuf; 
    uint8_t* dst8   = pdst->pbuf; 
    int src_stride  = psrc->y_stride; 
    int dst_stride  = pdst->y_stride;
    int w   = psrc->w_align; 
    int h   = psrc->h_align; 

    ENTER_FUNC;
    
    assert (psrc->b10     == pdst->b10);
    assert (psrc->w_align == pdst->w_align);
    assert (psrc->h_align == pdst->h_align);
    
    if (fmt == YUVFMT_420P || fmt == YUVFMT_422P)
    {
        b10_unpack_rect(src8, src_stride, dst8, dst_stride, w, h);
        
        src8       += psrc->y_size;
        dst8       += pdst->y_size; 
        src_stride  = psrc->uv_stride;
        dst_stride  = pdst->uv_stride;
        
        w   = w/2;
        h   = (fmt == YUVFMT_422P) ? h : h/2;
        
        b10_unpack_rect(src8, src_stride, dst8, dst_stride, w, h);
        
        src8 += psrc->uv_size;
        dst8 += pdst->uv_size;
        
        b10_unpack_rect(src8, src_stride, dst8, dst_stride, w, h);
    }
    else if (fmt == YUVFMT_420SP || fmt == YUVFMT_422SP)
    {
        b10_unpack_rect(src8, src_stride, dst8, dst_stride, w, h);
        
        src8       += psrc->y_size;
        dst8       += pdst->y_size; 
        src_stride  = psrc->uv_stride;
        dst_stride  = pdst->uv_stride;
        
        h   = (fmt == YUVFMT_422SP) ? h : h/2;
        
        b10_unpack_rect(src8, src_stride, dst8, dst_stride, w, h);
    }
    else if (fmt == YUVFMT_UYVY || fmt == YUVFMT_YUYV)
    {
        w   = w*2;
        b10_unpack_rect(src8, src_stride, dst8, dst_stride, w, h);
    }
    
    LEAVE_FUNC;

    return;
}

//int b10tile_unpack_2_tile
//(
//    uint8_t* tight_base, int tw, int th,  int tsz1, int ts1, 
//    uint8_t* loose_base, int  w, int  h,  int tsz2, int ts2,
//)
//{
//    int x, y, tx, ty;
//
//    for (ty=0, y=0; y<h; y+=th, ++ty) 
//    {
//        for (tx=0, x=0; x<w; x+=tw, ++tx) 
//        {
//            uint8_t* src = &tight_base[ts1*ty + tsz1*tx];
//            uint8_t* dst = &loose_base[ts2*ty + tsz2*tx];
//    
//            b10_unpack_linear(src, tsz1, dst, tw*th);
//        }
//    }
//}

int b10tile_unpack_planar
(
    uint8_t* tight_base, int tw, int th, int tsz, int ts, 
    uint8_t* plane_base, int w,  int h,  int s
)
{
    int x, y, tx, ty;
    
    ENTER_FUNC;
    
    assert( (tsz & 7) == 0 );
    assert( ts >= (w*th*5+3)/4 );
    
    #define LOOSE_BUF_SIZE 4094
    static uint8_t loose_buf[LOOSE_BUF_SIZE];
    uint8_t *loose_base = loose_buf;
    int size_needed = tw * th * sizeof(uint16_t);
    if (size_needed>LOOSE_BUF_SIZE)
    {
        loose_base = (uint8_t*) malloc ( size_needed );
        if (loose_base==0) {
            printf("%s : malloc fail!\n", __FUNCTION__);
            return 0;
        }
    }
    
    for (ty=0, y=0; y<h; y+=th, ++ty) 
    {
        for (tx=0, x=0; x<w; x+=tw, ++tx) 
        {
            uint8_t* src = &tight_base[ts*ty + tsz*tx];
            uint8_t* dst = &plane_base[s * y + sizeof(uint16_t) * x];
            
            b10_unpack_linear(src, tsz, loose_base, tw*th);
            
            b8tile_2_rect(loose_base, tw*2, th, dst, s);
        }
    }
    
    if (size_needed>LOOSE_BUF_SIZE)
    {
        free(loose_base);
    }
    
    LEAVE_FUNC;
    
    return w*h;
}

 int b10tile_unpack_mch(yuv_seq_t *psrc, yuv_seq_t *pdst)
{
    int fmt = psrc->yuvfmt;

    ENTER_FUNC;
    
    int tw  = psrc->tile.tw;
    int th  = psrc->tile.th;
    int tsz = psrc->tile.tsz;
    int ts  = psrc->y_stride;
    int w   = pdst->w_align;
    int h   = pdst->h_align;
    int s   = pdst->y_stride;
    uint8_t *pt = psrc->pbuf;
    uint8_t *pl = pdst->pbuf;
    
    if (fmt == YUVFMT_420P || fmt == YUVFMT_422P)
    {
        b10tile_unpack_planar(pt, tw, th, tsz, ts, pl, w, h, s);
        
        ts  = psrc->uv_stride;
        s   = pdst->uv_stride;
        w   = w/2;
        h   = (fmt == YUVFMT_422P) ? h : h/2;
        
        pt += psrc->y_size;
        pl += pdst->y_size;
        
        b10tile_unpack_planar(pt, tw, th, tsz, ts, pl, w, h, s);
        
        pt += psrc->uv_size;
        pl += pdst->uv_size;
        
        b10tile_unpack_planar(pt, tw, th, tsz, ts, pl, w, h, s);
    }
    else if (fmt == YUVFMT_420SP)
    {
        b10tile_unpack_planar(pt, tw, th, tsz, ts, pl, w, h, s);
        
        ts  = psrc->uv_stride;
        s   = pdst->uv_stride;
        h   = h/2;

        pt += psrc->y_size;
        pl += pdst->y_size;
        
        b10tile_unpack_planar(pt, tw, th, tsz, ts, pl, w, h, s);
    }
    else if (fmt == YUVFMT_UYVY || fmt == YUVFMT_YUYV)
    {
        w   = w*2;
        b10tile_unpack_planar(pt, tw, th, tsz, ts, pl, w, h, s);
    }
    
    LEAVE_FUNC;

    return;
}

/**
 *  8bit, lama raster, not tiled
 */
int set_bufsz_aligned_b8(yuv_seq_t *yuv, int bufw, int bufh, int w_align_bit, int h_align_bit)
{
    int fmt = yuv->yuvfmt;

    ENTER_FUNC;
    
    bufw = bit_saturate(w_align_bit, bufw);
    bufh = bit_saturate(h_align_bit, bufh);
    assert( is_bit_aligned(1, bufw) );
    
    yuv->y_stride   = bufw;
    yuv->y_size     = bufw * bufh;
    
    if (fmt == YUVFMT_420P)
    {
        yuv->uv_stride  = yuv->y_stride / 2;
        yuv->uv_size    = yuv->y_size   / 4;
        yuv->io_size    = yuv->y_size + 2 * yuv->uv_size;
    }
    else if (fmt == YUVFMT_420SP)
    {
        assert( is_bit_aligned(1, bufh) );
        
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
    
    yuv->buf_size = bit_saturate(7, yuv->io_size);
    
    LEAVE_FUNC;

    return yuv->buf_size;
}

int set_bufsz_aligned(yuv_seq_t *yuv)
{
    ENTER_FUNC;

    set_bufsz_aligned_b8(yuv, yuv->w_align, yuv->h_align, 0, 0);
    
    if (yuv->b10) {
        yuv->y_stride   *= 2;
        yuv->y_size     *= 2;
        yuv->uv_stride  *= 2;
        yuv->uv_size    *= 2;
        yuv->io_size    *= 2;
        yuv->buf_size   = bit_saturate(8, yuv->io_size);
    }
    
    LEAVE_FUNC;
    
    return yuv->io_size;
}

/**
 *  luma raster
 */
int set_bufsz_src_raster(yuv_seq_t *yuv)
{
    int bufw = yuv->w_align;
    int bufh = yuv->h_align;
    
    ENTER_FUNC;

    if (yuv->b10) {
        bufw = bit_saturate(3, bufw * 5 / 4);
        
        //if (fmt == YUVFMT_420P || fmt == YUVFMT_422P)  {
        //    set_bufsz_aligned_b8(yuv, bufw, bufh, 6, 0);
        //} else if (fmt == YUVFMT_420SP) {
        //    set_bufsz_aligned_b8(yuv, bufw, bufh, 5, 0);
        //} else if (fmt == YUVFMT_UYVY || fmt == YUVFMT_YUYV) {
        //    set_bufsz_aligned_b8(yuv, bufw, bufh, 4, 0);
        //}
    }
    
    set_bufsz_aligned_b8(yuv, bufw, bufh, 0, 0);
    
    LEAVE_FUNC;
    
    return yuv->io_size;
}

int set_bufsz_src_tile(yuv_seq_t *yuv, int tw, int th, int tsz)
{
    ENTER_FUNC;
    
    int fmt = yuv->yuvfmt;
    yuv->tile.tw    = tw;
    yuv->tile.th    = th;
    yuv->tile.tsz   = tsz;
    
    yuv->y_stride   = sat_div(yuv->w_align, tw) * tsz; 
    yuv->y_size     = sat_div(yuv->h_align, th) * yuv->y_stride;
    
    if (fmt == YUVFMT_420P)
    {
        yuv->uv_stride  = sat_div(yuv->w_align / 2, tw) * tsz;
        yuv->uv_size    = sat_div(yuv->h_align / 2, th) * yuv->uv_stride;
        yuv->io_size    = yuv->y_size + 2 * yuv->uv_size;
    }
    else if (fmt == YUVFMT_420SP)
    {
        yuv->uv_stride  = yuv->y_stride; 
        yuv->uv_size    = sat_div(yuv->h_align / 2, th) * yuv->uv_stride; 
        yuv->io_size    = yuv->y_size + yuv->uv_size;
    }

    else if (fmt == YUVFMT_422P)  
    {
        yuv->uv_stride  = sat_div(yuv->w_align / 2, tw) * tsz;
        yuv->uv_size    = sat_div(yuv->h_align    , th) * yuv->uv_stride;
        yuv->io_size    = yuv->y_size + 2 * yuv->uv_size;
    }
    else if (fmt == YUVFMT_UYVY || fmt == YUVFMT_YUYV)
    {
        yuv->y_stride   = sat_div(yuv->w_align * 2, tw) * tsz; 
        yuv->y_size     = sat_div(yuv->h_align * 2, th) * yuv->y_stride;
        yuv->uv_stride  = 0;
        yuv->uv_size    = 0;
        yuv->io_size    = yuv->y_size;
    }
    
    yuv->buf_size = bit_saturate(7, yuv->io_size);
    
    LEAVE_FUNC;
    
    return yuv->io_size;
}

/**
 *  set buffer size for tile format in bitdepth=16 
 *  
 *  b10tile_unpack_mch() can be splited into 2 steps
 *      1) b10tile_unpack_2_tile()
 *      2) tile2raster_mch()
 */
//int set_bufsz_aligned_tile(yuv_seq_t *yuv, int tw, int th)
//{
//    ENTER_FUNC;
//    
//    int tile_size = tw*th * (yuv->b10 ? 2 : 1);
//    set_bufsz_src_tile(yuv, tw, th, tile_size);
//    
//    LEAVE_FUNC;
//    
//    return yuv->io_size;
//}

void set_seq_info(yuv_seq_t *yuv, int w, int h, int fmt, int b10, int btile, int w_align_bit, int h_align_bit)
{
    ENTER_FUNC;
    
    yuv->width      = w;
    yuv->height     = h;
    yuv->w_align    = bit_saturate( w_align_bit, yuv->width  );
    yuv->h_align    = bit_saturate( h_align_bit, yuv->height );
    yuv->yuvfmt     = fmt;
    yuv->b10        = b10;
    yuv->btile      = btile;
    
    set_bufsz_aligned(yuv);

    LEAVE_FUNC;
}

void show_yuv_info(yuv_seq_t *yuv)
{
    printf("\n");
    printf("width       = %d\n" , yuv->width     );
    printf("height      = %d\n" , yuv->height    );
    printf("w_align     = %d\n" , yuv->w_align   );
    printf("h_align     = %d\n" , yuv->h_align   );
    printf("yuvfmt      = %d\n" , yuv->yuvfmt    );
    printf("b10         = %d\n" , yuv->b10       );
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

int b10_clip8_planar
(
    uint8_t* src8, int src_stride, 
    uint8_t* dst8, int dst_stride,
    int w,  int h
)
{
    int x, y;

    for (y=0; y<h; ++y) 
    {
        uint16_t* src = (uint16_t*)(src8 + y * src_stride);
        uint8_t*  dst = (uint8_t* )(dst8 + y * dst_stride);
        
        for (x=0; x<w; ++x) 
        {
            //dst[x] = (uint8_t)(src[x] >> 2);
            *(dst++) = (uint8_t)(*(src++) >> 2);
        }
    }
    
    return w*h;
}

int b10_clip8_mch(yuv_seq_t *psrc, yuv_seq_t *pdst)
{
    int fmt = psrc->yuvfmt;
    
    uint8_t* src8   = psrc->pbuf; 
    uint8_t* dst8   = pdst->pbuf; 
    int src_stride  = psrc->y_stride; 
    int dst_stride  = pdst->y_stride;
    int w   = psrc->w_align; 
    int h   = psrc->h_align; 

    ENTER_FUNC;
    
    assert (psrc->yuvfmt  == pdst->yuvfmt);
    assert (psrc->w_align == pdst->w_align);
    assert (psrc->h_align == pdst->h_align);
    
    if (fmt == YUVFMT_420P || fmt == YUVFMT_422P)
    {
        b10_clip8_planar(src8, src_stride, dst8, dst_stride, w, h);
        
        src8       += psrc->y_size;
        dst8       += pdst->y_size; 
        src_stride  = psrc->uv_stride;
        dst_stride  = pdst->uv_stride;
        
        w   = w/2;
        h   = (fmt == YUVFMT_422P) ? h : h/2;
        
        b10_clip8_planar(src8, src_stride, dst8, dst_stride, w, h);
        
        src8 += psrc->uv_size;
        dst8 += pdst->uv_size;
        
        b10_clip8_planar(src8, src_stride, dst8, dst_stride, w, h);
    }
    else if (fmt == YUVFMT_420SP)
    {
        b10_clip8_planar(src8, src_stride, dst8, dst_stride, w, h);
        
        src8       += psrc->y_size;
        dst8       += pdst->y_size; 
        src_stride  = psrc->uv_stride;
        dst_stride  = pdst->uv_stride;
        h   = (fmt == YUVFMT_422SP) ? h : h/2;
        
        b10_clip8_planar(src8, src_stride, dst8, dst_stride, w, h);
    }
    else if (fmt == YUVFMT_UYVY || fmt == YUVFMT_YUYV)
    {
        w   = w*2;
        b10_clip8_planar(src8, src_stride, dst8, dst_stride, w, h);
    }
    
    LEAVE_FUNC;

    return 0;
}

int b8mch_p2p(yuv_seq_t *psrc, yuv_seq_t *pdst)
{
    uint8_t* src_y_base = psrc->pbuf;
    uint8_t* src_u_base = src_y_base + psrc->y_size;
    uint8_t* src_v_base = src_u_base + psrc->uv_size;
    
    uint8_t* dst_y_base = pdst->pbuf;
    uint8_t* dst_u_base = dst_y_base + pdst->y_size;
    uint8_t* dst_v_base = dst_u_base + pdst->uv_size;
    
    int w   = psrc->w_align; 
    int h   = psrc->h_align; 
    int x, y;

    ENTER_FUNC;
    
    for (y=0; y<h; ++y) {
        uint8_t* src_y = src_y_base + y * psrc->y_stride;
        uint8_t* dst_y = dst_y_base + y * pdst->y_stride;
        memcpy(dst_y, src_y, w);
    }

    w   = w/2;
    if (psrc->yuvfmt == YUVFMT_420P) {
        h = h/2;
    }
    
    for (y=0; y<h; ++y) {
        uint8_t* src_u = src_u_base + y * psrc->uv_stride;
        uint8_t* src_v = src_v_base + y * psrc->uv_stride;
        uint8_t* dst_u = dst_u_base + y * pdst->uv_stride;
        uint8_t* dst_v = dst_v_base + y * pdst->uv_stride;
        
        memcpy(dst_u, src_u, w);
        memcpy(dst_v, src_v, w);
    }
    
    LEAVE_FUNC;
    
    return 0;
}

int b8mch_sp2p(yuv_seq_t *psrc, yuv_seq_t *pdst)
{
    uint8_t* src_y_base = psrc->pbuf;
    uint8_t* src_u_base = src_y_base + psrc->y_size;
    
    uint8_t* dst_y_base = pdst->pbuf;
    uint8_t* dst_u_base = dst_y_base + pdst->y_size;
    uint8_t* dst_v_base = dst_u_base + pdst->uv_size;
    
    int w   = psrc->w_align; 
    int h   = psrc->h_align; 
    int x, y;

    ENTER_FUNC;
    
    for (y=0; y<h; ++y) {
        uint8_t* src_y = src_y_base + y * psrc->y_stride;
        uint8_t* dst_y = dst_y_base + y * pdst->y_stride;
        memcpy(dst_y, src_y, w);
    }

    w   = w/2;
    if (psrc->yuvfmt == YUVFMT_420SP) {
        h = h/2;
    }
    
    for (y=0; y<h; ++y) {
        uint8_t* src_u = src_u_base + y * psrc->uv_stride;
        uint8_t* dst_u = dst_u_base + y * pdst->uv_stride;
        uint8_t* dst_v = dst_v_base + y * pdst->uv_stride;

        for (x=0; x<w; ++x) {
            *(dst_u++) = *(src_u++);
            *(dst_v++) = *(src_u++);
        }
    }
    
    LEAVE_FUNC;
    
    return 0;
}

int b8mch_yuyv2p(yuv_seq_t *psrc, yuv_seq_t *pdst)
{
    uint8_t* src_y_base = psrc->pbuf;
    
    uint8_t* dst_y_base = pdst->pbuf;
    uint8_t* dst_u_base = dst_y_base + pdst->y_size;
    uint8_t* dst_v_base = dst_u_base + pdst->uv_size;

    int w   = psrc->w_align; 
    int h   = psrc->h_align; 
    int x, y;

    ENTER_FUNC;

    w   = w/2;

    for (y=0; y<h; ++y) 
    {
        uint8_t* src_y  = src_y_base + y * psrc->y_stride;
        uint8_t* dst_y  = dst_y_base + y * pdst->y_stride;
        uint8_t* dst_u  = dst_u_base + y * pdst->uv_stride;
        uint8_t* dst_v  = dst_v_base + y * pdst->uv_stride;

        if (psrc->yuvfmt == YUVFMT_YUYV) {
            for (x=0; x<w; ++x) {
                *(dst_y++) = *(src_y++);
                *(dst_u++) = *(src_y++);
                *(dst_y++) = *(src_y++);
                *(dst_v++) = *(src_y++);
            }
        } else {
            for (x=0; x<w; ++x) {
                *(dst_u++) = *(src_y++);
                *(dst_y++) = *(src_y++);
                *(dst_v++) = *(src_y++);
                *(dst_y++) = *(src_y++);
            }
        }
    }   /* end for y*/

    LEAVE_FUNC;
    
    return 0;
}

int b8mch_spliting(yuv_seq_t *psrc, yuv_seq_t *pdst)
{
    int fmt = psrc->yuvfmt;
    
    ENTER_FUNC;

    assert (psrc->b10     == pdst->b10);
    assert (psrc->w_align == pdst->w_align);
    assert (psrc->h_align == pdst->h_align);
    assert (pdst->yuvfmt  == get_spl_fmt(fmt));

    if        (fmt == YUVFMT_420P  || fmt == YUVFMT_422P ) {
        b8mch_p2p(psrc, pdst);    
    } else if (fmt == YUVFMT_420SP || fmt == YUVFMT_422SP) {
        b8mch_sp2p(psrc, pdst);
    } else if (fmt == YUVFMT_UYVY  || fmt == YUVFMT_YUYV ) {
        b8mch_yuyv2p(psrc, pdst);
    }
    
    LEAVE_FUNC;
    
    return 0;
}

int b8mch_spliting_all(yuv_seq_t *psrc, yuv_seq_t *pdst)
{
    int fmt = psrc->yuvfmt;

    uint8_t* src_y_base = psrc->pbuf;
    uint8_t* src_u_base = src_y_base + psrc->y_size;
    uint8_t* src_v_base = src_u_base + psrc->uv_size;
    
    uint8_t* dst_y_base = pdst->pbuf;
    uint8_t* dst_u_base = dst_y_base + pdst->y_size;
    uint8_t* dst_v_base = dst_u_base + pdst->uv_size;
    
    int w   = psrc->w_align; 
    int h   = psrc->h_align; 
    int x, y;

    ENTER_FUNC;
    
    if (fmt == YUVFMT_UYVY   || fmt == YUVFMT_YUYV ) 
    {
        w   = w/2;
        if (psrc->yuvfmt == YUVFMT_UYVY) {
            dst_v_base = dst_y_base + pdst->y_size;
            dst_u_base = dst_v_base + pdst->uv_size;
        }
        
        for (y=0; y<h; ++y) {
            uint8_t* src_y  = src_y_base + y * psrc->y_stride;
            
            uint8_t* dst_y  = dst_y_base + y * pdst->y_stride;
            uint8_t* dst_u  = dst_u_base + y * pdst->uv_stride;
            uint8_t* dst_v  = dst_v_base + y * pdst->uv_stride;

            for (x=0; x<w; ++x) {
                *(dst_y++) = *(src_y++);
                *(dst_u++) = *(src_y++);
                *(dst_y++) = *(src_y++);
                *(dst_v++) = *(src_y++);
            }
        }
    } 
    else    /* 420p, 420sp, 422p, 422sp*/
    {
        for (y=0; y<h; ++y) {
            uint8_t* src_y = src_y_base + y * psrc->y_stride;
            uint8_t* dst_y = dst_y_base + y * pdst->y_stride;
            memcpy(dst_y, src_y, w);
        }
        
        w   = w/2;
        if (psrc->yuvfmt == YUVFMT_420P || psrc->yuvfmt == YUVFMT_420SP) {
            h = h/2;
        }
        
        if (fmt == YUVFMT_420SP || fmt == YUVFMT_422SP) 
        {
            for (y=0; y<h; ++y) {
                uint8_t* src_u = src_u_base + y * psrc->uv_stride;
                uint8_t* dst_u = dst_u_base + y * pdst->uv_stride;
                uint8_t* dst_v = dst_v_base + y * pdst->uv_stride;

                for (x=0; x<w; ++x) {
                    *(dst_u++) = *(src_u++);
                    *(dst_v++) = *(src_u++);
                }
            }
        } 
        else if (fmt == YUVFMT_420P  || fmt == YUVFMT_422P ) 
        {
            for (y=0; y<h; ++y) {
                uint8_t* src_u = src_u_base + y * psrc->uv_stride;
                uint8_t* src_v = src_v_base + y * psrc->uv_stride;
                uint8_t* dst_u = dst_u_base + y * pdst->uv_stride;
                uint8_t* dst_v = dst_v_base + y * pdst->uv_stride;
                
                memcpy(dst_u, src_u, w);
                memcpy(dst_v, src_v, w);
            }
        }
    }       /* 420p, 420sp, 422p, 422sp*/
    
    LEAVE_FUNC;
    
    return 0;
}

int main(int argc, char **argv)
{
    int r;
    if (argc<=7)
    {
        printf("prog [1]dst.yuv [2]src.yuv [3]w<int> [4]h<int> [5]start<int> [6]nfrm<int> \n");
        printf("     [7]srcfmt<str>, [8]b10, [9]btile [10]tw [11]th [12]tsz\n");
        return -1;
    } 

    yuv_seq_t   seq_src;
    yuv_seq_t   seq_b16;
    yuv_seq_t   seq_dst;
    yuv_seq_t   seq_spl;
    memset(&seq_src, 0, sizeof(yuv_seq_t));
    memset(&seq_b16, 0, sizeof(yuv_seq_t));
    memset(&seq_dst, 0, sizeof(yuv_seq_t));
    memset(&seq_spl, 0, sizeof(yuv_seq_t));
    
    seq_dst.fp = fopen(argv[1],"wb");
    seq_src.fp = fopen(argv[2],"rb");
    if( !seq_dst.fp || !seq_src.fp )
    {
        printf("error : open %s %s fail\n", seq_dst.fp?"":argv[1], seq_src.fp?"":argv[2]);
        return -1;
    }

    int w       = atoi(argv[3]);
    int h       = atoi(argv[4]);
    int start   = atoi(argv[5]);
    int nfrm    = atoi(argv[6]);
    
    printf("w=%d, h=%d, start=%d, nfrm=%d\n", w, h, start, nfrm);

    int fmt     = YUVFMT_420P;
    int b10     = 0;
    int btile   = 0;
    int tw      = 0;
    int th      = 0;
    int tsz     = 0; 
    
    if (argc>=8) {
        if      ( !strcmp(argv[7], "420p" ) )  fmt = YUVFMT_420P;
        else if ( !strcmp(argv[7], "420sp") )  fmt = YUVFMT_420SP;
        else if ( !strcmp(argv[7], "422p" ) )  fmt = YUVFMT_422P;
        else if ( !strcmp(argv[7], "422sp") )  fmt = YUVFMT_422SP;
        else if ( !strcmp(argv[7], "uyvy" ) )  fmt = YUVFMT_UYVY;
        else if ( !strcmp(argv[7], "yuyv" ) )  fmt = YUVFMT_YUYV;
        else    { printf("error fmt\n");        return -1;  }
    }
    if (argc>=9) {
        b10     = atoi(argv[8]);
    }
    if (argc>=10) {
        btile   = atoi(argv[9]);
        if (btile) {
            if (argc<12) {
                printf("tile size(wxh) must be specified under tile format\n");
                return -1;
            }
            tw  = atoi(argv[10]);
            th  = atoi(argv[11]);
            if (argc<13) {
                printf("tile size(byte) must be specified for b10 tile\n");
                return -1;
            }
            tsz = atoi(argv[12]);
        }
    }

    set_seq_info( &seq_src, w, h, fmt,  b10, btile, 3, 3);
    set_seq_info( &seq_dst, w, h, fmt,  0,      0 , 3, 3);
    
    if (btile)  set_bufsz_src_tile(&seq_src, tw, th, tsz);
    else        set_bufsz_src_raster(&seq_src);
    
    printf("\n seq_src:\n");       show_yuv_info(&seq_src);
    printf("\n seq_dst:\n");       show_yuv_info(&seq_dst);

    seq_src.pbuf = (uint8_t *)malloc(seq_src.buf_size);
    seq_dst.pbuf = (uint8_t *)malloc(seq_dst.buf_size);
    if(!seq_src.pbuf || !seq_dst.pbuf)
    {
        printf("error: malloc %s %s fail\n", 
                seq_src.pbuf ? (free(seq_src.pbuf), "") : "src.pbuf", 
                seq_dst.pbuf ? (free(seq_dst.pbuf), "") : "dst.pbuf");
        return -1;
    }

    if (b10) 
    {  
        set_seq_info( &seq_b16, w, h, fmt, 1, 0, 3, 3 );
        printf("\n seq_b16:\n");    
        show_yuv_info(&seq_b16);

        seq_b16.pbuf = (uint8_t *)malloc(seq_b16.buf_size);
        if(!seq_b16.pbuf)
        {
            printf("error: malloc %s fail\n", "seq_b16.pbuf");
            return -1;
        }
    } 
    //if (fmt != YUVFMT_420P && fmt != YUVFMT_422P ) 
    {  
        set_seq_info( &seq_spl, w, h, get_spl_fmt(fmt), 0, 0, 3, 3 );
        printf("\n seq_spl:\n");        
        show_yuv_info(&seq_spl);

        seq_spl.pbuf = (uint8_t *)malloc(seq_spl.buf_size);
        if(!seq_spl.pbuf)
        {
            printf("error: malloc %s fail\n", "seq_spl.pbuf");
            return -1;
        }
    }      

    int i;
    for (i=0; i<nfrm; i++) 
    {
        int r=fseek(seq_src.fp, seq_src.io_size * i, SEEK_SET);
        if (r) {
            printf("fseek %d error\n", seq_src.io_size * i);
            return -1;
        }
        r = fread(seq_src.pbuf, seq_src.io_size, 1, seq_src.fp);
        if (r<1) {
            if ( feof(seq_src.fp) ) {
                printf("reach file end, force stop\n");
            } else {
                printf("error reading file\n");
            }
            break;
        }

        if (b10) {
            if (btile) {
                b10tile_unpack_mch(&seq_src, &seq_b16);
            } else {
                b10_unpack_mch(&seq_src, &seq_b16);
            }
            
            
/*
            seq_b16.fp = fopen("seq_b16.yuv","wb");
            r = fwrite(seq_b16.pbuf, seq_b16.io_size, 1, seq_b16.fp);
            if (r<1) {
                printf("error writing file\n");
                break;
            }
            fclose(seq_b16.fp);
*/
            
            b10_clip8_mch(&seq_b16, &seq_dst);
            b8mch_spliting(&seq_dst, &seq_spl);
        } else {
            if (btile) {
                b8tile_rescan_mch(&seq_src, &seq_dst);
                b8mch_spliting(&seq_dst, &seq_spl);
            } else {
                b8mch_spliting(&seq_src, &seq_spl);
            }
        }
        
        r = fwrite(seq_spl.pbuf, seq_spl.io_size, 1, seq_dst.fp);
      
        if (r<1) {
            printf("error writing file\n");
            break;
        }
    }

    if(seq_src.pbuf)    free(seq_src.pbuf);
    if(seq_b16.pbuf)    free(seq_b16.pbuf);
    if(seq_dst.pbuf)    free(seq_dst.pbuf);
    if(seq_spl.pbuf)    free(seq_spl.pbuf);
    
    if(seq_src.fp)      fclose(seq_src.fp);
    if(seq_dst.fp)      fclose(seq_dst.fp);

    return 0;
}
