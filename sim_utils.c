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

#include <limits.h>
#include <string.h>
#include <math.h>

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

int get_1st_field(const char* str, int search_from,
                       const char* prejumpset,
                       const char* delemiters,
                       int *field_start)
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

    if (field_start) {
        *field_start = start;
    }

    for (end = start; (c = str[end]); ++end) {
        if ( delemiters && strchr(delemiters, c) ) {
            break;
        }
    }

    return end - start;
}
                       
int str_2_fields(char *record, int arrSz, char *fieldArr[])
{
    int nkey=0, keylen=0, pos=0;
    keylen = get_1st_field(record, 0, " ", " ,", &pos);
    while (keylen && nkey<arrSz) {
        fieldArr[nkey++] = &record[pos];
        if (record[pos+keylen] == 0) {
            break;
        } else {
            record[pos+keylen] = 0;
            keylen = get_1st_field(record, pos+keylen+1, " ,", " ,", &pos);
        }
    }
    return nkey;
}

const char *field_in_record(const char *field, const char *record)
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



/**
 *  @brief scan for uint64_t without numerical base prefix.
 *  
 *  @param [i] _str
 *  @param [o] ret_
 *  @param [o] end_
 *  @return error flag during scanning
 */
int scan_for_uint64_npre(const char *_str, uint32_t base, uint64_t max, uint64_t *ret_, const char **end_)
{
    uint64_t acc = 0;
    uint32_t c, v;
    uint32_t b_err = 0;
    uint64_t max_div;
    uint64_t max_mod;

    const char *end = _str;
    while (c = *end) 
    {
        v = isdigit(c) ? (c - '0'     ) : (
            islower(c) ? (c - 'a' + 10) : (
            isupper(c) ? (c - 'A' + 10) : 16) );
            
        if (v>=16) {
            xdbg("scan_for_uint64_base%d() stop at `0x%02x`\n", base, c);
            break;
        }

        /* took unwanted digit as err */
        if (v>=base) {
            xerr("`%c` is not charset for base %d\n", c, base);
            b_err |= SCAN_ERR_OUTOFSET;
            break;
        }

        max = max ? max : UINT64_MAX;
        max_div = max / base;
        max_mod = max % base;
        if ( !b_err ) {
            int b_of = (acc > max_div) || (acc == max_div && c > max_mod);
            if (b_of) {
                xerr("`%8s...` to uint64 would overflow\n", _str);
                b_err |= SCAN_ERR_OVERFLOW;
            }
            acc = b_err ? max : acc * base + v;
        }

        ++ end;
    }
    
    if (end<=_str) {
        xerr("`%4s...` not seems int type\n", _str);
        b_err |= SCAN_ERR_NOBEGIN;
    }

    if (ret_) { *ret_ = acc; }
    if (end_) { *end_ = end; }

    return b_err;
}

int scan_for_uint64_pre(const char *_str, uint64_t max, uint64_t *ret_, const char **end_)
{
    uint32_t base = 10;
    
    // first non space;
    while ( isspace(*_str) ) { ++_str; }
    
    if (*_str == '0') {
        uint32_t n = *(_str+1);
        if ( n=='x' || n=='X' ) {
            base = 16;  _str += 2;
        } else 
        if ( n=='b' || n=='B' ) {
            base = 2;   _str += 2;
        } else {
            base = 8;
        }
    }
    return scan_for_uint64_npre(_str, base, max, ret_, end_);
}

static int scan_for_uintN(const char *_str, int Nbit, uint64_t *ret_, const char **end_)
{
    uint64_t ret;
    uint64_t max;
    int b_err = 0;
    const char *end;
 
    // first non space;
    while ( isspace(*_str) ) { ++_str; }

    // sign flag
    if (*_str == '+') {
        ++_str;
    } 
    max = ( Nbit == 64 ? UINT64_MAX : (
            Nbit == 32 ? UINT32_MAX : (
            Nbit == 16 ? UINT16_MAX : (
            Nbit ==  8 ? UINT8_MAX : 0))));

    b_err = scan_for_uint64_pre(_str, max, &ret, &end);

    if (ret_) { *ret_ = ret; }
    if (end_) { *end_ = end; }

    return b_err;
}

static int scan_for_intN(const char *_str, int Nbit, int64_t *ret_, const char **end_)
{
    uint64_t ret;
    uint64_t max;
    uint32_t b_neg = 0;
    int b_err = 0;
    const char *end;
 
    // first non space;
    while ( isspace(*_str) ) { ++_str; }

    // sign flag
    if (*_str == '-' || *_str == '+') {
        b_neg = (*_str == '-');
        ++_str;
    } 
    max = ( Nbit == 64 ? INT64_MAX : (
            Nbit == 32 ? INT32_MAX : (
            Nbit == 16 ? INT16_MAX : (
            Nbit ==  8 ? INT8_MAX : 0))));
    max += (b_neg ? 1 : 0);

    b_err = scan_for_uint64_pre(_str, max, &ret, &end);

    if (ret_) { *ret_ = b_neg ? -ret : ret; }
    if (end_) { *end_ = end; }

    return b_err;
}

