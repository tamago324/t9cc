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
    // 関数のエピローグにジャンプする
    printf("  jmp Lepilogue%s\n", node->funcname);
    return;
  }

  switch (node->kind) {

  case ND_EXPR_STMT:
    gen(node->lhs);
    printf("  add rsp, 8\n"); // 式文は、結果 (スタックの先頭) を捨てる
    return;

  case ND_BLOCK:
    for (Node *n = node->body; n; n = n->next) {
      gen(n);
    }
    return;

  case ND_IF:
    labelSeq++;
    // 条件式を評価
    gen(node->cond);
    printf("  pop rax\n"); // スタックの先頭に、条件式の結果が入っている
    if (!node->els) {
      // jump equals: false(0)の場合にジャンプする
      printf("  cmp rax, 0\n");
      printf("  je .Lend%d\n", labelSeq);
      gen(node->then);
    } else {
      // jump equals: false(0)の場合にジャンプする
      printf("  cmp rax, 0\n");
      printf("  je .Lelse%d\n", labelSeq);
      gen(node->then);
      printf("  jmp .Lend%d\n", labelSeq);

      printf(".Lelse%d:\n", labelSeq);
      gen(node->els);
    }

    printf(".Lend%d:\n", labelSeq);
    return;

  case ND_WHILE:
    labelSeq++;
    printf(".Lbegin%d:\n", labelSeq);

    gen(node->cond);
    printf("  pop rax\n");
    printf("  cmp rax, 0\n");
    printf("  je .Lend%d\n", labelSeq);

    gen(node->then);
    printf("  jmp .Lbegin%d\n", labelSeq);

    printf(".Lend%d:\n", labelSeq);
    return;

  case ND_FOR:
    labelSeq++;
    if (node->init)
      gen(node->init);
    printf(".Lbegin%d:\n", labelSeq);

    if (node->cond)
      gen(node->cond);
    printf("  pop rax\n");
    printf("  cmp rax, 0\n");
    printf("  je .Lend%d\n", labelSeq);

    gen(node->then);
    if (node->inc)
      gen(node->inc);
    printf("  jmp .Lbegin%d\n", labelSeq);
    printf(".Lend%d:\n", labelSeq);
    return;

  case ND_NUM:
    // 終端文字だから、そのまま出力して終わり
    printf("  push %d\n", node->val);
    return;

  case ND_CALL: {
    // 引数あり
    int argsLen = 0;
    for (Node *n = node->args; n; n = n->next) {
      gen(n);
      argsLen++;
    }
    // 後ろから順にセットしていく
    if (argsLen >= 6)
      printf("  pop r9\n");
    if (argsLen >= 5)
      printf("  pop r8\n");
    if (argsLen >= 4)
      printf("  pop rcx\n");
    if (argsLen >= 3)
      printf("  pop rdx\n");
    if (argsLen >= 2)
      printf("  pop rsi\n");
    if (argsLen >= 1)
      printf("  pop rdi\n");

    // RSPを16バイト境界に調整 (リファレンス実装のものを使用)
    labelSeq++;
    printf("  mov rax, rsp\n");
    printf("  and rax, 15\n");            // 16で割った余りを取得
    printf("  jnz .Lcall%d\n", labelSeq); // 16のあまりが0ではないならジャンプ
    printf("  mov rax, 0\n"); // 非可変長引数関数のため、RAX に0をセット
    printf("  call %s\n", node->funcname);
    printf("  jmp .Lend%d\n", labelSeq);
    printf(".Lcall%d:\n", labelSeq);
    printf("  sub rsp, 8\n"); // call でリターンアドレスが8バイトのため、調整
    printf("  mov rax, 0\n"); // 非可変長引数関数のため、RAX に0をセット
    printf("  call %s\n", node->funcname);
    printf("  add rsp, 8\n");
    printf(".Lend%d:\n", labelSeq);

    // call の結果を積んでおく (呼び出された側は RAX にいれるだけのため)
    printf("  push rax\n");
    return;
  }

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
  case ND_ARG:
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
  case ND_FOR:
  case ND_BLOCK:
  case ND_CALL:
  case ND_ARG:
  case ND_EXPR_STMT:
    break;
  }

  // 計算結果を push する必要がある (スタックに積みなおす)
  printf("  push rax\n");
}

void codegen(Function *prog) {

  // アセンブリの前半部分を出力
  printf(".intel_syntax noprefix\n");

  for (Function *fn = prog; fn; fn = fn->next) {

    printf(".global main\n");
    printf("%s:\n", fn->funcname);

    // プロローグ
    printf("  push rbp\n");
    printf("  mov rbp, rsp\n");
    printf("  sub rsp, %d\n", fn->stack_size);

    // 引数レジスタの値をローカル変数の値としてセットする
    int argsLen = 0;
    for (Node *n = fn->args; n; n = n->next) {
      printf("  mov rax, rbp\n");
      printf("  sub rax, %d\n", n->offset);

      argsLen += 1;

      if (argsLen == 6)
        printf("  mov [rax], r9\n");
      if (argsLen == 5)
        printf("  mov [rax], r8\n");
      if (argsLen == 4)
        printf("  mov [rax], rcx\n");
      if (argsLen == 3)
        printf("  mov [rax], rdx\n");
      if (argsLen == 2)
        printf("  mov [rax], rsi\n");
      if (argsLen == 1)
        printf("  mov [rax], rdi\n");
    }

    gen(fn->body);

    // エピローグ
    printf("Lepilogue%s:\n", fn->funcname);
    printf("  mov rsp, rbp\n");
    printf("  pop rbp\n");
    // ret でリターンアドレスにジャンプできる
    printf("  ret\n");
  }
}
