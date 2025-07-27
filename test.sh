#!/bin/bash

assert() {
  # == 準備 ==
  expected="$1"
  input="$2"

  # 入力を渡して、アセンブリに変換
  ./9cc "$input" > tmp.s
  # 実行ファイル作成 (+テスト関数が書かれたオブジェクトファイルをリンク)
  cc -o tmp tmp.s
  
  # == 実行 ==
  ./tmp

  # == 検証 ==
  actual="$?"
  if [ "$actual" = "$expected" ]; then
    echo "$input => $actual"
  else
    echo "$input => $expected expected, but got $actual"
  fi
}

# assert 期待値 入力

assert 0 'int main() {return 0;}'
assert 42 'int main() {return 42;}'
assert 21 'int main() {return 5+20-4;}'
assert 51 'int main() {return 5 - 3 + 99 - 50;}'

# step5
assert 98 'int main() {return 200 - (3 + 99);}'
assert 7 'int main() {return 1+2*3;}'
assert 26 'int main() {return 2*3+4*5;}'
assert 2 'int main() {return 1+2+10-(2+3*3);}'

# step6
assert 10 'int main() {return -10+20;}'
assert 7 'int main() {return -(5+8)+20;}'

# step7
assert 0 'int main() {return 0==1;}'
assert 1 'int main() {return 42==42;}'
assert 1 'int main() {return 0!=1;}'
assert 0 'int main() {return 42!=42;}'

assert 1 'int main() {return 0<1;}'
assert 0 'int main() {return 1<1;}'
assert 0 'int main() {return 2<1;}'
assert 1 'int main() {return 0<=1;}'
assert 1 'int main() {return 1<=1;}'
assert 0 'int main() {return 2<=1;}'

assert 1 'int main() {return 1>0;}'
assert 0 'int main() {return 1>1;}'
assert 0 'int main() {return 1>2;}'
assert 1 'int main() {return 1>=0;}'
assert 1 'int main() {return 1>=1;}'
assert 0 'int main() {return 1>=2;}'

# step9
assert 3 'int main() {1;2; return 3;}'
assert 3 'int main() {int a; a=1; int b;b=2; return a+b;}'

# step10
assert 3 'int main() {int abc;abc=1; int def;def=2; return abc+def;}'
assert 6 'int main() {int a1; a1=1; int b2;b2=2; int c_3; c_3=3; return a1 + b2 + c_3;}'

# step11;
assert 3 'int main() {return 3;}'
assert 4 'int main() {int a;a=1; return a+3;}'
assert 14 'int main() {int a; a=3; int b; b=5*6-8; return a + b / 2;}'

# step12
assert 2 'int main() {if (1==1) return 2;}'

assert 3 'int main() {if (0) return 2; return 3;}'
assert 3 'int main() {if (1-1) return 2; return 3;}'
assert 2 'int main() {if (1) return 2; return 3;}'
assert 2 'int main() {if (2-1) return 2; return 3;}'

  # else
  assert 1 'int main() {if (0) return 3; else return 1;}'
  assert 3 'int main() {if (1) return 3; else return 1;}'

  # while
  assert 10 'int main() {int i;i=0; while(i<10) i=i+1; return i;}'

  # for
  assert 55 'int main() {int i; int j;j=0; for (i=0; i<=10; i=i+1) j=i+j; return j;}'
  assert 3 'int main() {for (;;) return 3; return 5;}'

# step13
assert 10 'int main() {int i;i=0; int j;j=0; for (i=0; i<10; i=i+1) {j=j+1;} return j;}'
assert 10 'int main() {{return 10;}}'

# step14
# assert 10 'int main(){foo(); return 10;}'

# assert 3 'int main(){bar(1, 2); return 3;}'
# assert 21 'int main(){huga(1, 2, 3, 4, 5, 6); return 21;}'

# step15
assert 1 'int main() {return 1;}'
assert 7 'int main() {return one()+two()*3;} int one(){return 1;} int two(){return 2; 100;}'

assert 1 'int main(){ return huga(1);} int huga(int a) { a; }'
assert 3 'int main(){ return huga(1, 2);} int huga(int a, int b) { a+b; }'
assert 21 'int main(){ return huga(1, 2, 3, 4, 5, 6);} int huga(int a, int b, int c, int d, int e, int f) { a+b+c+d+e+f; }'

# フィボナッチ数列のテスト (再帰を使う)
assert 0 'int main(){return fib(0);} int fib(int n) { if (n <= 1) return n; return fib(n-1) + fib(n-2); }'
assert 1 'int main(){return fib(1);} int fib(int n) { if (n <= 1) return n; return fib(n-1) + fib(n-2); }'
assert 1 'int main(){return fib(2);} int fib(int n) { if (n <= 1) return n; return fib(n-1) + fib(n-2); }'
assert 2 'int main(){return fib(3);} int fib(int n) { if (n <= 1) return n; return fib(n-1) + fib(n-2); }'
assert 3 'int main(){return fib(4);} int fib(int n) { if (n <= 1) return n; return fib(n-1) + fib(n-2); }'
assert 5 'int main(){return fib(5);} int fib(int n) { if (n <= 1) return n; return fib(n-1) + fib(n-2); }'
assert 8 'int main(){return fib(6);} int fib(int n) { if (n <= 1) return n; return fib(n-1) + fib(n-2); }'

# step16
assert 3 'int main() {int x;x=3; int y;y=&x; return *y;}'
assert 9 'int main() {int x;x=3; int y;y=&x; return 6 + *y;}'

# step18
assert 3 'int main() {int x; int *y; x=1; y=&x; *y=3; return x;}';
assert 3 'int main() {int x;x=3; return *&x; }'
assert 3 'int main() {int x;x=3; int *y;y=&x; int **z;z=&y; return **z; }'

assert 10 'int main() {int x; x=1; ptr(&x); return x;} int *ptr(int *a) { *a=10; }';


echo OK



