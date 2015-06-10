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

#ifndef __SIM_BITS_H__
#define __SIM_BITS_H__

#include <stdint.h>

typedef enum {
    SBS_OK  = 0,
    SBS_ERR = (1<<0),
    SBS_EOS = (1<<1),
    SBS_BUG = (1<<2),
} sbs_state_t;

typedef struct simple_bitstream 
{
    struct {
        uint8_t     *base;
        uint32_t    size;
    } buf;

    struct {
        uint32_t    wbyte;          /** Next bytePos to be wrote out */
        uint32_t    rbyte;          /** Next bytePos to be read in */
        
        uint32_t    reg1;           /** Cached reg1 */
        uint32_t    reg2;           /** Cached reg2 */
        uint32_t    nbit;           /** Num of bits in cache. Normally 64, 
                                         or else EOS is nearly. */
        uint32_t    ibit;           /** Bitpos in reg1. 0~31, or else bug */
    } cache;
    
    uint32_t        state;
} 
sim_bs_t;

uint32_t    sbs_initbits(sim_bs_t *bs, uint8_t *base, uint32_t size);
uint32_t    sbs_seek2pos(sim_bs_t *bs, uint32_t bytePos, uint32_t bitPos);          
uint32_t    sbs_byte_pos(sim_bs_t *bs);
uint32_t    sbs_bit_pos (sim_bs_t *bs);

uint32_t    sbs_nextbits(sim_bs_t *bs, uint8_t *base, uint32_t size);

/**
 *  \brief 
 *      Read `nbit` bits to the LSBs of @pOut
 *      - If (nbitleft<nbit), pad (nbit-nbitleft) bits '0' to the LSBs.
 *  \param [o] pOut 
 *      Point to an uint32_t value who's LSBs are the read bits.
 *  \param [i] bs 
 *  \param [i] nbit 
 *      Number of bits to be read [0,32]
 *  \return 
 *      Number of bits actually read.
 */
int sbs_showbits(uint32_t *pOut, const sim_bs_t *bs, uint32_t nbit);
int sbs_showbits1(uint32_t *pOut, const sim_bs_t *bs);

/**
 *  \brief 
 *      Update bit-stream context : bs->cache.ibit += nbit
 *  \return 
 *      - SBS_OK
 *      - SBS_ERR              # @see bs->state
 *  \note
 *      If nbit cross the EOS, bs->cache.ibit is not changed.
 *      So that after switching to a new stream buffer with
 *      @ref sbs_nextbits(), the state can be continue.
 */
uint32_t sbs_flushin(sim_bs_t *bs, uint32_t nbit);

/**
 *  \brief Read @nbit to the LSB(s) of @pOut, and then update `bs`.
 *  \param [o] pOut
 *      Point to an uint32_t value who's LSBs are the read bits.
 *  \param [i] bs
 *      `bs->cache.ibit` will be updated after reading.
 *  \param [i] nbit 
 *      Number of bits to be read [0,32]
 *  \return 
 *      - SBS_OK
 *      - SBS_ERR              # end of stream, pOut would be meaningless
 *  \note
 *      bs->state can be help to indicate sth. beside return value
 *      - SBS_OK / VSSP_EOU / SBS_EOS
 */
uint32_t sbs_getbits(uint32_t *pOut, sim_bs_t *bs, uint32_t nbit);
uint32_t sbs_getbit1(uint32_t *pOut, sim_bs_t *bs);

uint32_t sbs_coverbits(sim_bs_t *bs, uint32_t val, int nbit, int b_force_flush);
uint32_t sbs_putbits(sim_bs_t *bs, uint32_t val, int nbit);
uint32_t sbs_flushout(sim_bs_t *bs);


#endif  // __SIM_BITS_H__
