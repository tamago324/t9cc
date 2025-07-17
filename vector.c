#include "9cc.h"
#include <stdio.h>
#include <stdlib.h>

// 初期化
NodeVector *node_vec_new() {
  NodeVector *v = malloc(sizeof(NodeVector));
  // v->data のサイズは、ポインタ (Node*) ののサイズ分、確保する必要がある
  v->capacity = 2;
  v->data = malloc(sizeof(Node *) * v->capacity);
  v->size = 0;
  return v;
}

// 要素追加
void node_vec_push(NodeVector *v, Node *node) {
  if (v->size >= v->capacity) {
    v->capacity *= 2;
    // v->data のサイズは、ポインタ (Node*) ののサイズ分、確保する必要がある
    v->data = realloc(v->data, sizeof(Node *) * v->capacity);
  }
  v->data[v->size++] = node;
}

// 要素取得
Node *node_vec_get(NodeVector *v, int index) {
  if (index >= v->size)
    return NULL;
  return v->data[index];
}

// メモリ解放
void node_vec_free(NodeVector *v) {
  free(v->data);
  free(v);
}