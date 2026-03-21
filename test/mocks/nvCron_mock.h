#ifndef __NVCRON_MOCK_H__
#define __NVCRON_MOCK_H__

#include "nvCron.h"

// Mock function declarations
int nvCron_readMultipleEntry(char* buffer, int length);
void nvCron_writeMultipleEntry(const char* buffer, int length);
void nvCron_mock_reset(void);

#endif /* __NVCRON_MOCK_H__ */