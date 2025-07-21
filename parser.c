#define _GNU_SOURCE
#include "9cc.h"
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
  program    = function*
  function   = ident func-def-args "{" stmt* "}"
  func-def-args = "(" (ident ("," ident)*)? ")"
  stmt       = expr ";"
             | "{" stmt* "}"
             | "if" "(" expr ")" stmt ("else" stmt)?
             | "while" "(" expr ")" stmt
             | "for" "(" expr? ";" expr? ";" expr? ")" stmt
             | "return" expr ";"
  expr       = assign
  assign	   = equality ("=" assign)?
  equality   = relational ("==" relational | "!=" relational)*
  relational = add ("<" add | "<=" add | ">" add | ">=" add)*
  add        = mul ("+" mul | "-" mul)*
  mul        = unary ("*" unary | "/" unary)*
  unary      = ("+" | "-")? primary
  primary    = num
             | ident func-args?
             | "(" expr ")"
  func-args  = "(" (expr ("," expr)*)? ")"
 */

// 現在の関数 (return するときのラベル名に使用する)
Function *cur_func;

// 現在のトークンが期待している記号のときは、トークンを消費して
// true を返す。
// それ以外の場合は false を返す。
bool consume(char *op) {
  if (token->kind != TK_RESERVED || strlen(op) != token->len ||
      memcmp(token->str, op, token->len)) {
    return false;
  }

  token = token->next;
  return true;
}

// 識別子かどうかチェックし、もしそうなら読み進める
Token *consume_by_kind(TokenKind kind) {
  if (token->kind != kind) {
    return false;
  }

  Token *tok = token;
  token = token->next;
  return tok;
}

// 現在のトークンが期待している記号のときは、トークンを消費する。
// それ以外の場合はエラーを報告する。
void expect(char *op) {
  if (token->kind != TK_RESERVED || strlen(op) != token->len ||
      memcmp(token->str, op, token->len)) {
    error_at(token->str, "'%c'ではありません", op);
  }

  token = token->next;
}

// 現在のトークンが数値の場合、そのトークンを消費して、次に進む
// それ以外の場合にはエラーを報告する。
int expect_number() {
  if (token->kind != TK_NUM) {
    error_at(token->str, "数ではありません。");
  }

  int val = token->val;
  token = token->next;
  return val;
}

// 現在のトークンが識別子の場合、そのトークンを消費して、次に進む
// それ以外の場合にはエラーを報告する
Token *expect_ident() {
  if (token->kind != TK_IDENT)
    error_at(token->str, "ident ではありません");

  Token *tok = token;
  token = token->next;
  return tok;
}

bool at_eof() { return token->kind == TK_EOF; }

Node *new_node(NodeKind kind) {
  Node *node = calloc(1, sizeof(Node));
  node->kind = kind;
  return node;
}

// ノード作成
Node *new_binary(NodeKind kind, Node *lhs, Node *rhs) {
  Node *node = new_node(kind);
  node->lhs = lhs;
  node->rhs = rhs;
  return node;
}

// 数値のノード作成
Node *new_node_num(int val) {
  Node *node = new_node(ND_NUM);
  node->val = val;
  return node;
}

// 変数を名前で検索する。見つからなかったら NULL を返す
LVar *find_lvar(Token *tok) {
  for (LVar *var = cur_func->locals; var; var = var->next) {
    // memcmp は、結果が同じ場合、0 となるため、 !memcmp とする必要がある
    if (var->len == tok->len && !memcmp(var->name, tok->str, var->len)) {
      return var;
    }
  }
  return NULL;
}

