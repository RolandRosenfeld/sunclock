/*
 * Sun clock definitions.
 */

#define XK_MISCELLANY
#define XK_LATIN1

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysymdef.h>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include "version.h"
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

#define	FAILFONT	"fixed"
#define EARTHRADIUS_KM  6378.125
#define EARTHRADIUS_ML  3963.180
#define SUN_APPDIAM     0.533      /* Sun apparent diameter, in degrees */

#define SP		"         "

#define COORDINATES 'c'
#define DISTANCES 'd'
#define EXTENSION 'e'
#define LEGALTIME 'l'
#define SOLARTIME 's'

#define TIMECOUNT 25
#define PRECOUNT  TIMECOUNT-1
#define TIMESTEP  10000
#define CHWID	  24

enum {RANDOM=0, FIXED, CENTER, NW, NE, SW, SE};


