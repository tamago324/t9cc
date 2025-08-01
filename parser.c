#define _GNU_SOURCE
#include "9cc.h"
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
  program    = function*
  function   = "int" ("*")* ident "(" params? ")" "{" stmt* "}"
  params     = "int" ("*")* ident ("," "int" ("*")* ident)*
  stmt       = expr ";"
             | "{" stmt* "}"
             | "if" "(" expr ")" stmt ("else" stmt)?
             | "while" "(" expr ")" stmt
             | "for" "(" expr? ";" expr? ";" expr? ")" stmt
             | "return" expr ";"
             | "int" ("*")* ident ";"
  expr       = assign
  assign	   = equality ("=" assign)?
  equality   = relational ("==" relational | "!=" relational)*
  relational = add ("<" add | "<=" add | ">" add | ">=" add)*
  add        = mul ("+" mul | "-" mul)*
  mul        = unary ("*" unary | "/" unary)*
  unary      = ("+" | "-")? primary
             | "*" unary
             | "&" unary
  primary    = num
             | ident func-args?
             | "(" expr ")"
  func-args  = "(" (expr ("," expr)*)? ")"
 */

// 現在の関数のローカル変数
VarList *locals;

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
char *expect_ident() {
  if (token->kind != TK_IDENT)
    error_at(token->str, "ident ではありません");

  char *name = strndup(token->str, token->len);
  token = token->next;
  return name;
}

// 現在のトークンが肩の場合、そのトークンを消費して、次に進む
// それ以外の場合にはエラーを報告する
Type *expect_type() {
  if (token->kind != TK_INT)
    error_at(token->str, "int ではありません");

  token = token->next;

  // 今はINTのみ
  Type *ty = calloc(1, sizeof(Type));
  Type *ptr_to;
  // ポインタ型定義
  while (consume("*")) {
    ty->ty = PTR;
    ptr_to = calloc(1, sizeof(Type));
    ty->ptr_to = ptr_to;
    ty = ptr_to;
  }
  ty->ty = INT;

  return ty;
}

bool at_eof() { return token->kind == TK_EOF; }

Node *new_node(NodeKind kind) {
  Node *node = calloc(1, sizeof(Node));
  node->kind = kind;
  return node;
}

Node *new_unary(NodeKind kind, Node *lhs) {
  Node *node = new_node(kind);
  node->lhs = lhs;
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
Var *find_lvar(Token *tok) {
  for (VarList *vl = locals; vl; vl = vl->next) {
    Var *var = vl->var;
    // memcmp は、結果が同じ場合、0 となるため、 !memcmp とする必要がある
    if (strlen(var->name) == tok->len &&
        !memcmp(tok->str, var->name, tok->len)) {
      return var;
    }
  }
  return NULL;
}

// 入れ子になるため、先に定義しておく
Function *function();
VarList *read_func_params();
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

// function   = "int" ("*")* ident "(" params? ")" "{" stmt* "}"
// params     = "int" ("*")* ident ("," "int" ("*")* ident)*
Function *function() {
  locals = NULL;
  Function *func = calloc(1, sizeof(Function));
  func->return_type = expect_type();
  func->funcname = expect_ident();
  func->params = read_func_params();

  // block
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
  func->locals = locals;

  return func;
}

// ローカル変数として定義する (locals にも追加する)
Var *push_var(Type *type, char *name) {
  // 追加する変数
  Var *var = calloc(1, sizeof(Var));
  var->name = name;
  var->ty = type;

  // ローカル変数のリストに追加する
  VarList *vl = calloc(1, sizeof(VarList));
  vl->var = var;
  vl->next = locals;
  locals = vl;

  // 追加した変数を返す
  return var;
}

VarList *read_func_params() {
  // 今は引数無しだけにしておく
  expect("(");
  if (consume(")")) {
    // 引数無し
    return NULL;
  }

  // 引数あり
  VarList head;
  head.next = calloc(1, sizeof(VarList));
  Type *type = expect_type();
  head.next->var = push_var(type, expect_ident());

  VarList *cur = head.next;

  while (consume(",")) {
    cur->next = calloc(1, sizeof(VarList));
    Type *type = expect_type();
    cur->next->var = push_var(type, expect_ident());
    cur = cur->next;
  }
  expect(")");

  return head.next;
}

/**
 stmt = expr ";"
      | "{" stmt* "}"
      | "if" "(" expr ")" stmt ("else" stmt)?
      | "while" "(" expr ")" stmt
      | "for" "(" expr? ";" expr? ";" expr? ")" stmt
      | "return" expr ";"
      | "int" ident ";"
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

  if (consume_by_kind(TK_INT)) {
    node = new_node(ND_VAR_DEF);

    Type *ty = calloc(1, sizeof(Type));
    Type *ptr_to;
    // ポインタ型定義
    while (consume("*")) {
      ty->ty = PTR;
      ptr_to = calloc(1, sizeof(Type));
      ty->ptr_to = ptr_to;
      ty = ptr_to;
    }
    ty->ty = INT;

    Var *var = push_var(ty, expect_ident());
    node->var = var;
    expect(";");

    return node;
  }

  if (consume_by_kind(TK_RETURN)) {
    node = new_node(ND_RETURN);
    node->lhs = expr();
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
//       | "*" unary
//       | "&" unary
Node *unary() {
  if (consume("*")) {
    return new_unary(ND_DEREF, unary());
  }

  if (consume("&")) {
    return new_unary(ND_ADDR, unary());
  }

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
Node *new_var(Var *var) {
  // 識別子なら、var として処理する
  Node *node = new_node(ND_VAR);
  node->var = var;
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
      // 変数
      Var *var = find_lvar(tok);
      if (!var) {
        error_at(tok->str, "変数が定義されていません");
      }
      return new_var(var);
    }
  }

  return new_node_num(expect_number());
}