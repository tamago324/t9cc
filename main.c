#include "9cc.h"
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>

// 入力プログラム (定義)
char *user_input;

int main(int argc, char **argv) {
  if (argc != 2) {
    fprintf(stderr, "引数の個数が正しくありません\n");
    return 1;
  }

  // トークナイズして、ASTにパースする
  // トークないずの結果は code に保存されるため、ここでは保持しない
  user_input = argv[1];
  tokenize();
  program();

  // アセンブリの前半部分を出力
  printf(".intel_syntax noprefix\n");
  printf(".global main\n");

  // 先頭の式から順にコード生成
  for (int i = 0; code[i]; i++) {
    gen(code[i]);
  }

  // 最後の式の結果が RAX に残っているため、それが返り値になる
  return 0;
}