#include "9cc.h"
#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// 現在着目しているトークン (定義)
Token *token;

// 複数文に対応するためのノードを格納する (定義)
Node *code[100];

// ローカル変数
LVar *locals;

// エラー箇所を報告する
void error_at(char *loc, char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);

  int pos = loc - user_input;
  fprintf(stderr, "%s\n", user_input);
  fprintf(stderr, "%*s", pos, " "); // pos 個分、空白を出力
  fprintf(stderr, "^ ");
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  exit(1);
}

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
Token *consume_ident() {
  if (token->kind != TK_IDENT) {
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

// トークンを生成して、cur につなげる
Token *new_token(TokenKind kind, Token *cur, char *str, int len) {
  Token *tok = calloc(1, sizeof(Token));
  tok->kind = kind;
  tok->str = str;
  tok->len = len;
  cur->next = tok;
  return tok;
}

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

// 指定の文字で始まるか？
//   memcmp の結果は同じなら 0 を返す
bool startswith(char *a, char *b) { return memcmp(a, b, strlen(b)) == 0; }

// 識別子かどうか
int is_alnum(char c) {
  return ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z') ||
         ('0' <= c && c <= '9') || (c == '_');
}

// 入力文字列p をトークナイズして、それを返す
void tokenize() {
  char *p = user_input;

  Token head;
  head.next = NULL;
  Token *cur = &head;

  while (*p) {
    // 空白文字はスキップ
    if (isspace(*p)) {
      p++;
      continue;
    }

    // 2桁の記号の処理
    if (startswith(p, "==") || startswith(p, "!=") || startswith(p, ">=") ||
        startswith(p, "<=")) {
      cur = new_token(TK_RESERVED, cur, p, 2);
      p += 2;
      continue;
    }

    // 1桁の記号の処理
    if (strchr("+-*/()><;=", *p)) {
      cur = new_token(TK_RESERVED, cur, p++, 1);
      continue;
    }

    // 数字の処理
    if (isdigit(*p)) {
      cur = new_token(TK_NUM, cur, p, 0);
      char *q = p;
      cur->val = strtol(p, &p, 10);
      // 長さを計算してセットしなおす
      cur->len = p - q;
      continue;
    }

    // 変数
    if (is_alnum(*p)) {
      cur = new_token(TK_IDENT, cur, p, 0);
      // a~z の部分までループして、消費しつつ、長さを取得する
      char *q = p;
      while (*p) {
        if (!(is_alnum(*p))) {
          break;
        }
        p++;
      }
      cur->len = p - q;
      continue;
    }

    error_at(p, "トークナイズできません");
  }

  // 最後は、EOFにしておく
  new_token(TK_EOF, cur, p, 0);
  // 次に進めておく。 (最初はダミーのため。)
  token = head.next;
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

// stmt = expr ";"
Node *stmt() {
  Node *node = expr();
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

// primary = num | ident | "(" expr ")"
Node *primary() {
  if (consume("(")) {
    Node *node = expr();
    expect(")");
    return node;
  }

  Token *tok = consume_ident();
  if (tok) {
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

  return new_node_num(expect_number());
}