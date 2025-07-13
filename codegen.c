#include "9cc.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

int labelSeq = 0;

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

  printf("  mov rax, rbp\n");
  printf("  sub rax, %d\n", node->offset);
  printf("  push rax\n");
}

void gen(Node *node) {
  if (node->kind == ND_RETURN) {
    gen(node->lhs);
    printf("  pop rax\n");
    // エピローグ
    printf("  mov rsp, rbp\n");
    printf("  pop rbp\n");
    printf("  ret\n");
    return;
  }

  switch (node->kind) {

  case ND_IF:
    labelSeq++;
    // 条件式を評価
    gen(node->cond);
    printf("  pop rax\n"); // スタックの先頭に、条件式の結果が入っている
    // jump equals: false(0)の場合にジャンプする
    printf("  cmp rax, 0\n");
    printf("  je Lelse%d\n", labelSeq);
    // true の場合
    gen(node->then);
    printf("  jmp Lend%d\n", labelSeq);

    // else の場合
    printf("Lelse%d:\n", labelSeq);
    if (node->els) {
      gen(node->els);
    }

    printf("Lend%d:\n", labelSeq);

  case ND_WHILE:
    labelSeq++;
    printf("Lbegin%d:\n", labelSeq);

    gen(node->cond);
    printf("  pop rax\n");
    printf("  cmp rax, 0\n");
    printf("  je Lend%d\n", labelSeq);

    gen(node->then);
    printf("  jmp Lbegin%d\n", labelSeq);

    printf("Lend%d:\n", labelSeq);

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
    // 右辺の値を push しておくのはなんでだろうか？
    //  -> C言語の代入演算子は、右辺の値を返す式であるため。
    printf("  push rdi\n");
    return;
  case ND_ADD:
  case ND_SUB:
  case ND_MUL:
  case ND_DIV:
  case ND_EQ:
  case ND_NE:
  case ND_LT:
  case ND_LE:
  case ND_RETURN:
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
    printf("  movzx rax, al\n");
    break;
  case ND_NE:
    printf("  cmp rax, rdi\n");
    printf("  setne al\n");
    printf("  movzx rax, al\n");
    break;
  case ND_LT:
    printf("  cmp rax, rdi\n");
    printf("  setl al\n");
    printf("  movzx rax, al\n");
    break;
  case ND_LE:
    printf("  cmp rax, rdi\n");
    printf("  setle al\n");
    printf("  movzx rax, al\n");
    break;
  case ND_ASSIGN:
  case ND_LVAR:
  case ND_NUM:
  case ND_RETURN:
  case ND_IF:
  case ND_WHILE:
    break;
  }

  // 計算結果を push する必要がある (スタックに積みなおす)
  printf("  push rax\n");
}
