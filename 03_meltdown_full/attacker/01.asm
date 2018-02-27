BITS 64

global ooe
global memory_access

section .text

ooe:
  movzx rax, byte [rcx]
  shl rax, 0Ch
  jz ooe
  mov al, byte [rdx + rax]
  ret

memory_access:
  mov r9, rcx

  rdtscp
  shl rdx, 20h
  or rax, rdx
  mov r8, rax

  mov rax, [r9]

  rdtscp
  shl rdx, 20h
  or rax, rdx

  sub rax,r8
  ret