int scan_for_uint64(const char *_str, uint64_t *ret_, const char **end_)
{
    uint64_t ret;
    int b_err = scan_for_uintN(_str, 64, &ret, end_);
    if (ret_) { *ret_ = ret; }

    return b_err;
}
int scan_for_uint32(const char *_str, uint32_t *ret_, const char **end_)
{
    uint64_t ret;
    int b_err = scan_for_uintN(_str, 32, &ret, end_);
    if (ret_) { *ret_ = ret; }

    return b_err;
}
int scan_for_uint16(const char *_str, uint16_t *ret_, const char **end_)
{
    uint64_t ret;
    int b_err = scan_for_uintN(_str, 16, &ret, end_);
    if (ret_) { *ret_ = ret; }

    return b_err;
}
int scan_for_int64(const char *_str, int64_t *ret_, const char **end_)
{
    int64_t ret;
    int b_err = scan_for_intN(_str, 64, &ret, end_);
    if (ret_) { *ret_ = ret; }

    return b_err;
}
int scan_for_int32(const char *_str, int32_t *ret_, const char **end_)
{
    int64_t ret;
    int b_err = scan_for_intN(_str, 32, &ret, end_);
    if (ret_) { *ret_ = ret; }

    return b_err;
}
int scan_for_int16(const char *_str, int16_t *ret_, const char **end_)
{
    int64_t ret;
    int b_err = scan_for_intN(_str, 16, &ret, end_);
    if (ret_) { *ret_ = ret; }

    return b_err;
}

int scan_for_float(const char *_str, float *ret_, const char **end_)
{
    double   ret;
    uint64_t r_int=0, r_frac=0;
     int32_t r_exp=0;                       // ret value
    uint32_t n_int=0, n_frac=0, n_exp=0;    // num of digit
    uint32_t e_int =SCAN_ERR_NOBEGIN;       // err flag
    uint32_t e_frac=SCAN_ERR_NOBEGIN;
    uint32_t e_exp =SCAN_ERR_NOBEGIN;
    uint32_t b_neg = 0;
    uint32_t b_err = 0;
    const char *end;
    
    // first non space;
    while ( isspace(*_str) ) { ++_str; }
    // sign flag
    if (*_str == '-' || *_str == '+') {
        b_neg = (*_str == '-');
        ++_str;
    }   
    
    e_int = scan_for_uint64_npre(_str, 10, INT_MAX, &r_int, &end);
    n_int = end - _str; 
    
    if (*end == '.') {
        _str = end + 1;
        e_frac = scan_for_uint64_npre(_str, 10, INT_MAX, &r_frac, &end);
        n_frac = end - _str;
    }
    
    if ( SCAN_ERR_NOBEGIN & (e_int & e_frac) ) {
        b_err = SCAN_ERR_NOBEGIN;
        goto str2float_end;
    } else {
        b_err = ( SCAN_ERR_OVERFLOW & (e_int | e_frac) );
    }
    
    if (*end == 'e' || *end == 'E') {
        _str   = end + 1;
        e_exp  = scan_for_int32(_str, &r_exp, &end);
        e_exp |= (r_exp>37 || r_exp<-37) ? SCAN_ERR_OVERFLOW : 0;
        b_err |= e_exp;
        r_exp  = e_exp ? 0 : r_exp;
    }
    
str2float_end:
    //printf("int=%-10lld, frac=%lf, exp=%lf, ", 
    //    r_int, r_frac / pow(10, n_frac), pow(10, r_exp));

    ret = (double)r_int + (double)r_frac / pow((double)10.0, (double)n_frac);
    ret = ret * pow((double)10.0, (double)r_exp);

    if (ret_) { *ret_ = (float)(b_neg ? -ret : ret); }
    if (end_) { *end_ = end; }
    
    return b_err;
}

int str_2_uint(const char *_str, unsigned int *ret_)
{
    uint64_t ret;
    const char *end;
    int N = sizeof(unsigned int) * 8;
    int b_err = scan_for_uintN(_str, N, &ret, &end);
    if (!b_err && (*end != 0)) {
        xerr("str_2_uint(%s) not nul ending\n", _str);
        b_err |= SCAN_ERR_NNULEND;
    }
    if (ret_) { *ret_ = ret; }

    return b_err;
}
int str_2_int(const char *_str, int *ret_)
{
    int64_t ret;
    const char *end;
    int N = sizeof(unsigned int) * 8;
    int b_err = scan_for_intN(_str, N, &ret, &end);
    if (!b_err && (*end != 0)) {
        xerr("str_2_int(%s) not nul ending\n", _str);
        b_err |= SCAN_ERR_NNULEND;
    }
    if (ret_) { *ret_ = ret; }

    return b_err;
}


/*
int scan_for_token(const char *_str, int b_first_alpha, int *pos1_, int *pos2_, const char **end_)
{
    uint32_t ret1, ret2;
    const char *end = _str;
    uint32_t b_err = 0;
    uint32_t b_open;
    
    // first non space;
    while ( isspace(*end) ) { ++end; }
    ret1 = ret2 = (end - _str);
    b_open = b_first_alpha ? isalpha(*end) : isalnum(*end);
    
    // open
    if (b_open || *end=='_') {
        b_err = 0;
        ret1 = (end++) - _str;
    } else {
        b_err |= SCAN_ERR_NOBEGIN;
        goto scanforidentifier_end;
    }

    //close
    while (*end!=0 && (isalnum(*end) || *end=='_')) { ++end; }
    ret2 = end - _str;

scanforidentifier_end:
    if (pos1_) { *pos1_ = ret1; }
    if (pos2_) { *pos2_ = ret2; }
    if (end_)  { *end_  = end; }
    
    return b_err;
}
*/