#include <stdio.h>
#include <stdint.h>
#include <string.h>

#define SIZE 35
#define DEVIS 1000000000
    
typedef struct {
    uint32_t block[SIZE];
}uint1024_t;

uint1024_t from_uint(unsigned int x) {
    uint1024_t num;
    memset(&num, 0, sizeof(uint32_t)*SIZE);
    for (int i = 0; i < SIZE; i++) {
        num.block[i] = x % DEVIS;
        x = x/DEVIS;
        if (x == 0) {
            break;
        }
    }

    return num; 
}

uint1024_t add_op (uint1024_t x, uint1024_t y) {
    uint1024_t result;
    memset(&result, 0, sizeof(uint32_t)*SIZE);
    for (int i = 0; i < SIZE; i++) {
        result.block[i] += (x.block[i] + y.block[i])%DEVIS;
        if (i != SIZE-1) {
            result.block[i+1] = (x.block[i] + y.block[i])/DEVIS ;
        }
    }
    return result;
}

int cmp_op(uint1024_t x, uint1024_t y) {
    for (int i = SIZE - 1; i >= 0; i--) {
        if (x.block[i] > y.block[i]) {
            return 1;
        }
        else if (y.block[i] > x.block[i]) {
            return 2;
        }
    }
    return 0;
}

uint1024_t subtr_op (uint1024_t x, uint1024_t y) {
    uint1024_t result, buf;
    uint64_t tmp;
    int j;
    memset(&result, 0, sizeof(uint32_t)*SIZE);
    int flag = cmp_op(x, y);
    if (flag == 0) {
        return result;
    }
    else if (flag == 2) {
        buf = x;
        x = y;
        y = buf;
    }
    for (int i = 0; i < SIZE; i++) {
        if (x.block[i] < y.block[i]) { 
            tmp = (DEVIS + x.block[i] - y.block[i])%DEVIS;
            result.block[i] = tmp;
            if (i != SIZE-1) {
                j = i + 1;
                while (x.block[j] == 0) {
                    x.block[j] = 999999999;
                    j++;
                }
                x.block[j]--;
            }
        }
        else {
            result.block[i] = x.block[i] - y.block[i];
        }
    }
    return result;
}

uint1024_t mult_op (uint1024_t x, uint1024_t y) {
    uint1024_t result;
    uint64_t mult;
    int index;
    memset(&result, 0, sizeof(uint32_t)*SIZE);
    for (int i = 0; i < SIZE; i++) {
        for (int j = 0; j + i < SIZE; j++) {
            if (x.block[j] && y.block[i]) {
                mult = ((uint64_t)x.block[j])*y.block[i];
                index = i + j; 
                while (mult) {
                    result.block[index] += mult%DEVIS;
                    mult /= DEVIS;
                    if (result.block[index] >= DEVIS) {
                        result.block[index] %= DEVIS;
                        mult += 1;
                    }
                    index++;
                }
            }
        }
    }
    return result;

}

void printf_value (uint1024_t x) {
    int i = SIZE - 1;
    while (i >= 0 && x.block[i] == 0) {
        i--;
    }

    if (i < 0) {
        printf("0");
        return;
    }

    printf("%I32u", x.block[i--]);
    
    while (i >= 0) {
        printf("%09I32u", x.block[i--]);
    }
}

void sscanf_value(char *str, uint1024_t *x) {
    memset(x, 0, sizeof(uint32_t)*SIZE);
    int len = strlen(str);
    int pos, idx;
    char chunk[10]; chunk[9]='\0';
    for (pos = len-9, idx = 0; pos >= 0; pos -= 9, idx++) {
        strncpy(chunk, str+pos, 9);
        x->block[idx] = atoi(chunk);
    }
    if (pos%9 != 0) {
        int rest = (pos+9)%9;
        strncpy(chunk, str, rest);
        chunk[rest] = '\0';
        x->block[idx] = atoi(chunk);
    }
}

void scanf_value(uint1024_t *x) {
    char str[310]; 
    scanf("%s", str);
    sscanf_value(str, x);
}

int main() {

    uint1024_t x, y;
    scanf_value(&x);
    scanf_value(&y);
    printf_value(x);
    printf("\n");
    printf_value(y);
    printf("\n");
    printf_value(mult_op(x, y));
    printf("\n");
    printf_value(add_op(x, y));
    printf("\n");
    printf_value(subtr_op(x, y));
    printf("\n");
    return 0;
}