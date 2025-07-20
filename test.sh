#!/bin/bash

assert() {
  # == 準備 ==
  expected="$1"
  input="$2"

  # 入力を渡して、アセンブリに変換
  ./9cc "$input" > tmp.s
  # 実行ファイル作成 (+テスト関数が書かれたオブジェクトファイルをリンク)
  cc -o tmp tmp.s test_func.o
  
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

assert 0 'main() {0;}'
assert 42 'main() {42;}'
assert 21 'main() {5+20-4;}'
assert 51 'main() {5 - 3 + 99 - 50;}'

# step5
assert 98 'main() {200 - (3 + 99);}'
assert 7 'main() {1+2*3;}'
assert 26 'main() {2*3+4*5;}'
assert 2 'main() {1+2+10-(2+3*3);}'

# step6
assert 10 'main() {-10+20;}'
assert 7 'main() {-(5+8)+20;}'

# step7
assert 0 'main() {0==1;}'
assert 1 'main() {42==42;}'
assert 1 'main() {0!=1;}'
assert 0 'main() {42!=42;}'

assert 1 'main() {0<1;}'
assert 0 'main() {1<1;}'
assert 0 'main() {2<1;}'
assert 1 'main() {0<=1;}'
assert 1 'main() {1<=1;}'
assert 0 'main() {2<=1;}'

assert 1 'main() {1>0;}'
assert 0 'main() {1>1;}'
assert 0 'main() {1>2;}'
assert 1 'main() {1>=0;}'
assert 1 'main() {1>=1;}'
assert 0 'main() {1>=2;}'

# step9
assert 3 'main() {1;2;3;}'
assert 3 'main() {a=1;b=2;a+b;}'

# step10
assert 3 'main() {abc=1;def=2;abc+def;}'
assert 6 'main() {a1 = 1;b2 = 2;c_3 = 3;a1 + b2 + c_3;}'

# step11;
assert 3 'main() {return 3;}'
assert 4 'main() {a=1; return a+3;}'
assert 14 'main() {a = 3; b = 5 * 6 - 8; return a + b / 2;}'

# step12
assert 2 'main() {if (1==1) return 2;}'

assert 3 'main() {if (0) return 2; return 3;}'
assert 3 'main() {if (1-1) return 2; return 3;}'
assert 2 'main() {if (1) return 2; return 3;}'
assert 2 'main() {if (2-1) return 2; return 3;}'

  # else
assert 1 'main() {if (0) return 3; else return 1;}'
assert 3 'main() {if (1) return 3; else return 1;}'

  # while
assert 10 'main() {i=0; while(i<10) i=i+1; return i;}'

  # for
assert 55 'main() {i=0; j=0; for (i=0; i<=10; i=i+1) j=i+j; return j;}'
assert 3 'main() {for (;;) return 3; return 5;}'

# step13
assert 10 'main() {i=0; j=0; for (i=0; i<10; i=i+1) {j=j+1;} return j;}'
assert 10 'main() {{return 10;}}'

# step14
assert 10 'main(){foo(); return 10;}'

assert 3 'main(){bar(1, 2); return 3;}'
assert 21 'main(){huga(1, 2, 3, 4, 5, 6); return 21;}'

# step15
assert 1 'main() {return 1;}'
assert 7 'main() {return one()+two()*3;} one(){1;} two(){return 2; 100;}'

echo OK



