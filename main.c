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
  Function *prog = program();

  // TODO: スタックのサイズを計算する

  codegen(prog);

  // 最後の式の結果が RAX に残っているため、それが返り値になる
  return 0;
}