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

  // 各ローカル変数のオフセットと、fn->stack_size を計算する
  for (Function *fn = prog; fn; fn = fn->next) {
    int offset = 0;
    for (VarList *vl = fn->locals; vl; vl = vl->next) {
      offset += 8;
      vl->var->offset = offset;
    }
    fn->stack_size = offset;
  }

  codegen(prog);

  // 最後の式の結果が RAX に残っているため、それが返り値になる
  return 0;
}