// 変数の領域のオフセットを取得する
// 既に宣言されている場合は、その変数のオフセット、まだの場合は領域を確保する
int lvar_offset(Token *tok) {
  LVar *lvar = find_lvar(tok);
  if (lvar) {
    // すでに宣言されているものは、その変数のオフセットを使う
    return lvar->offset;
  } else {
    // 新しい変数の場合は、新しい領域のオフセットを計算する
    lvar = calloc(1, sizeof(LVar));
    lvar->next = cur_func->locals;
    lvar->name = tok->str;
    lvar->len = tok->len;
    // ベースポインタからのオフセット
    //   1つ前の変数からのオフセットで新しいオフセットが計算できる
    if (cur_func->locals) {
      lvar->offset = cur_func->locals->offset + 8;
    } else {
      // オフセットを8にしておくことで、関数の引数の領域を確保できる
      lvar->offset = 8;
    }
    // 先頭の更新
    cur_func->locals = lvar;

    return lvar->offset; // これがコード生成のときに使用するもの
  }
}

// 入れ子になるため、先に定義しておく
Function *function();
Node *func_def_args();
Node *stmt();
Node *expr();
Node *assign();
Node *equality();
Node *relational();
Node *add();
Node *mul();
Node *unary();
Node *primary();

// program    = func-def func-def*
Function *program() {
  Function head;
  head.next = NULL;
  Function *cur = &head;

  while (!at_eof()) {
    cur->next = function();
    cur = cur->next;
  }

  return head.next;
}

// func-def = ident func-def-args "{" stmt* "}"
Function *function() {
  Token *tok = expect_ident();

  Function *func = calloc(1, sizeof(Function));
  cur_func = func;
  func->funcname = strndup(tok->str, tok->len);
  func->args = func_def_args();

  // block
  // TODO: stmt の部分と合わせて、関数にしておきたい
  expect("{");

  Node head;
  head.next = NULL;
  Node *cur = &head;

  while (!consume("}")) {
    // 後ろに追加していく
    cur->next = stmt();
    cur = cur->next;
  }

  // ND_BLOCK とすることで、if や for と同じコード生成の処理を通せる
  Node *body = new_node(ND_BLOCK);
  body->body = head.next;

  func->body = body;

  return func;
}

// 関数定義の引数
Node *arg() {
  Token *tok = expect_ident();
  Node *node = new_node(ND_ARG);
  // 引数をローカル変数として領域を確保する
  //   プロローグでそのローカル変数にレジスタから値をコピーすることでローカル変数と同じように扱える
  node->offset = lvar_offset(tok);
  return node;
}

// func-def-args = "(" (ident ("," ident)*)? ")"
Node *func_def_args() {
  // 今は引数無しだけにしておく
  expect("(");
  if (consume(")")) {
    // 引数無し
    return NULL;
  }

  // 引数あり
  Node *head = arg();
  Node *cur = head;

  while (consume(",")) {
    cur->next = arg();
    cur = cur->next;
  }
  expect(")");

  return head;
}

/**
 stmt = expr ";"
      | "{" stmt* "}"
      | "if" "(" expr ")" stmt ("else" stmt)?
      | "while" "(" expr ")" stmt
      | "for" "(" expr? ";" expr? ";" expr? ")" stmt
      | "return" expr ";"
 */
Node *stmt() {
  Node *node;

  if (consume("{")) {
    Node head;
    head.next = NULL;
    Node *cur = &head;

    while (!consume("}")) {
      // 後ろに追加していく
      cur->next = stmt();
      cur = cur->next;
    }

    node = new_node(ND_BLOCK);
    node->body = head.next;

    return node;
  }

  if (consume_by_kind(TK_IF)) {
    node = new_node(ND_IF);
    expect("(");
    node->cond = expr();
    expect(")");
    node->then = stmt();

    if (consume_by_kind(TK_ELSE)) {
      node->els = stmt();
    }
    return node;
  }

  if (consume_by_kind(TK_WHILE)) {
    node = new_node(ND_WHILE);
    expect("(");
    node->cond = expr();
    expect(")");
    node->then = stmt();
    return node;
  }

  if (consume_by_kind(TK_FOR)) {
    node = new_node(ND_FOR);
    expect("(");

    if (!consume(";")) {
      node->init = expr();
      expect(";");
    }

    if (!consume(";")) {
      node->cond = expr();
      expect(";");
    }

    if (!consume(")")) {
      node->inc = expr();
      expect(")");
    }

    node->then = stmt();
    return node;
  }

  if (consume_by_kind(TK_RETURN)) {
    node = new_node(ND_RETURN);
    node->lhs = expr();
    // エピローグのラベルにジャンプするときに使用する
    node->funcname = cur_func->funcname;
  } else {
    // 式文
    node = new_node(ND_EXPR_STMT);
    node->lhs = expr();
  }
  expect(";");
  return node;
}

