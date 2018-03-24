#include <windows.h>
#include <intrin.h>
#include <stdio.h>
#include <cstdint>
#include <vector>
#include <algorithm>

extern "C" {
  uint64_t memory_access(const void*);
}

void Log(LPCWSTR format, ...) {
  WCHAR linebuf[1024];
  va_list v;
  va_start(v, format);
  wvsprintf(linebuf, format, v);
  va_end(v);
  OutputDebugString(linebuf);
}

bool enable_large_page() {
  bool ret = false;
  HANDLE process_token = nullptr;
  TOKEN_PRIVILEGES privilege;

  if (!OpenProcessToken(GetCurrentProcess(),
                        TOKEN_ADJUST_PRIVILEGES,
                        &process_token)) {
    Log(L"OpenProcessToken failed - 0x%08x\n", GetLastError());
    goto cleanup;
  }

  privilege.PrivilegeCount = 1;
  privilege.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

  if (!LookupPrivilegeValue(/*lpSystemName*/nullptr,
                            L"SeLockMemoryPrivilege",
                            &privilege.Privileges[0].Luid)) {
    Log(L"LookupPrivilegeValue failed - 0x%08x\n", GetLastError());
    goto cleanup;
  }

  if (!AdjustTokenPrivileges(process_token,
                             /*DisableAllPrivileges*/FALSE,
                             &privilege,
                             sizeof(privilege),
                             /*PreviousState*/nullptr,
                             /*ReturnLength*/nullptr)) {
    Log(L"LookupPrivilegeValue failed - 0x%08x\n", GetLastError());
    goto cleanup;
  }

  ret = true;

cleanup:
  if (process_token) CloseHandle(process_token);
  return ret;
}

class L3_cache_analyzer final {
private:
  static constexpr uint32_t L3         = 10 * (1 << 20);
  static constexpr uint32_t L3_ASSOC   = 20;
  static constexpr uint32_t CACHE_LINE = 1 << 6;
  static constexpr uint32_t CACHE_SET  = L3 / L3_ASSOC;
  static constexpr uint32_t SET_INDEX_SIZE = CACHE_SET / CACHE_LINE;

  static constexpr int PAGES = 32;
  static constexpr uint32_t L3_hit_threshold = 150;

  static
  uint64_t probe(uint8_t * const set[],
                 size_t set_count,
                 uint8_t *candidate) {
    volatile auto junk = *candidate;
    for (uint32_t i = 0; i < set_count; ++i) {
      junk ^= *set[i];
      _mm_lfence();
    }
    return memory_access(candidate);
  }

  const size_t large_page_size;
  uint8_t *large_page_region_;
  std::vector<uint8_t*> lines_;
  std::vector<uint8_t*> conflict_set_;

public:
  L3_cache_analyzer(uint32_t target_set_index)
    : large_page_size(GetLargePageMinimum()),
      large_page_region_(nullptr),
      lines_(PAGES * large_page_size / CACHE_SET) {
    large_page_region_ =
      reinterpret_cast<uint8_t*>(VirtualAlloc(nullptr,
                                              large_page_size * PAGES,
                                              MEM_COMMIT | MEM_RESERVE | MEM_LARGE_PAGES,
                                              PAGE_READWRITE));
    if (large_page_region_) {
      target_set_index &= (SET_INDEX_SIZE - 1);
      for (uint32_t i = 0; i < lines_.size(); ++i) {
        const int j = (i * 107 + 13) % lines_.size(); // simple randomization
        lines_[j] = large_page_region_
                    + i * CACHE_SET
                    + target_set_index * CACHE_LINE;
      }
      printf("# lines: %llu\n", lines_.size());
    }
    else {
      Log(L"VirtualAlloc failed - %08x\n", GetLastError());
    }
  }

  ~L3_cache_analyzer() {
    if (large_page_region_)
      VirtualFree(large_page_region_, 0, MEM_RELEASE);
  }

  void update_conflict_set(uint32_t trials) {
    std::vector<int> counts(lines_.size());
    for (uint32_t trial = 0; trial < trials; ++trial) {
      std::vector<uint32_t> conflict_set;
      for (uint32_t i = 0; i < lines_.size(); ++i) {
        auto candidate = lines_[i];
        auto latency = probe(conflict_set_.data(), conflict_set_.size(), candidate);
        //printf("%llu\n", latency);
        if (latency < L3_hit_threshold) {
          conflict_set_.push_back(candidate);
          ++counts[i];
        }
      }
    }

    conflict_set_.clear();
    for (uint32_t i = 0; i < lines_.size(); ++i) {
      if (counts[i] > trials * 0.9)
        conflict_set_.push_back(lines_[i]);
    }
    printf("# conflict_set: %llu\n", conflict_set_.size());
  }

  void update_eviction_set() {
    std::vector<uint8_t*> non_conflict_set;
    for (auto it : lines_) {
      if (std::find(conflict_set_.begin(),
                    conflict_set_.end(),
                    it) == conflict_set_.end()) {
        non_conflict_set.push_back(it);
      }
    }

    for (auto candidate : non_conflict_set) {
      if (probe(conflict_set_.data(), conflict_set_.size(), candidate) > L3_hit_threshold) {
        std::vector<uint32_t> eviction_set_count(conflict_set_.size());
        for (int trial = 0; trial < 1000; ++trial) {
          for (uint32_t i = 0; i < conflict_set_.size(); ++i) {
            std::swap(conflict_set_[i], conflict_set_[conflict_set_.size() - 1]);
            auto latency = probe(conflict_set_.data(),
                                 conflict_set_.size() - 1,
                                 candidate);
            if (latency < L3_hit_threshold) ++eviction_set_count[i];
            std::swap(conflict_set_[i], conflict_set_[conflict_set_.size() - 1]);
            //printf("%llu\n", latency);
          }
        }
#if 1
        for (uint32_t i = 0; i < conflict_set_.size(); ++i) {
          //if (eviction_set_count[i] > 90)
            printf("%u\n", eviction_set_count[i]);
        }
#endif

#if 0
          for (uint32_t i = 0; i < conflict_set_.size(); ++i) {
            std::vector<uint8_t*> conflict_set_minus_l;
            conflict_set_minus_l.reserve(conflict_set_.size() * 2);
            for (uint32_t j = 0; j < conflict_set_.size(); ++j) {
              if (i != j) conflict_set_minus_l.push_back(conflict_set_[j]);
            }
            if (probe(conflict_set_minus_l, candidate) < L3_hit_threshold) {
              //++eviction_set_count[i];
              printf("%d\n", i);
              break;
            }
          }
        }

        for (uint32_t j = 0; j < eviction_set_count.size(); ++j) {
          if (eviction_set_count[j] > 0) {
            //printf("%d\n", eviction_set_count[j]);
          }
          //if ((j + 1) % 16 == 0)
          //  printf("% 4d\n", eviction_set_count[j]);
          //else
          //  printf("% 4d", eviction_set_count[j]);
        }
#endif

        return;
      }
    }
  }
};

int main() {
  if (enable_large_page()) {
    L3_cache_analyzer cache(/*target_set_index*/0xdead);
    cache.update_conflict_set(/*trials*/100);
    cache.update_eviction_set();
  }
  return 0;
}
