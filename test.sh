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

assert 0 0
assert 42 42

echo OK