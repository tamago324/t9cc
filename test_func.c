#include <stdio.h>

void foo() { printf("OK\n"); }
void hoge(int a) { printf("OK, %d\n", a); }
void bar(int a, int b) { printf("OK, %d\n", a + b); }
void huga(int a, int b, int c, int d, int e, int f) {
  printf("OK, %d\n", a + b + c + d + e + f);
}