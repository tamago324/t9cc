// トークンの種類
typedef enum {
  TK_RESERVED, // 記号
  TK_IDENT,    // 識別子
  TK_NUM,      // 整数トークン
  TK_RETURN,   // return のトークン
  TK_IF,       // if のトークン
  TK_ELSE,     // else のトークン
  TK_WHILE,    // while のトークン
  TK_FOR,      // for のトークン
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
  ND_ADD,       // +
  ND_SUB,       // -
  ND_MUL,       // *
  ND_DIV,       // /
  ND_ASSIGN,    // =
  ND_VAR,       // ローカル変数
  ND_EQ,        // ==
  ND_NE,        // !=
  ND_LT,        // < (Less Than)
  ND_LE,        // <= (Less than or Equal)
  ND_NUM,       // 整数
  ND_RETURN,    // return
  ND_IF,        // if
  ND_WHILE,     // while
  ND_FOR,       // for
  ND_BLOCK,     // block
  ND_CALL,      // 関数呼び出し
  ND_EXPR_STMT, // 式文
} NodeKind;

// ローカル変数の型
typedef struct Var Var;
struct Var {
  char *name; // 変数名
  int offset; // RBP からのオフセット (これがスタック領域の割り当てと同等)
};

// 変数のリスト (連結リストで表現)
typedef struct VarList VarList;
struct VarList {
  VarList *next; // 次の変数 or NULL
  Var *var;
};

typedef struct Node Node;

struct Node {
  NodeKind kind; // ノードの型
  Node *lhs;     // 左側の木
  Node *rhs;     // 右側の木

  // if のための属性
  Node *cond; // 条件式
  Node *then; // true のときに実行する文
  Node *els;  // false のときに実行する文

  // for のための属性 (cond, then も含む)
  Node *init; // 初期化式
  Node *inc;  // 更新式

  // block のための属性
  Node *body; // ブロック内の文のリスト (連結リストで複数文を表現)
  Node *next;

  // 関数呼び出しのための属性
  Node *args;     // 引数
  char *funcname; // return に対応する関数名

  // 変数のための属性
  Var *var;

  // 関数定義の引数のための属性
  int val;    // kind が ND_NUM の場合、数値が入る
  int offset; // kind が ND_LVAR の場合、ベースポインタからのオフセットが入る
};

typedef struct Function Function;
struct Function {
  Function *next;
  char *funcname;  // 関数名
  VarList *params; // 引数のリスト
  VarList *locals; // 関数内のローカル変数
  Node *body;      // ブロック内の文のリスト (連結リストで複数文を表現)
  int stack_size;  // 関数のスタックのサイズ
};

// 入力プログラム (宣言)
extern char *user_input;

// 現在のトークン
extern Token *token;

// トークナイズ
void tokenize();

// パース
Function *program();

// コード生成
void gen(Node *node);

// エラーの出力
void error_at(char *loc, char *fmt, ...);

/**
 * codegen.c
 */

// コード生成
void codegen(Function *prog);
