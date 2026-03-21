#ifndef __TMU_MOCK_H__
#define __TMU_MOCK_H__

#include "tmu.h"

// Mock function declarations
void tmu_set(const char* buffer, int length);
void tmu_updateRTC(void);

// Getter functions for test assertions
int tmu_mock_get_time_set_called(void);
int tmu_mock_get_update_rtc_called(void);
void tmu_mock_reset(void);

#endif /* __TMU_MOCK_H__ */