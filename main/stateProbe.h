#ifndef __STATEPROBE_H__
#define __STATEPROBE_H__

/*
 * Maximum length of probeSymbol of user is 54-22 = 32 Characters
 */
#if defined(DEBUG_STATE_PROBE)
#include <stdio.h>
#endif /*defined(DEBUG_STATE_PROBE)*/
#include <stdint.h>
#include <esp_err.h>

typedef void (*probeFn)(const char* data, int lenght);

typedef struct s_stateProbe_probe{
  struct s_stateProbe_probe *pNext;
  const char* probeSymbol;
  probeFn pProbeFn;
  char subscribed;
}stateProbe_probe;

/* Function IDs for sProbe endpoint */
#define SPROBE_FUNC_ID_NONE 0
#define SPROBE_FUNC_ID_LOG 1
#define SPROBE_FUNC_ID_CUSTOM 2

/* Maximum number of registered probes */
#define SPROBE_MAX_PROBES 10

/**@brief Initialises the stateProbe module
 */
void stateProbe_init(const char* device_uname, uint8_t len_device_uname);

/**@brief Second stage of initialisation for stateProbe module
 */
void stateProbe_startTask(void);

/**@brief Registers a custom probe function with the stateProbe module.
 * The registered function will be called when the probe event happens
 * The user is responsible for allocating and holding the memory needed for *pProbe
 *
 * The probe symbol can be any ascii string otherthan "stateProbe/dbg". The "stateProbe/dbg" is a special
 * probe symbol reserved for interactive state-probing
 */
void stateProbe_register(stateProbe_probe* pProbe, const char* probeSymbol, probeFn pProbeFunction);

/**@brief To send data back to the caller. Must be called from a callback context only
 */
void stateProbe_write(stateProbe_probe* probe, char* buf, int len);

/**@brief Send a log immediately under the name/log topic. Dont care if anyone listens
 */
void stateProbe_log(char* log);

esp_err_t stateProbe_prov_handler(uint32_t session_id, const uint8_t *inbuf, ssize_t inlen,
                                       uint8_t **outbuf, ssize_t *outlen, void *priv_data);

#if defined(DEBUG_STATE_PROBE)
#define stateProbe_printf(...)\
{\
  S8 dbg_buffer[16];\
  U8 length;\
  length = snprintf(dbg_buffer, 16, __VA_ARGS__);\
  stateProbe_write(dbg_buffer, length);\
}

#else
  #define stateProbe_printf(...)  /* Nothing to do */
#endif /*defined(DEBUG_STATE_PROBE)*/

void stateProbe_late_init(void);

#endif /*__STATEPROBE_H__*/
