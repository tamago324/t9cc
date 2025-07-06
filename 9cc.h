// トークンの種類
typedef enum {
  TK_RESERVED, // 記号
  TK_NUM,      // 整数トークン
  TK_EOF,      // 入力の終わりを表すトークン
} TokenKind;

typedef struct Token Token;

struct Token {
  TokenKind kind; // トークンの型
  Token *next;    // 次の入力トークン
  int val;        // kind が TK_NUM の場合、その数値
  char *str;      // トークン文字列
  int len;        // トークンの長さ (== や >= などで使う)
};

// 抽象構文木のノードの種類
typedef enum {
  ND_ADD, // +
  ND_SUB, // -
  ND_MUL, // *
  ND_DIV, // /
  ND_EQ,  // ==
  ND_NE,  // !=
  ND_LT,  // < (Less Than)
  ND_LE,  // <= (Less than or Equal)
  ND_NUM, // 整数
} NodeKind;

typedef struct Node Node;

struct Node {
  NodeKind kind; // ノードの型
  Node *lhs;     // 左側の木
  Node *rhs;     // 右側の木
  int val;       // kind が ND_NUM の場合、数値が入る
};

// 現在着目しているトークン (宣言)
extern Token *token;

// 入力プログラム (宣言)
extern char *user_input;

// トークナイズ
Token *tokenize();

// パース
Node *expr();

// コード生成
void gen(Node *node);