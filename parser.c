#include "9cc.h"
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
  program    = stmt*
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
             | ident ("(" ")")?
             | "(" expr ")"
 */

// 複数文に対応するためのノードを格納する (定義)
Node *code[100];

// ローカル変数
LVar *locals;

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

bool at_eof() { return token->kind == TK_EOF; }

// ノード作成
Node *new_node(NodeKind kind, Node *lhs, Node *rhs) {
  Node *node = calloc(1, sizeof(Node));
  node->kind = kind;
  node->lhs = lhs;
  node->rhs = rhs;
  return node;
}

// 数値のノード作成
Node *new_node_num(int val) {
  Node *node = calloc(1, sizeof(Node));
  node->kind = ND_NUM;
  node->val = val;
  return node;
}

// 変数を名前で検索する。見つからなかったら NULL を返す
LVar *find_lvar(Token *tok) {
  for (LVar *var = locals; var; var = var->next) {
    // memcmp は、結果が同じ場合、0 となるため、 !memcmp とする必要がある
    if (var->len == tok->len && !memcmp(var->name, tok->str, var->len)) {
      return var;
    }
  }
  return NULL;
}

// ローカル変数の数
int lvar_len() {
  int i = 0;
  for (LVar *var = locals; var; var = var->next) {
    i++;
  }
  return i;
}

// 入れ子になるため、先に定義しておく
Node *stmt();
Node *expr();
Node *assign();
Node *equality();
Node *relational();
Node *add();
Node *mul();
Node *unary();
Node *primary();

// program = stmt*
void program() {
  int i = 0;

  while (!at_eof()) {
    code[i] = stmt();
    i++;
  }

  // 最後は NULL を入れておく
  code[i] = NULL;
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

    node = calloc(1, sizeof(Node));
    node->kind = ND_BLOCK;
    node->body = head.next;

    return node;
  }

  if (consume_by_kind(TK_IF)) {
    node = calloc(1, sizeof(Node));
    node->kind = ND_IF;
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
    node = calloc(1, sizeof(Node));
    node->kind = ND_WHILE;
    expect("(");
    node->cond = expr();
    expect(")");
    node->then = stmt();
    return node;
  }

  if (consume_by_kind(TK_FOR)) {
    node = calloc(1, sizeof(Node));
    node->kind = ND_FOR;
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
    node = calloc(1, sizeof(Node));
    node->kind = ND_RETURN;
    node->lhs = expr();
  } else {
    node = expr();
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
    node = new_node(ND_ASSIGN, node, assign());
  }

  return node;
}

// equality = relational ("==" relational | "!=" relational)*
Node *equality() {
  Node *node = relational();

  for (;;) {
    if (consume("==")) {
      node = new_node(ND_EQ, node, relational());
    } else if (consume("!=")) {
      node = new_node(ND_NE, node, relational());
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
      node = new_node(ND_LT, node, add());
    } else if (consume("<=")) {
      node = new_node(ND_LE, node, add());
    } else if (consume(">")) {
      // この時点で反転しておく
      node = new_node(ND_LT, add(), node);
    } else if (consume(">=")) {
      // この時点で反転しておく
      node = new_node(ND_LE, add(), node);
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
      node = new_node(ND_ADD, node, mul());
    } else if (consume("-")) {
      node = new_node(ND_SUB, node, mul());
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
      node = new_node(ND_MUL, node, unary());
    } else if (consume("/")) {
      node = new_node(ND_DIV, node, unary());
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
    return new_node(ND_SUB, new_node_num(0), primary());
  }

  // 通常の数値の場合は、通常通りに処理する
  return primary();
}

// 変数
Node *lvar(Token *tok) {
  // 識別子なら、LVAR (ローカル変数) として処理する
  Node *node = calloc(1, sizeof(Node));
  node->kind = ND_LVAR;

  LVar *lvar = find_lvar(tok);
  if (lvar) {
    // すでに宣言されているものは、その変数のオフセットを使う
    node->offset = lvar->offset;
  } else {
    // 新しい変数の場合は、新しい領域のオフセットを計算する
    lvar = calloc(1, sizeof(LVar));
    lvar->next = locals;
    lvar->name = tok->str;
    lvar->len = tok->len;
    // ベースポインタからのオフセット
    //   1つ前の変数からのオフセットで新しいオフセットが計算できる
    if (locals) {
      lvar->offset = locals->offset + 8;
    } else {
      lvar->offset = 0;
    }
    node->offset = lvar->offset; // これがコード生成のときに使用するもの

    // 先頭の更新
    locals = lvar;
  }

  return node;
}

// primary = num
//         | ident ("(" ")")?
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
      Node *node = calloc(1, sizeof(Node));
      node->kind = ND_CALL;
      node->name = tok->str;
      node->len = tok->len;

      // 今は、引数無しでの呼び出しのみ対応
      expect(")");
      return node;
    } else {
      return lvar(tok);
    }
  }

  return new_node_num(expect_number());
}