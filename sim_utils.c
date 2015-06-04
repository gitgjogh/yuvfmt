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
#include <limits.h>
#include <string.h>
#include <stdio.h>

#include "sim_utils.h"


int is_in_range(int min, int max, int v)
{
    return (((min)<=(max)) && ((min)<=(v)) && ((max)>=(v)));
}

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


int clip(int v, int minv, int maxv)
{
    v = (v<minv) ? minv : v;
    v = (v>maxv) ? maxv : v;
    return v;
}


char* get_uint32 (char *str, uint32_t *out)
{
    char  *curr = str;
    
    if ( curr ) {
        int  c, sum;
        for (sum=0, curr=str; (c = *curr) && c >= '0' && c <= '9'; ++ curr) {
            sum = sum * 10 + ( c - '0' );
        }
        if (out) { *out = sum; }
    }

    return curr;
}

int get_token_pos(const char* str, int search_from,
                       const char* prejumpset,
                       const char* delemiters,
                       int *stoken_start)
{
    int c, start, end;

    if (prejumpset) 
    {
        for (start = search_from; (c = str[start]); ++start) {
            if ( !strchr(prejumpset, c) ) {
                break;
            }
        }
    }

    if (stoken_start) {
        *stoken_start = start;
    }

    for (end = start; (c = str[end]); ++end) {
        if ( delemiters && strchr(delemiters, c) ) {
            break;
        }
    }

    return end - start;
}
                       
int str_2_field(char *record, int arrSz, char *fieldArr[])
{
    int nkey=0, keylen=0, pos=0;
    keylen = get_token_pos(record, 0, " ", " ,", &pos);
    while (keylen && nkey<arrSz) {
        fieldArr[nkey++] = &record[pos];
        if (record[pos+keylen] == 0) {
            break;
        } else {
            record[pos+keylen] = 0;
            keylen = get_token_pos(record, pos+keylen+1, " ,", " ,", &pos);
        }
    }
    return nkey;
}

const char *search_in_fields(const char *field, const char *record)
{
    const char *curr=0, *next=0;
    int flen = strlen(field);
    
    for(curr=record; curr!=0; curr=next)
    {
        next = strstr(curr, field);
        if (next) 
        {
            char s = (next==record ? 0 : next[-1]);
            char e = next[flen+1];
            if ((s==0 || s==',' || s==' ') && 
                (e==0 || e==',' || e==' ')) 
            {
                return next;
            }
        }   
    }
    
    return 0;
}
