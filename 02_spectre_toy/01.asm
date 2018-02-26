BITS 64

global branch_predictor
global memory_access

section .text

branch_predictor:
  cmp ecx, dword [r8]
  jae .skip_access

  movzx eax, byte [rdx + rcx]
  shl rax, 0Ch
  movzx eax, byte [r9 + rax]

.skip_access:
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
