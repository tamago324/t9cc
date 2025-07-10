#include "9cc.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

// エラーを報告するための関数
// printfと同じ引数をとる
void error(char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  exit(1);
}

// 左辺の変数のアドレスを算出して、スタックに積む
void gen_lval(Node *node) {
  if (node->kind != ND_LVAR) {
    error("代入の左辺値が変数ではありません。");
  }

  printf("  mov, rax, rbp\n");
  printf("  sub rax, %d\n", node->offset);
  printf("  push rax\n");
}

void gen(Node *node) {
  switch (node->kind) {
  case ND_NUM:
    // 終端文字だから、そのまま出力して終わり
    printf("  push %d\n", node->val);
    return;
  case ND_LVAR:
    // 変数のアドレスを計算してスタックに積む
    gen_lval(node);
    // スタックの先頭にある変数のアドレスから値を読み取る
    printf("  pop rax\n");
    printf("  mov rax, [rax]\n");
    printf("  push rax\n");
    return;
  case ND_ASSIGN:
    // 左辺の変数のアドレスを計算してスタックに積む
    gen_lval(node->lhs);
    // 右辺の式を評価して、スタックに積む
    gen(node->rhs);
    // 左辺の値を右辺の変数に代入する
    printf("  pop rdi\n");
    printf("  pop rax\n");
    printf("  mov [rax], rdi\n");
    printf("  push rdi"); // 右辺の値を push しておくのはなんでだろうか？
    return;
  case ND_ADD:
  case ND_SUB:
  case ND_MUL:
  case ND_DIV:
  case ND_EQ:
  case ND_NE:
  case ND_LT:
  case ND_LE:
    break;
  }

  // 帰りがけで巡回する
  // left -> right を出力する
  gen(node->lhs);
  gen(node->rhs);

  // pop して、rdi と rax に格納する
  // (除算の場合、順序が重要になるため、rdi, rax の順にしておく)
  printf("  pop rdi\n");
  printf("  pop rax\n");

  // 帰りがけの根の部分を処理
  switch (node->kind) {
  case ND_ADD:
    printf("  add rax, rdi\n");
    break;
  case ND_SUB:
    printf("  sub rax, rdi\n");
    break;
  case ND_MUL:
    // 符号付の乗算
    printf("  imul rax, rdi\n");
    break;
  case ND_DIV:
    printf("  cqo\n");
    printf("  idiv rdi\n");
    break;
  case ND_EQ:
    printf("  cmp rax, rdi\n");
    printf("  sete al\n");
    printf("  movzb rax, al\n");
    break;
  case ND_NE:
    printf("  cmp rax, rdi\n");
    printf("  setne al\n");
    printf("  movzb rax, al\n");
    break;
  case ND_LT:
    printf("  cmp rax, rdi\n");
    printf("  setl al\n");
    printf("  movzb rax, al\n");
    break;
  case ND_LE:
    printf("  cmp rax, rdi\n");
    printf("  setle al\n");
    printf("  movzb rax, al\n");
    break;
  case ND_ASSIGN:
  case ND_LVAR:
  case ND_NUM:
    break;
  }

  // 計算結果を push する必要がある (スタックに積みなおす)
  printf("  push rax\n");
}
