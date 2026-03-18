#ifndef __FIXED_TYPES_H__
#define __FIXED_TYPES_H__

typedef unsigned char       U8;
typedef unsigned short      U16;
typedef unsigned long       U32;
typedef unsigned long long  U64;
typedef char                S8;
typedef short               S16;
typedef long                S32;
typedef long long           S64;

/* Utility Macros */
#if 0
#define BIT(reg, offset)\
  ((reg>>offset) & 0x1)
#endif // 0

#define CLEAR_BIT(reg, offset)\
  reg &= ~(1<<offset)

#define SET_BIT(reg, offset)\
  reg |= (1<<offset)

#define TOGGLE_BIT(reg, offset)\
  reg ^= (1<<offset)

#define UPDATE_PORT(reg, mask, value)\
  reg = ( ~mask & reg ) | ( mask & value)

#ifndef NULL
  #define NULL 0x0
#endif /*NULL*/

#endif /*__FIXED_TYPES_H__*/
