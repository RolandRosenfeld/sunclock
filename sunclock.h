/*
 * Sun clock definitions.
 */

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include "patchlevel.h"
#include "bitmaps.h"

#define abs(x) ((x) < 0 ? (-(x)) : x)			  /* Absolute value */
#define sgn(x) (((x) < 0) ? -1 : ((x) > 0 ? 1 : 0))	  /* Extract sign */
#define dtr(x) ((x) * (PI / 180.0))			  /* Degree->Radian */
#define rtd(x) ((x) / (PI / 180.0))			  /* Radian->Degree */
#define fixangle(a) ((a) - 360.0 * (floor((a) / 360.0)))  /* Fix angle	  */

#define PI 3.14159265358979323846

#define TERMINC  100		   /* Circle segments for terminator */

#define PROJINT  (60 * 10)	   /* Frequency of seasonal recalculation
				      in seconds. */
