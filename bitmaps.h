/*
 * bitmaps for sun clock, X11 version; header file
 */

#ifdef	DEFINE_BITS
#define	EXTERN
#else
#define	EXTERN	extern
#endif

#define clock_icon_width 128
#define clock_icon_height 63
EXTERN char clock_icon_bits[]
#ifdef	DEFINE_BITS
#include "clock.bits.h"
#else
;
#endif

#define map_icon_width 640
#define map_icon_height 320
EXTERN char map_icon_bits[] 

#ifdef	DEFINE_BITS
#include "map.bits.h"
#else
;
#endif
