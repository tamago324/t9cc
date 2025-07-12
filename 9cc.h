// トークンの種類
typedef enum {
  TK_RESERVED, // 記号
  TK_IDENT,    // 識別子
  TK_NUM,      // 整数トークン
  TK_EOF,      // 入力の終わりを表すトークン
} TokenKind;

typedef struct Token Token;

struct Token {
  TokenKind kind; // トークンの型
  Token *next;    // 次の入力トークン
  int val;        // kind が TK_NUM の場合、その数値
  char *str;      // トークン文字列
  int len;        // トークンの長さ (== や >= や変数などで使う)
};

// 抽象構文木のノードの種類
typedef enum {
  ND_ADD,    // +
  ND_SUB,    // -
  ND_MUL,    // *
  ND_DIV,    // /
  ND_ASSIGN, // =
  ND_LVAR,   // ローカル変数
  ND_EQ,     // ==
  ND_NE,     // !=
  ND_LT,     // < (Less Than)
  ND_LE,     // <= (Less than or Equal)
  ND_NUM,    // 整数
} NodeKind;

typedef struct Node Node;

struct Node {
  NodeKind kind; // ノードの型
  Node *lhs;     // 左側の木
  Node *rhs;     // 右側の木
  int val;       // kind が ND_NUM の場合、数値が入る
  int offset;    // kind が ND_LVAR の場合、ベースポインタからのオフセットが入る
};

// ローカル変数の型 (連結リストで全変数を表現)
typedef struct LVar LVar;
struct LVar {
  LVar *next; // 次の変数 or NULL
  char *name; // 変数名
  int len;    // 名前の長さ
  int offset; // RBP からのオフセット (これがスタック領域の割り当てと同等)
};

// 入力プログラム (宣言)
extern char *user_input;

// コード全体 (宣言)
extern Node *code[100];

// トークナイズ
void tokenize();

// パース
void program();

// コード生成
void gen(Node *node);

// ローカル変数の数
int lvar_len();