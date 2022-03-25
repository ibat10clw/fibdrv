#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define SIZE 110


void add(char *a, char *b, char *out)
{
    int len_a = strlen(a);
    int len_b = strlen(b);
    int i = 0;
    int carry = 0;
    int diff = len_a - len_b;
    for (i = len_b - 1; i >= 0; i--) {
        int sum = (a[i + diff] - '0') + (b[i] - '0') + carry;
        out[i + diff] = '0' + sum % 10;
        carry = sum / 10;
    }
    for (i = diff - 1; i >= 0; i--) {
        int sum = (a[i] - '0') + carry;
        out[i] = '0' + sum % 10;
        carry = sum / 10;
    }
    if (carry) {
        if (diff == 0)
            len_a++;
        for (i = len_a; i >= 0; i--) {
            out[i + 1] = out[i];
        }
        out[len_a + 1] = 0;
        out[0] = '0' + carry;
    }
    out[len_a] = 0;
}
void sub(char *a, char *b, char *out)
{
    int len_a = strlen(a);
    int len_b = strlen(b);
    int i = 0;
    int borrow = 0;
    int idx = len_a - len_b;
    for (i = len_b - 1; i >= 0; i--) {
        int diff = (a[i + idx] - '0') - (b[i] - '0') - borrow;
        if (diff < 0) {
            diff += 10;
            borrow = 1;
        } else
            borrow = 0;
        out[i + idx] = '0' + diff;
    }
    for (i = idx - 1; i >= 0; i--) {
        int diff = (a[i] - '0') - borrow;
        if (diff < 0) {
            diff += 10;
            borrow = 1;
        } else
            borrow = 0;
        out[i] = '0' + diff;
    }
    out[len_a] = 0;
    i = 0;
    while (out[i] == '0')
        i++;
    if (i != 0) {
        for (int j = 0; j < len_a; ++j)
            out[j] = out[j + i];
    }
}
void mul(char *a, char *b, char *out)
{
    int len_a = strlen(a);
    int len_b = strlen(b);
    int N = len_a + len_b;
    int i;
    memset(out, '0', N);
    out[N] = 0;
    for (i = len_b - 1; i >= 0; i--) {
        for (int j = len_a - 1; j >= 0; j--) {
            int digit1 = a[j] - '0';
            int digit2 = b[i] - '0';
            int pos = 1 + i + j;
            int carry = out[pos] - '0';
            int multiplication = digit1 * digit2 + carry;
            out[pos] = (multiplication % 10) + '0';
            out[pos - 1] += (multiplication / 10);
        }
    }

    i = 0;
    while (out[i] == '0')
        i++;
    if (i != 0) {
        for (int j = 0; j < N; ++j)
            out[j] = out[j + i];
    }
}

static long fast_fib(int k, char *buf)
{
    int stack[100];
    int top = 0;
    while (k > 1) {
        stack[top++] = k;
        k /= 2;
    }
    char f1[SIZE] = "1";
    char f2[SIZE] = "1";
    char tmp1[SIZE];
    char tmp2[SIZE];
    char res1[SIZE];
    while (top--) {
        int n = stack[top];
        // f1 = f1 * (2 * f2 - f1)
        mul("2", f2, tmp1);
        sub(tmp1, f1, tmp2);
        mul(tmp2, f1, res1);
        // f2 = f1 ** 2 + f2 ** 2
        mul(f1, f1, tmp1);
        mul(f2, f2, tmp2);
        add(tmp2, tmp1, f2);
        if (n % 2 == 1) {
            add(f2, res1, tmp1);
            strncpy(f1, f2, strlen(f2) + 1);
            strncpy(f2, tmp1, strlen(tmp1) + 1);
        } else {
            strncpy(f1, res1, strlen(res1) + 1);
        }
    }
    strncpy(buf, f1, strlen(f1) + 1);
    return 0;
}
int main()
{
    char buf[SIZE];
    fast_fib(500, buf);
    printf("%s\n", buf);
    return 0;
}