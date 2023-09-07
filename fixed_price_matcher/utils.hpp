#pragma once
#include <iostream>
#include <cstdint>
#include <cmath>

static inline uint64_t __UMLH(uint32_t a, uint32_t b) {
    return static_cast<uint64_t>(a) * static_cast<uint64_t>(b) >> 32;
}

static inline uint64_t __UMULH(uint64_t n1, uint64_t n2) {
    return static_cast<uint64_t>((static_cast<__uint128_t>(n1) * static_cast<__uint128_t>(n2)) >> 64);
}

static int64_t SHIFT(int64_t n, int cnt) {
    switch (cnt) {
        case -1:
            return n/10 + (n%10>=5?1:0);
            break;
        case -3:
            return n/1000 + (n%1000>=500?1:0);
            break;
        case -5:
            return n/100000 + (n%100000>=50000?1:0);
            break;
        case -8:
            return n/100000000 + (n%100000000>=50000000?1:0);
            break;
        case 5:
            return n*100000;
            break;
        default:
            return ((cnt==0)
                    ?
                    n
                    :
                    (cnt>0)
                    ?
                    (SHIFT(10*n, cnt-1))
                    :
                    (SHIFT((n + (((n%10)>=5&&cnt==-1)?10:0))/10, cnt+1)));
    }
}

static uint64_t USHIFT(uint64_t n, int cnt) {
    switch (cnt) {
        case -1:
            return n/10 + (n%10>=5?1:0);
            break;
        case -3:
            return n/1000 + (n%1000>=500?1:0);
            break;
        case -5:
            return n/100000 + (n%100000>=50000?1:0);
            break;
        case -8:
            return n/100000000 + (n%100000000>=50000000?1:0);
            break;
        case 5:
            return n*100000;
            break;
        default:
            return ((cnt==0)
                    ?
                    n
                    :
                    (cnt>0)
                    ?
                    (SHIFT(10*n, cnt-1))
                    :
                    (SHIFT((n + (((n%10)>=5&&cnt==-1)?10:0))/10, cnt+1)));
    }
}

static int ngts_math_ulog10(uint64_t x) {
    int cnt = -1;
    while (x) {
        x = USHIFT(x, -1);
        cnt++;
    }
    return cnt;
}

static double DIVD(double dividend, double divisor) {
    return std::round(dividend / divisor * 100000.0) / 100000.0;
}

static int64_t MULT(int64_t x, int64_t y, int decimal_places) {
    int64_t result = x * y;  // 计算乘积
    int64_t integer_part = std::trunc(result / std::pow(10, decimal_places));  // 截取小数点前的整数部分
    int64_t decimal_part = result % (int64_t)std::pow(10, decimal_places);  // 截取小数点后的部分
    return integer_part * (int64_t)std::pow(10, decimal_places) + (decimal_part >= 0 ? decimal_part : -decimal_part);  // 拼接整数和小数部分并返回
}