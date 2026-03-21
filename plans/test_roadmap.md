# Regression Testing Roadmap: Phase 1 — Unity Unit Testing

## Objective
Implement unit tests for `nvCron.c` and `tmu.c` using the Unity testing framework, without modifying any functional firmware code. All test infrastructure will be isolated in a dedicated `test/` directory.

## Strategy
- Use Unity (C unit testing framework) + CMock (mocking framework) for host-based testing.
- Mock all hardware dependencies: NVS, I2C, system time, GPIO, and MQTT.
- Compile and run tests on Linux/macOS host using GCC.
- Test files will reside in `test/unit/` with corresponding mocks in `test/mocks/`.
- Build system will be managed via a separate `test/CMakeLists.txt`.
- Tests will be triggered via `make test` from the workspace root.

## Phase 1 Scope
- Test `nvCron.c` functions:
  - `nvCron_traverseList()`
  - `nvCron_writeMultipleEntry()`
  - `nvCron_readMultipleEntry()`
- Test `tmu.c` functions:
  - `tmu_set()`
  - `tmu_updateRTC()` (mock RTC call)
- Do not test `stateProbe`, `comm`, or hardware drivers in Phase 1.

## Implementation Plan

### Step 1: Set up test infrastructure
- [ ] Create `test/` directory structure:
  - `test/unit/` — Unity test files
  - `test/mocks/` — Mock implementations
  - `test/CMakeLists.txt` — Test build configuration
  - `test/unity/` — Unity framework source (downloaded)
  - `test/cmock/` — CMock framework source (downloaded)
- [ ] Download and integrate Unity and CMock from GitHub:
  - https://github.com/ThrowTheSwitch/Unity
  - https://github.com/ThrowTheSwitch/CMock
- [ ] Create `test/test_runner.c` to run all tests via `main()`
- [ ] Configure `test/CMakeLists.txt` to build tests with GCC and link Unity/CMock

### Step 2: Mock dependencies for `nvCron.c`
- [ ] Create `test/mocks/nvs_mock.c` and `test/mocks/nvs_mock.h`:
  - Mock `nvs_open()`, `nvs_get_blob()`, `nvs_set_blob()`, `nvs_erase_key()`, `nvs_commit()`
  - Use in-memory hash table to simulate NVS key-value store
- [ ] Create `test/mocks/actuator_mock.c` and `test/mocks/actuator_mock.h`:
  - Mock `actuator_on()`, `actuator_off()`, `actuator_reflectState()`
- [ ] Define `#define nvs_get_blob nvs_get_blob_mock` in test headers to override real calls

### Step 3: Write unit tests for `nvCron.c`
- [ ] Create `test/unit/test_nvCron.c`
- [ ] Test 1: `nvCron_traverseList()` returns correct next trigger time
  - Load 3 valid cron entries: (05:00, ON), (12:00, OFF), (23:59, ON)
  - Call with `t_now = 04:30` → expect return `05:00`
  - Call with `t_now = 05:00` → expect trigger and return `12:00`
- [ ] Test 2: `nvCron_writeMultipleEntry()` overwrites duplicates
  - Write two entries with same time+func → verify only one remains
- [ ] Test 3: `nvCron_readMultipleEntry()` returns correct number of entries
  - Write 5 entries → read buffer → verify count and data integrity
- [ ] Test 4: Invalid entries (time >= MIDNIGHT) are skipped

### Step 4: Mock dependencies for `tmu.c`
- [ ] Create `test/mocks/time_mock.c` and `test/mocks/time_mock.h`:
  - Mock `time()`, `localtime_r()`, `mktime()`, `settimeofday()`
  - Allow test to set fixed timestamps
- [ ] Create `test/mocks/ds1307_mock.c` and `test/mocks/ds1307_mock.h`:
  - Mock `ds1307_set_time()` and `ds1307_get_time()`
  - Return predefined values or errors
- [ ] Define `#define ds1307_set_time ds1307_set_time_mock` in test headers

### Step 5: Write unit tests for `tmu.c`
- [ ] Create `test/unit/test_tmu.c`
- [ ] Test 1: `tmu_set()` successfully parses valid time string
  - Input: `"Wed Mar 07 05:00:00 2026"` → verify `settimeofday()` called with correct timestamp
- [ ] Test 2: `tmu_set()` rejects malformed string
  - Input: `"invalid"` → verify error log and no `settimeofday()` call
- [ ] Test 3: `tmu_updateRTC()` spawns task only if `BOARD_CFG_USE_DS1307` is defined
  - Mock `xTaskCreate()` → verify called once when enabled, never when disabled

### Step 6: Integrate and validate test build
- [ ] Add `test/` to workspace root CMakeLists.txt (only if building test target)
- [ ] Run `cd test && cmake . && make` to build test executable
- [ ] Run `./test_runner` and verify all tests pass
- [ ] Add `make test` alias in root `CMakeLists.txt`

### Step 7: Document and automate
- [ ] Write `test/README.md` with setup instructions
- [ ] Add `test/` to `.gitignore` for build artifacts
- [ ] Create GitHub Actions workflow to run tests on push (future phase)

## Next Steps After Phase 1
- Phase 2: Add unit tests for `stateProbe.c` and `comm.c`
- Phase 3: Integrate with CI/CD pipeline
- Phase 4: Implement HIL test automation

## Notes
- All test files must be kept separate from `main/` — no `#include "main/nvCron.c"` allowed.
- Use `#include "main/nvCron.h"` and `#include "main/tmu.h"` to access declarations.
- Mocks must be written to simulate behavior, not just stub.
- All tests must be deterministic and independent.