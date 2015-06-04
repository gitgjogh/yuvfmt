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
    
    
int sat_div(int num, int den);
int bit_sat(int nbit, int val);
int is_bit_aligned(int nbit, int val);
int is_in_range(int v, int min, int max);
int clip(int v, int minv, int maxv);

char* get_uint32 (char *str, uint32_t *out);
int get_token_pos(const char* str, int search_from,
                   const char* prejumpset,
                   const char* delemiters,
                   int *stoken_start);

int str_2_field(char *record, int arrSz, char *fieldArr[]);
const char *search_in_fields(const char *filed, const char *record);

#endif  // __SIM_UTILS_H__
