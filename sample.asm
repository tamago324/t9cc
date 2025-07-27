.intel_syntax noprefix
.global main
main:
; プロローグ
  push rbp
  mov rbp, rsp
  sub rsp, 16

; x=1;
  mov rax, rbp
  sub rax, 16
  push rax
  push 1
  pop rdi
  pop rax
  mov [rax], rdi
  push rdi
  add rsp, 8

; y=&x
  mov rax, rbp
  sub rax, 8
  push rax
  ; &x
  mov rax, rbp
  sub rax, 16
  push rax
  pop rdi
  pop rax
  mov [rax], rdi
  push rdi
  add rsp, 8

; *y=3;
  ; yのアドレスを計算
  mov rax, rbp
  sub rax, 8
  push rax
    ; yのアドレスをraxに代入
  pop rax
  mov rax, [rax]
  push rax
  ; raxのアドレスに3を代入
  pop rax
  mov rax, [rax]  ; ここで、さらにアドレスを持ってきてしまっている？
  push rax

  push 3
  pop rdi
  pop rax
  mov [rax], rdi
  push rdi
  add rsp, 8

  mov rax, rbp
  sub rax, 16
  push rax
  pop rax
  mov rax, [rax]
  push rax
  pop rax
  jmp Lreturn.main
Lreturn.main:
  mov rsp, rbp
  pop rbp
  ret