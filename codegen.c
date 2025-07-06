#include "9cc.h"
#include <stdio.h>

void gen(Node *node) {
  if (node->kind == ND_NUM) {
    // 終端文字だから、そのまま出力して終わり
    printf("  push %d\n", node->val);
    return;
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
  case ND_NUM:
    // ここには来ない想定
    break;
  }

  // 計算結果を push する必要がある (スタックに積みなおす)
  printf("  push rax\n");
}
