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

#ifndef __SIM_UTILS_H__
#define __SIM_UTILS_H__


#include <stdint.h>


#ifndef MAX_PATH
#define MAX_PATH    (256)
#endif

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(a)   (sizeof(a)/sizeof(a[0]))
#endif

#ifndef MAX
#define MAX(a,b) ((a)>(b) ? (a) : (b))
#endif

#ifndef MIN
#define MIN(a,b) ((a)<(b) ? (a) : (b))
#endif

#ifndef CLIP
#define CLIP(v, min, max) \
    (((min) > (max)) ? (v) : (((v)<(min)) ? (min) : ((v)>(max) ? (max) : (v))))
#endif

#define IS_IN_RANGE(v, min, max) \
    (((min)<=(max)) && ((min)<=(v)) && ((max)>=(v)))

#define IS_LTE()        ((uint8_t)((uint32_t)0x00000001))
#define IS_BGE()        ((uint8_t)((uint32_t)0x01000000))
void mem_put_lte16(uint8_t *mem, uint32_t val);
void mem_put_lte32(uint8_t *mem, uint32_t val);

#define LSBSMASK(nbit)                  (((1<<(nbit)) - 1))
#define BITSMASK(ibit, nbit)            (((1<<(nbit)) - 1) << (ibit))

#define CLEARBITS(val, ibit, nbit)      ((val) & ~BITSMASK(ibit, nbit))
#define CLEARLSBS(val, nbit)            ((val) >> (nbit) << (nbit))
#define CLEARMSBS(val, nbit)            ((val) << (nbit) >> (nbit))

#define GETBITS(val, ibit, nbit)        (((val)>>(ibit)) & LSBSMASK(nbit))
#define GETLSBS(val, nbit)              ((val) & LSBSMASK(nbit))
#define GETMSBS(val, nbit)              ((val)>>(32-(nbit)))
#define GETBIT(val, ibit)               (((val)>>(ibit)) & 1)

#define SETBITS(val, ibit, nbit, src)   (CLEARBITS(val, ibit, nbit) | (GETLSBS(src, nbit)<<(ibit)))
#define SETLSBS(val, nbit, src)         (CLEARMSBS(val, nbit) | GETLSBS((src), (nbit)))
#define SETMSBS(val, nbit, src)         (CLEARMSBS(val, nbit) | ((src)<<(32-(nbit))))

#define ENDIAN_REVERSE_32(val)          (((val)<<24)|(((val)&0xff00)<<16)|(((val)&0xff000)>>8)|((val)>>24))
#define BGE2LTE_32      ENDIAN_REVERSE_32
#define LTE2BGE_32      ENDIAN_REVERSE_32
    
int sat_div(int num, int den);
int bit_sat(int nbit, int val);
int is_bit_aligned(int nbit, int val);
int is_in_range(int v, int min, int max);
int clip(int v, int minv, int maxv);

int num_leading_zero_bits_u64(uint64_t val);
int num_leading_zero_bits_u32(uint32_t val);
#define NLZB32(val)     (num_leading_zero_bits_u32(val))
#define NLZB64(val)     (num_leading_zero_bits_u64(val))

char* get_uint32 (char *str, uint32_t *out);
int get_1st_field(const char* str, int search_from,
                   const char* prejumpset,
                   const char* delemiters,
                   int *field_start);

int str_2_fields(char *record, int arrSz, char *fieldArr[]);
const char *field_in_record(const char *filed, const char *record);

typedef enum {
    SCAN_OK = 0,
    SCAN_ERR_OVERFLOW   = (1<<1),
    SCAN_ERR_OUTOFSET   = (1<<2),
    SCAN_ERR_NOBEGIN    = (1<<3),
    SCAN_ERR_NOCLOSE    = (1<<5),
    SCAN_ERR_NNULEND    = (1<<6),
}scan_ret_t;

/**
 * @param [i] max  
 * @param [o] ret_ only useful if (@return == 0)
 * @param [o] end_ 
 * @return: 0 if success, else @see scan_ret_t
 */
int scan_for_uint64_npre(const char *_str, uint32_t base, uint64_t max, uint64_t *ret_, const char **end_);
int scan_for_uint64_pre(const char *_str, uint64_t max, uint64_t *ret_, const char **end_);
int scan_for_uint64(const char *_str, uint64_t *ret_, const char **end_);
int scan_for_uint32(const char *_str, uint32_t *ret_, const char **end_);
int scan_for_uint16(const char *_str, uint16_t *ret_, const char **end_);
int scan_for_int64(const char *_str, int64_t *ret_, const char **end_);
int scan_for_int32(const char *_str, int32_t *ret_, const char **end_);
int scan_for_int16(const char *_str, int16_t *ret_, const char **end_);
int scan_for_float(const char *_str, float *ret_, const char **end_);
int str_2_uint(const char *_str, unsigned int *ret_);
int str_2_int(const char *_str, int *ret_);

#endif  // __SIM_UTILS_H__
