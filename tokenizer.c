#include "9cc.h"
#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// 現在着目しているトークン (定義)
Token *token;

// トークンを生成して、cur につなげる
Token *new_token(TokenKind kind, Token *cur, char *str, int len) {
  Token *tok = calloc(1, sizeof(Token));
  tok->kind = kind;
  tok->str = str;
  tok->len = len;
  cur->next = tok;
  return tok;
}

// 指定の文字で始まるか？
//   memcmp の結果は同じなら 0 を返す
bool startswith(char *a, char *b) { return memcmp(a, b, strlen(b)) == 0; }

// 識別子かどうか
int is_alnum(char c) {
  return ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z') ||
         ('0' <= c && c <= '9') || (c == '_');
}

// キーワードかどうか
int is_keyward(char *p, char *keyword) {
  return startswith(p, keyword) && !is_alnum(p[strlen(keyword)]);
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
    if (strchr("+-*/()><;={},&", *p)) {
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

    // return キーワード
    if (is_keyward(p, "return")) {
      cur = new_token(TK_RETURN, cur, p, 6);
      p += 6;
      continue;
    }

    if (is_keyward(p, "if")) {
      cur = new_token(TK_IF, cur, p, 2);
      p += 2;
      continue;
    }

    if (is_keyward(p, "else")) {
      cur = new_token(TK_ELSE, cur, p, 4);
      p += 4;
      continue;
    }

    if (is_keyward(p, "while")) {
      cur = new_token(TK_WHILE, cur, p, 5);
      p += 5;
      continue;
    }

    if (is_keyward(p, "for")) {
      cur = new_token(TK_FOR, cur, p, 3);
      p += 3;
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