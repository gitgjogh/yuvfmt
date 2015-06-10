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

#include <string.h>
#include "sim_utils.h"
#include "sim_bits.h"

#ifndef ASSERT
#include <assert.h>
#define ASSERT assert
#endif


int sbs_fill_cache(sim_bs_t *bs)
{   
    xdbg("@sbs>> bs@0x%08x[0x%08x], pre cache(0x-%08x-%08x)\n", 
        bs->buf.base, bs->cache.rbyte, bs->cache.reg1, bs->cache.reg2);

    if (bs->cache.nbit & 7 || bs->cache.nbit > 64) {
        xerr("@sbs>> cache.nbit = %d\n", bs->cache.nbit);
        bs->state |= SBS_BUG;
        return SBS_BUG;
    }

    if (bs->cache.rbyte >= bs->buf.size) {
        xerr("@sbs>> already EOS\n");
        bs->state |= SBS_EOS;
        return SBS_EOS;
    }

    if (bs->state != SBS_OK) {
        xerr("@sbs>> invalid state (%d)\n", bs->state);
        return  SBS_ERR;
    }

    while((bs->cache.nbit < 64) && (bs->cache.rbyte < bs->buf.size)) 
    {
        uint32_t currByte = bs->buf.base[ bs->cache.rbyte ++ ];
        bs->cache.nbit += 8;
        if (bs->cache.nbit <= 32) {
            bs->cache.reg1 |= currByte<<(32 - bs->cache.nbit);
        } else {
            bs->cache.reg2 |= currByte<<(64 - bs->cache.nbit);
        }
    }

    xdbg("@sbs>> bs@0x%08x[0x%08x], bre cache(0x-%08x-%08x)\n", 
        bs->buf.base, bs->cache.rbyte, bs->cache.reg1, bs->cache.reg2);

    bs->state |= (bs->cache.rbyte < bs->buf.size) ? bs->state : SBS_EOS;
    return SBS_OK;
}

uint32_t sbs_seek2pos(sim_bs_t *bs, uint32_t bytePos, uint32_t ibit)
{
    xdbg("@sbs>> bs@0x%08x[:0x%08x:], jumm2pos(0x%08x:%d)\n", 
        bs->buf.base, bs->buf.size, bytePos, ibit);

    bytePos += (ibit >> 3);
    ibit   = (ibit &  7);
    if (bytePos >= bs->buf.size) {
        xerr("@sbs>> cross EOS\n");
        return SBS_EOS;
    }

    bs->state       = SBS_OK;
    bs->cache.rbyte   = bytePos;
    bs->cache.wbyte   = bytePos;

    bs->cache.ibit  = ibit;
    bs->cache.nbit  = 0;
    bs->cache.reg1  = 0;
    bs->cache.reg2  = 0;
    return sbs_fill_cache(bs);
}

uint32_t sbs_initbits(sim_bs_t *bs, uint8_t *base, uint32_t size)
{
    xdbg("@sbs>> init @0x%08x[:%08x:] \n", base, size);

    memset(bs, 0, sizeof(sim_bs_t));
    bs->buf.base    = base;
    bs->buf.size    = size;
    bs->state       = SBS_OK;

    return sbs_fill_cache(bs);
}

/**
 *  |-------- reg1 ---------|-------- reg2 ---------|
 *  |---- ibit ---- |------ showbit32 ------| >>>>>>| (32-ibit)
 *                  |--- nbit ---|
 */
int sbs_showbits(uint32_t *pOut, const sim_bs_t *bs, uint32_t nbit)
{
    ASSERT(bs && bs->cache.ibit < 32 && nbit <= 32);

    if (pOut) { 
        /* showbit32() */
        uint32_t pos = (bs->cache.ibit);
        uint32_t out = (bs->cache.reg1 << pos)
                     | (bs->cache.reg2 >> (32 - pos)); 
        /* nbit-MSB of the showbit32() ret */
        *pOut = out >> (32 - nbit); 
    }

    return MIN(nbit, bs->cache.nbit - bs->cache.ibit);
}

int sbs_showbits1(uint32_t *pOut, const sim_bs_t *bs)
{
    ASSERT(bs && bs->cache.ibit < 32);

    if (pOut) { 
        uint32_t pos = (bs->cache.ibit);
        uint32_t out = (bs->cache.reg1 << pos) >> 31;
        *pOut = out;
    }

    return (bs->cache.ibit < bs->cache.nbit);
}

