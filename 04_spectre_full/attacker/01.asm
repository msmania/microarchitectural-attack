global memory_access
global flush_reload
global evict

section .text

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

flush_reload:
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
  clflush [r9]
  ret

evict:
  mov al, byte [rcx]
  add rcx, r8
  dec edx
  jnz evict
  ret