// expr = assign
Node *expr() { return assign(); }

// assign = equality ("=" assign)?
Node *assign() {
  Node *node = equality();

  if (consume("=")) {
    node = new_binary(ND_ASSIGN, node, assign());
  }

  return node;
}

// equality = relational ("==" relational | "!=" relational)*
Node *equality() {
  Node *node = relational();

  for (;;) {
    if (consume("==")) {
      node = new_binary(ND_EQ, node, relational());
    } else if (consume("!=")) {
      node = new_binary(ND_NE, node, relational());
    } else {
      return node;
    }
  }
}

// relational = add ("<" add | "<=" add | ">" add | ">=" add)*
Node *relational() {
  Node *node = add();

  for (;;) {
    if (consume("<")) {
      node = new_binary(ND_LT, node, add());
    } else if (consume("<=")) {
      node = new_binary(ND_LE, node, add());
    } else if (consume(">")) {
      // この時点で反転しておく
      node = new_binary(ND_LT, add(), node);
    } else if (consume(">=")) {
      // この時点で反転しておく
      node = new_binary(ND_LE, add(), node);
    } else {
      return node;
    }
  }
}

// add = mul ("+" mul | "-" mul)*
Node *add() {
  Node *node = mul();

  for (;;) {
    if (consume("+")) {
      node = new_binary(ND_ADD, node, mul());
    } else if (consume("-")) {
      node = new_binary(ND_SUB, node, mul());
    } else {
      return node;
    }
  }
}

// mul = unary ("*" unary | "/" unary)*
Node *mul() {
  Node *node = unary();

  for (;;) {
    if (consume("*")) {
      node = new_binary(ND_MUL, node, unary());
    } else if (consume("/")) {
      node = new_binary(ND_DIV, node, unary());
    } else {
      return node;
    }
  }
}

// unary = ("+" | "-")? primary
Node *unary() {
  if (consume("+")) {
    return primary();
  }
  if (consume("-")) {
    return new_binary(ND_SUB, new_node_num(0), primary());
  }

  // 通常の数値の場合は、通常通りに処理する
  return primary();
}

// 変数
Node *lvar(Token *tok) {
  // 識別子なら、LVAR (ローカル変数) として処理する
  Node *node = new_node(ND_LVAR);
  node->offset = lvar_offset(tok);
  return node;
}

// func-args = "(" (expr ("," expr)*)? ")"
Node *func_args() {
  if (consume(")")) {
    // 引数無し
    return NULL;
  }

  // 引数あり
  Node *head = expr();
  Node *cur = head;

  while (consume(",")) {
    cur->next = expr();
    cur = cur->next;
  }
  expect(")");
  return head;
}

// primary = num
//         | ident func-args?
//         | "(" expr ")"
Node *primary() {
  if (consume("(")) {
    Node *node = expr();
    expect(")");
    return node;
  }

  Token *tok = consume_by_kind(TK_IDENT);
  if (tok) {
    if (consume("(")) {
      // 関数呼び出し
      Node *node = new_node(ND_CALL);
      node->funcname = strndup(tok->str, tok->len);
      node->args = func_args();

      return node;
    } else {
      return lvar(tok);
    }
  }

  return new_node_num(expect_number());
}