#ifndef __TMU_H__
#define __TMU_H__

void tmuRemote_init(void);

void tmu_set(const char* buffer, int length);

void tmu_updateRTC(void);

void tmu_init(void);

#endif /*__TMU_H__*/