uint32_t sbs_flushin(sim_bs_t *bs, uint32_t nbit)
{
    ASSERT(bs && bs->cache.ibit < 32);

    bs->cache.ibit += nbit;

    if (bs->cache.ibit > bs->cache.nbit) {
        bs->cache.ibit -= nbit;         /* rewind to pre pose */
        return SBS_ERR;
    }

    if (bs->cache.ibit < 32) {
        return SBS_OK;
    } else {
        bs->cache.reg1  = bs->cache.reg2;
        bs->cache.reg2  = 0;
        bs->cache.ibit -= 32;
        bs->cache.nbit -= 32;
        return sbs_fill_cache(bs);
    }
}

uint32_t sbs_getbits(uint32_t *pOut, sim_bs_t *bs, uint32_t nbit)
{
    ASSERT(bs && bs->cache.ibit < 32 && nbit <= 32);

    nbit = (nbit < 32) ? nbit : 32;
    sbs_showbits(pOut, bs, nbit);
    return sbs_flushin(bs, nbit);
}

uint32_t sbs_getbit1(uint32_t *pOut, sim_bs_t *bs)
{
    ASSERT(bs && bs->cache.ibit < 32);
    sbs_showbits1(pOut, bs);
    return sbs_flushin(bs, 1);
}

/**
 *  |-------- reg1 ---------|-------- reg2 ---------|
 *  |---- ibit ---- |--- nbit ---|
 */
uint32_t sbs_coverbits(sim_bs_t *bs, uint32_t val, int nbit, int b_force_flush)
{
    int r = SBS_OK;
    uint32_t pos = (bs->cache.ibit & 31);
    /*
    val <<= (32-nbit);
    bs->cache.reg1 |= val >> (32-pos);
    bs->cache.reg2 = val << pos;
    */
    
    val = GETLSBS(val, nbit);
    uint32_t pos0 = bs->cache.ibit;
    uint32_t pos1 = bs->cache.ibit = pos0 + nbit;
    if (pos1 >= 32) {
        uint32_t val_msb = val >> (pos1 - 32);
        uint32_t val_lsb = val << (64 - pos1);
        SETLSBS(bs->cache.reg1, 32-pos0, val_msb);
        SETMSBS(bs->cache.reg2, pos1-32, val_lsb);

        mem_put_lte32(bs->buf.base + bs->cache.wbyte, bs->cache.reg1);
        
        bs->cache.wbyte  += 4;
        bs->cache.rbyte  += 4;
        bs->cache.reg1  = bs->cache.reg2;
        bs->cache.reg2  = 0;
        bs->cache.ibit -= 32;
        r = sbs_fill_cache(bs);
    } else {
        SETBITS(bs->cache.reg1, pos0, nbit, val);
    }
    
    if (b_force_flush) {
        mem_put_lte32(bs->buf.base + bs->cache.wbyte, bs->cache.reg1);
    }

    return r;
}


/**
 *  |-------- reg1 ---------|-------- reg2 ---------|
 *  |---- ibit ---- |--- nbit ---|
 */
uint32_t sbs_putbits(sim_bs_t *bs, uint32_t val, int nbit)
{
    uint32_t pos = (bs->cache.ibit & 31);
    /*
    val <<= (32-nbit);
    bs->cache.reg1 |= val >> (32-pos);
    bs->cache.reg2 = val << pos;
    */
    
    val = GETLSBS(val, nbit);
    
    bs->cache.ibit +=  nbit;
    if (bs->cache.ibit >= 32) {
        pos = bs->cache.ibit;
        bs->cache.reg1 |= val >> (pos - 32);
        bs->cache.reg2  = val << (64 - pos);

        uint32_t bge32 = bs->cache.reg1;
        uint32_t lte32 = BGE2LTE_32(bge32);
        *((uint32_t *)(bs->buf.base + bs->cache.wbyte)) = lte32;
        
        bs->cache.wbyte  += 4;
        bs->cache.rbyte  += 4;
        bs->cache.reg1  = bs->cache.reg2;
        bs->cache.reg2  = 0;
        bs->cache.ibit -= 32;
    } else {
        bs->cache.reg1 |= val << (32 - bs->cache.ibit);
    }

    return SBS_OK;
}

uint32_t sbs_flushout(sim_bs_t *bs)
{
    if (bs->cache.nbit > 0) {
        mem_put_lte32(bs->buf.base + bs->cache.wbyte, bs->cache.reg1);
    }
    
    if (bs->cache.nbit > 32) {
        mem_put_lte32(bs->buf.base + bs->cache.wbyte + 4, bs->cache.reg2);
    }
    
    return 0;
}
