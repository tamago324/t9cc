#!/bin/bash

assert() {
  # == 準備 ==
  expected="$1"
  input="$2"

  # 入力を渡して、アセンブリに変換
  ./9cc "$input" > tmp.s
  # 実行ファイル作成
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

assert 0 "0;"
assert 42 "42;"
assert 21 "5+20-4;"
assert 51 "5 - 3 + 99 - 50;"

# step5
assert 98 "200 - (3 + 99);"
assert 7 "1+2*3;"
assert 26 "2*3+4*5;"
assert 2 "1+2+10-(2+3*3);"

# step6
assert 10 "-10+20;"
assert 7 "-(5+8)+20;"

# step7
assert 0 '0==1;'
assert 1 '42==42;'
assert 1 '0!=1;'
assert 0 '42!=42;'

assert 1 '0<1;'
assert 0 '1<1;'
assert 0 '2<1;'
assert 1 '0<=1;'
assert 1 '1<=1;'
assert 0 '2<=1;'

assert 1 '1>0;'
assert 0 '1>1;'
assert 0 '1>2;'
assert 1 '1>=0;'
assert 1 '1>=1;'
assert 0 '1>=2;'

# step9
assert 3 '1;2;3;'
assert 3 'a=1;b=2;a+b;'

# step10
assert 3 'abc=1;def=2;abc+def;'
assert 6 'a1 = 1;b2 = 2;c_3 = 3;a1 + b2 + c_3;'

# step11;
assert 3 'return 3;'
assert 4 'a=1; return a+3;'
assert 14 'a = 3; b = 5 * 6 - 8; return a + b / 2;'

# step12
assert 2 'if (1==1) return 2;'

assert 3 'if (0) return 2; return 3;'
assert 3 'if (1-1) return 2; return 3;'
assert 2 'if (1) return 2; return 3;'
assert 2 'if (2-1) return 2; return 3;'

  # else
assert 1 'if (0) return 3; else return 1;'
assert 3 'if (1) return 3; else return 1;'

  # while
assert 10 'i=0; while(i<10) i=i+1; return i;'

echo OK

