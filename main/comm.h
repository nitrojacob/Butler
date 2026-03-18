#ifndef __COMM_H__
#define __COMM_H__

#ifdef __cplusplus
extern "C" {
#endif

/** Initializes the wifi, takes care of wifi provisioning and connection to internet.
 * 
 */
void commInit(void);

#ifdef __cplusplus
}
#endif

#endif // __COMM_H__