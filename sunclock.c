/*

	Sun clock

	Designed and implemented by John Walker in November of 1988.

	Version for the Sun Workstation.

    The algorithm used to calculate the position of the Sun is given in
    Chapter 18 of:

    "Astronomical  Formulae for Calculators" by Jean Meeus, Third Edition,
    Richmond: Willmann-Bell, 1985.  This book can be obtained from:

       Willmann-Bell
       P.O. Box 35025
       Richmond, VA  23235
       USA
       Phone: (804) 320-7016

    This program was written by:

       John Walker
       Autodesk, Inc.
       2320 Marinship Way
       Sausalito, CA  94965
       USA
       Fax:   (415) 389-9418
       Voice: (415) 332-2344 Ext. 2829
       Usenet: {sun,well,uunet}!acad!kelvin
	   or: kelvin@acad.uu.net

    This  program is in the public domain: "Do what thou wilt shall be the
    whole of the law".  I'd appreciate  receiving  any  bug  fixes  and/or
    enhancements,  which  I'll  incorporate  in  future  versions  of  the
    program.  Please leave the original attribution information intact	so
    that credit and blame may be properly apportioned.

    Revision history:

	1.0  12/21/89  Initial version.
	      8/24/89  Finally got around to submitting.

*/

#include <stdio.h>
#include <ctype.h>
#include <math.h>
#include <sys/types.h>
#include <sys/timeb.h>
#include <assert.h>

#include <suntool/sunview.h>
#include <suntool/canvas.h>
#include <suntool/scrollbar.h>
#include <pixrect/pixrect_hs.h>

#define abs(x) ((x) < 0 ? (-(x)) : x)			  /* Absolute value */
#define sgn(x) (((x) < 0) ? -1 : ((x) > 0 ? 1 : 0))	  /* Extract sign */
#define dtr(x) ((x) * (PI / 180.0))			  /* Degree->Radian */
#define rtd(x) ((x) / (PI / 180.0))			  /* Radian->Degree */
#define fixangle(a) ((a) - 360.0 * (floor((a) / 360.0)))  /* Fix angle	  */

#define V      (void)

#define PI 3.14159265358979323846

#define TERMINC  100		   /* Circle segments for terminator */

#define PROJINT  (60 * 10)	   /* Frequency of seasonal recalculation
				      in seconds. */

#define CXDOTS	 126		   /* Closed window width */
#define CYDOTS	 63		   /* Closed window height */

#define IXDOTS	 128		   /* Total icon width */
#define IYDOTS	 74		   /* Total icon height */

#define OXDOTS	 640		   /* Open window width */
#define OYDOTS	 320		   /* Open window height */

/*  Globals imported  */

extern char *getenv(), *timezone(), *malloc(), *sprintf();
extern time_t time();
#ifdef lint
extern void pw_batch();
#endif

/*  Local variables  */

static int xdots, ydots;	   /* Screen size */
#include "sunclock.h"              /* Icon and open window bitmaps */
static char *wdname[] = {	   /* Week day names */
        "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
};
static	Frame	bf;
static	Pixwin	*fpw, *cpw, *apw;
static	Pixfont *tinyfont;
static	Pixfont *regfont;

static int onoon = -1;
static short *wtab, *wtab1, *wtabs;
static struct timeb tbtp;

static struct itimerval notif_timer = {{1, 0}, {1, 0}};
static struct itimerval quick_timer = {{0, 500000}, {0, 500000}};
static Menu actmenu;

static int fdate = FALSE, idir = 1, animate = FALSE;
static long lincr = 60;
static long cctime;

/*  Forward procedures	*/

double jtime(), gmst();
void drawterm(), sunpos(), projillum(), moveterm(), outdiff(), usage();

/*  UPDIMAGE  --  Update current displayed image.  */

static void updimage(istimer)
int istimer;
{
	int i, xl, isclosed;
	struct tm *ct;
	char tbuf[80];
	double jt, sunra, sundec, sunrv, sunlong, gt;
	struct tm lt;
	static int lisec = 61;	   /* Last iconic seconds */
	static long lctime = 0;    /* Last full calculation time */

	isclosed = (int) window_get(bf, FRAME_CLOSED);

	if (!istimer) {
	   if (isclosed) {
	      xdots = CXDOTS;
	      ydots = CYDOTS;

	      V pw_writebackground(fpw, 0, 0, IXDOTS, IYDOTS, PIX_SRC);
	      V pw_vector(fpw, 0, 0, IXDOTS - 1, 0, PIX_SRC, 1);
	      V pw_vector(fpw, IXDOTS - 1, 0, IXDOTS - 1,
			       IYDOTS - 1, PIX_SRC, 1);
	      V pw_vector(fpw, IXDOTS - 1, IYDOTS - 1, 0,
			       IYDOTS - 1, PIX_SRC, 1);
	      V pw_vector(fpw, 0, IYDOTS - 1, 0, 0, PIX_SRC, 1);
	      V pw_rop(fpw, 1, 1, micon.pr_size.x, micon.pr_size.y,
		    PIX_NOT(PIX_SRC),
		    &micon, 0, 0);
	   } else {
	      xdots = OXDOTS;
	      ydots = OYDOTS;

	      V pw_rop(cpw, 0, 0, bimage.pr_size.x, bimage.pr_size.y,
		    PIX_NOT(PIX_SRC),
		    &bimage, 0, 0);
	   }
	}

	/* If this is a full repaint of the window, force complete
	   recalculation. */

	if (!istimer) {
	   lctime = 0;
	   onoon = -1;
	   lisec = 61;
	   for (i = 0; i < OYDOTS; i++) {
	      wtab1[i] = -1;
	   }
	}

	if (fdate) {
	   if (animate)
	      cctime += lincr;
	   if (cctime < 0)
	      cctime = 0;
	} else {
	   V time(&cctime);
	}
	lt = *localtime(&cctime);

        /* Special  case  to  reduce overhead  whilst iconic: if we're
	   only showing the icon, update the  display  only  once  per
	   minute,  detected  by  the  fact  that  the current seconds
	   reading is less than that of the  last  update.   The  icon
	   shows  only	hours  and  minutes, and is sufficiently small
	   that once-a-minute updates are plenty to keep  the  picture
	   in sync.  */

	if (isclosed && !fdate && (lt.tm_sec > lisec))
	   return;

	ct = gmtime(&cctime);

	jt = jtime(ct);
	sunpos(jt, FALSE, &sunra, &sundec, &sunrv, &sunlong);
	gt = gmst(jt);

	/* Projecting the illumination curve  for the current seasonal
           instant is costly.  If we're running in real time, only  do
	   it every PROJINT seconds.  */

	if (fdate || !istimer || ((cctime - lctime) > PROJINT)) {
	   projillum(wtab, xdots, ydots, sundec);
	   wtabs = wtab;
	   wtab = wtab1;
	   wtab1 = wtabs;
	   lctime = cctime;
	}

	sunlong = fixangle(180.0 + (sunra - (gt * 15)));
	xl = sunlong * (xdots / 360.0);

	/* If the subsolar point has moved at least one pixel, update
	   the illuminated area on the screen.	*/

	if (fdate || !istimer || (onoon != xl)) {
	   apw = isclosed ? fpw : cpw;
	   pw_batch_on(apw);
	   moveterm(wtab1, xl, wtab, onoon, xdots, ydots);
	   pw_batch_off(apw);
	   onoon = xl;
	}

	if (isclosed) {

	   /* Display time in closed window */

           V sprintf(tbuf, "%02d:%02d %s %s %02d/%02d",
	      lt.tm_hour, lt.tm_min,
	      timezone(tbtp.timezone, lt.tm_isdst),
	      wdname[lt.tm_wday],
	      lt.tm_mon + 1, lt.tm_mday);
	   V pw_text(fpw, 8, IYDOTS - 3, PIX_SRC, tinyfont, tbuf);
	   lisec = lt.tm_sec;

	} else {

	   /* Display time in open window */

	   V sprintf(tbuf,
  " %02d:%02d:%02d %s %s %02d/%02d/%02d    %02d:%02d:%02d UTC %02d/%02d/%02d ",
	      lt.tm_hour, lt.tm_min, lt.tm_sec,
	      timezone(tbtp.timezone, lt.tm_isdst),
	      wdname[lt.tm_wday],
	      lt.tm_mon + 1, lt.tm_mday, (lt.tm_year % 100),
	      ct->tm_hour, ct->tm_min, ct->tm_sec,
	      ct->tm_mon + 1, ct->tm_mday, (ct->tm_year % 100));
	   V pw_text(cpw, 85, ydots - 6, PIX_SRC, regfont, tbuf);
	}
}

/*  Frame event processor  */

static frame_event_proc(frame, event, arg, type)
Frame frame;
Event *event;
Notify_arg arg;
Notify_event_type type;
{
	switch (event_id(event)) {

	   case WIN_REPAINT:
	      if (window_get(bf, FRAME_CLOSED)) {
		 updimage(FALSE);
	      } else {
		 xdots = OXDOTS;
		 ydots = OYDOTS;

		 updimage(FALSE);
	      }
	      break;

	   default:
	      window_default_event_func(frame, event, arg, type);
	      break;
	}
}

/*  Timer notification procedure.  */

/*ARGSUSED*/
static Notify_value timer_proc(frame, which)
Notify_client frame;
int which;
{
	updimage(TRUE);
	return(NOTIFY_DONE);
}

/*  CEVENT  --	Canvas event handler  */

/*ARGSUSED*/
static void cevent(window, event, arg)
Window window;
Event *event;
caddr_t arg;
{
#define mdis(x,y) V menu_set(menu_get(actmenu,MENU_NTH_ITEM,x),\
			     MENU_INACTIVE,y,0)

	mdis(1, idir > 0);
	mdis(2, idir < 0);
	mdis(11, !fdate);

	switch (event_id(event)) {

	   case MS_RIGHT:
	      switch (menu_show(actmenu, window, event, 0)) {

		 case 1:	   /* Forward */
		    if (idir < 0)
		       lincr = -lincr;
		    idir = 1;
		    break;

		 case 2:	   /* Backward */
		    if (idir > 0)
		       lincr = -lincr;
		    idir = -1;
		    break;

		 case 4:	   /* Hour */
		    cctime += (lincr = 3600L * idir);
		    fdate = TRUE;
		    break;

		 case 5:	   /* Day */
		    cctime += (lincr = 86400L * idir);
		    fdate = TRUE;
		    break;

		 case 6:	   /* Week */
		    cctime += (lincr = 86400L * 7 * idir);
		    fdate = TRUE;
		    break;

		 case 7:	   /* Month */
		    cctime += (lincr = 86400L * 30 * idir);
		    fdate = TRUE;
		    break;

		 case 8:	   /* Year */
		    cctime += (lincr = 86400L * 365L * idir);
		    fdate = TRUE;
		    break;

		 case 10:	   /* Animate */
		    animate = fdate = TRUE;
		    V notify_set_itimer_func(bf, timer_proc, ITIMER_REAL,
			  &quick_timer, (struct itimerval *) NULL);
		    break;

		 case 11:	   /* Real time */
		    animate = fdate = FALSE;
		    V notify_set_itimer_func(bf, timer_proc, ITIMER_REAL,
			  &notif_timer, (struct itimerval *) NULL);
		    updimage(FALSE);
		    break;

		 case 13:	   /* Quit */
		    V window_done(window);
		    break;
	      }
	}
}

/*  MAIN  --  Main program  */

void main(argc, argv)
int argc;
char *argv[];
{
	int i;
	char *op, opt;
	Icon icon;
	Canvas canvas;

        if (getenv("WINDOW_PARENT") == NULL) {
           V fprintf(stderr, "%s must be run from within Suntools\n", argv[0]);
	   exit(1);
	}

	/*  Process command line options.  */

	for (i = 1; i < argc; i++) {
	   op = argv[i];
           if (*op == '-') {
	      opt = *(++op);
	      if (islower(opt))
		 opt = toupper(opt);
	      switch (opt) {
                 case 'U':
                 case '?':
		    usage();
		    exit(0);
	      }
	   }
	}

	icon = icon_create(ICON_WIDTH, IXDOTS,
                  ICON_HEIGHT, IYDOTS, ICON_LABEL, "", 0);
	bf = window_create((Window) NULL, FRAME,
			      FRAME_LABEL,
           "Sun Clock          by John Walker, Autodesk, Inc.       v1.0",
			      FRAME_NO_CONFIRM, TRUE,
			      FRAME_ARGC_PTR_ARGV, &argc, argv,
			      FRAME_ICON, icon,
			      FRAME_CLOSED, TRUE,
			      WIN_EVENT_PROC, frame_event_proc,
			   0);
	canvas = window_create(bf, CANVAS,
				   WIN_EVENT_PROC, cevent,
			       0);

	V window_set(bf,
	   WIN_CONSUME_PICK_EVENTS,
	      WIN_NO_EVENTS, WIN_MOUSE_BUTTONS, LOC_DRAG, 0,
	   WIN_CONSUME_KBD_EVENT, WIN_LEFT_KEYS,
	   0);

	V window_set(canvas,
	   CANVAS_AUTO_EXPAND, FALSE,
	   CANVAS_AUTO_SHRINK, FALSE,
	   CANVAS_HEIGHT, OYDOTS,
	   CANVAS_WIDTH, OXDOTS,
	   WIN_HEIGHT, OYDOTS,
	   WIN_WIDTH, OXDOTS,
	   0);

	window_fit(canvas);

	window_fit(bf);
	fpw = (Pixwin *) window_get(bf, WIN_PIXWIN);
	cpw = canvas_pixwin(canvas);

        tinyfont = pf_open("/usr/lib/fonts/fixedwidthfonts/screen.r.7");
        regfont = pf_open("/usr/lib/fonts/fixedwidthfonts/screen.b.14");

	actmenu = menu_create(MENU_STRINGS,
           "Forward",
           "Backward",
           "",
           "Hour",
           "Day",
           "Week",
           "Month",
           "Year",
           "",
           "Animate",
           "Real time",
           "",
           "Quit",
	   0, 0);

	mdis(3, TRUE);		   /* Disable blank slots in menu */
	mdis(9, TRUE);
	mdis(12, TRUE);

	xdots = OXDOTS;
	ydots = OYDOTS;

	wtab = (short *) malloc((unsigned int) ydots * sizeof(short));
	wtab1 = (short *) malloc((unsigned int) ydots * sizeof(short));


	ftime(&tbtp);

	V notify_set_itimer_func(bf, timer_proc, ITIMER_REAL,
	      &notif_timer, (struct itimerval *) NULL);
	window_main_loop(bf);

	exit(0);
}

/*  JDATE  --  Convert internal GMT date and time to Julian day
	       and fraction.  */

static long jdate(t)
struct tm *t;
{
	long c, m, y;

	y = t->tm_year + 1900;
	m = t->tm_mon + 1;
	if (m > 2)
	   m = m - 3;
	else {
	   m = m + 9;
	   y--;
	}
	c = y / 100L;		   /* Compute century */
	y -= 100L * c;
	return t->tm_mday + (c * 146097L) / 4 + (y * 1461L) / 4 +
	    (m * 153L + 2) / 5 + 1721119L;
}

/* JTIME --    Convert internal GMT  date  and	time  to  astronomical
	       Julian  time  (i.e.   Julian  date  plus  day fraction,
	       expressed as a double).	*/

static double jtime(t)
struct tm *t;
{
	return (jdate(t) - 0.5) + 
	   (((long) t->tm_sec) +
	     60L * (t->tm_min + 60L * t->tm_hour)) / 86400.0;
}

/*  KEPLER  --	Solve the equation of Kepler.  */

static double kepler(m, ecc)
double m, ecc;
{
	double e, delta;
#define EPSILON 1E-6

	e = m = dtr(m);
	do {
	   delta = e - ecc * sin(e) - m;
	   e -= delta / (1 - ecc * cos(e));
	} while (abs(delta) > EPSILON);
	return e;
}

/*  SUNPOS  --	Calculate position of the Sun.	JD is the Julian  date
		of  the  instant for which the position is desired and
		APPARENT should be nonzero if  the  apparent  position
		(corrected  for  nutation  and aberration) is desired.
                The Sun's co-ordinates are returned  in  RA  and  DEC,
		both  specified  in degrees (divide RA by 15 to obtain
		hours).  The radius vector to the Sun in  astronomical
                units  is returned in RV and the Sun's longitude (true
		or apparent, as desired) is  returned  as  degrees  in
		SLONG.	*/

static void sunpos(jd, apparent, ra, dec, rv, slong)
double jd;
int apparent;
double *ra, *dec, *rv, *slong;
{
	double t, t2, t3, l, m, e, ea, v, theta, omega,
	       eps;

	/* Time, in Julian centuries of 36525 ephemeris days,
	   measured from the epoch 1900 January 0.5 ET. */

	t = (jd - 2415020.0) / 36525.0;
	t2 = t * t;
	t3 = t2 * t;

	/* Geometric mean longitude of the Sun, referred to the
	   mean equinox of the date. */

	l = fixangle(279.69668 + 36000.76892 * t + 0.0003025 * t2);

        /* Sun's mean anomaly. */

	m = fixangle(358.47583 + 35999.04975*t - 0.000150*t2 - 0.0000033*t3);

        /* Eccentricity of the Earth's orbit. */

	e = 0.01675104 - 0.0000418 * t - 0.000000126 * t2;

	/* Eccentric anomaly. */

	ea = kepler(m, e);

	/* True anomaly */

	v = fixangle(2 * rtd(atan(sqrt((1 + e) / (1 - e))  * tan(ea / 2))));

        /* Sun's true longitude. */

	theta = l + v - m;

	/* Obliquity of the ecliptic. */

	eps = 23.452294 - 0.0130125 * t - 0.00000164 * t2 + 0.000000503 * t3;

        /* Corrections for Sun's apparent longitude, if desired. */

	if (apparent) {
	   omega = fixangle(259.18 - 1934.142 * t);
	   theta = theta - 0.00569 - 0.00479 * sin(dtr(omega));
	   eps += 0.00256 * cos(dtr(omega));
	}

        /* Return Sun's longitude and radius vector */

	*slong = theta;
	*rv = (1.0000002 * (1 - e * e)) / (1 + e * cos(dtr(v)));

	/* Determine solar co-ordinates. */

	*ra =
	fixangle(rtd(atan2(cos(dtr(eps)) * sin(dtr(theta)), cos(dtr(theta)))));
	*dec = rtd(asin(sin(dtr(eps)) * sin(dtr(theta))));
}

/*  GMST  --  Calculate Greenwich Mean Siderial Time for a given
	      instant expressed as a Julian date and fraction.	*/

static double gmst(jd)
double jd;
{
	double t, theta0;


	/* Time, in Julian centuries of 36525 ephemeris days,
	   measured from the epoch 1900 January 0.5 ET. */

	t = ((floor(jd + 0.5) - 0.5) - 2415020.0) / 36525.0;

	theta0 = 6.6460656 + 2400.051262 * t + 0.00002581 * t * t;

	t = (jd + 0.5) - (floor(jd + 0.5));

	theta0 += (t * 24.0) * 1.002737908;

	theta0 = (theta0 - 24.0 * (floor(theta0 / 24.0)));

	return theta0;
}

/*  PROJILLUM  --  Project illuminated area on the map.  */

static void projillum(wtab, xdots, ydots, dec)
short *wtab;
int xdots, ydots;
double dec;
{
	int i, ftf = TRUE, ilon, ilat, lilon, lilat, xt;
	double m, x, y, z, th, lon, lat, s, c;

	/* Clear unoccupied cells in width table */

	for (i = 0; i < ydots; i++)
	   wtab[i] = -1;

	/* Build transformation for declination */

	s = sin(-dtr(dec));
	c = cos(-dtr(dec));

	/* Increment over a semicircle of illumination */

	for (th = -(PI / 2); th <= PI / 2 + 0.001;
	     th += PI / TERMINC) {

	   /* Transform the point through the declination rotation. */

	   x = -s * sin(th);
	   y = cos(th);
	   z = c * sin(th);

	   /* Transform the resulting co-ordinate through the
	      map projection to obtain screen co-ordinates. */

	   lon = (y == 0 && x == 0) ? 0.0 : rtd(atan2(y, x));
	   lat = rtd(asin(z));

	   ilat = ydots - (lat + 90) * (ydots / 180.0);
	   ilon = lon * (xdots / 360.0);

	   if (ftf) {

	      /* First time.  Just save start co-ordinate. */

	      lilon = ilon;
	      lilat = ilat;
	      ftf = FALSE;
	   } else {

	      /* Trace out the line and set the width table. */

	      if (lilat == ilat) {
		 wtab[(ydots - 1) - ilat] = ilon == 0 ? 1 : ilon;
	      } else {
		 m = ((double) (ilon - lilon)) / (ilat - lilat);
		 for (i = lilat; i != ilat; i += sgn(ilat - lilat)) {
		    xt = lilon + floor((m * (i - lilat)) + 0.5);
		    wtab[(ydots - 1) - i] = xt == 0 ? 1 : xt;
		 }
	      }
	      lilon = ilon;
	      lilat = ilat;
	   }
	}

	/* Now tweak the widths to generate full illumination for
	   the correct pole. */

	if (dec < 0.0) {
	   ilat = ydots - 1;
	   lilat = -1;
	} else {
	   ilat = 0;
	   lilat = 1;
	}

	for (i = ilat; i != ydots / 2; i += lilat) {
	   if (wtab[i] != -1) {
	      while (TRUE) {
		 wtab[i] = xdots / 2;
		 if (i == ilat)
		    break;
		 i -= lilat;
	      }
	      break;
	   }
	}
}

/*  XSPAN  --  Complement a span of pixels.  Called with line in which
	       pixels are contained, leftmost pixel in the  line,  and
	       the   number   of   pixels   to	 complement.   Handles
	       wrap-around at the right edge of the screen.  */

static void xspan(pline, leftp, npix)
int pline, leftp, npix;
{
	int xo, yo;

	if (window_get(bf, FRAME_CLOSED)) {
	   xo = 1;
	   yo = 1;
	} else {
	   xo = yo = 0;
	}

	leftp = leftp % xdots;
	pline += yo;

	if ((leftp + npix) > xdots) {
	   V pw_vector(apw, leftp + xo, pline, xo + (xdots - 1), pline,
		 PIX_SRC ^ PIX_DST, 1);
	   V pw_vector(apw, xo, pline, xo + ((leftp + npix) - (xdots + 1)),
		 pline, PIX_SRC ^ PIX_DST, 1);
	} else {
	   V pw_vector(apw, xo + leftp, pline, xo + leftp + (npix - 1), pline,
		 PIX_SRC ^ PIX_DST, 1);
	}
}

/*  MOVETERM  --  Update illuminated portion of the globe.  */

static void moveterm(wtab, noon, otab, onoon, xdots, ydots)
short *wtab, *otab;
int noon, onoon, xdots, ydots;
{
	int i, ol, oh, nl, nh;

	for (i = 0; i < ydots; i++) {

	   /* If line is off in new width table but is set in
	      the old table, clear it. */

	   if (wtab[i] < 0) {
	      if (otab[i] >= 0) {
		 xspan(i, ((onoon - otab[i]) + xdots) % xdots,
		    otab[i] * 2);
	      }
	   } else {

	      /* Line is on in new width table.  If it was off in
		 the old width table, just draw it. */

	      if (otab[i] < 0) {
		 xspan(i, ((noon - wtab[i]) + xdots) % xdots,
		    wtab[i] * 2);
	      } else {

		 /* If both the old and new spans were the entire
                    screen, they're equivalent. */

		 if ((otab[i] == wtab[i]) && (wtab[i] == (xdots / 2)))
		    continue;

		 /* The line was on in both the old and new width
		    tables.  We must adjust the difference in the
		    span.  */

		 ol =  ((onoon - otab[i]) + xdots) % xdots;
		 oh = (ol + otab[i] * 2) - 1;
		 nl =  ((noon - wtab[i]) + xdots) % xdots;
		 nh = (nl + wtab[i] * 2) - 1;

		 /* If spans are disjoint, erase old span and set
		    new span. */

		 if (oh < nl || nh < ol) {
		    xspan(i, ol, (oh - ol) + 1);
		    xspan(i, nl, (nh - nl) + 1);
		 } else {
		    /* Clear portion(s) of old span that extend
		       beyond end of new span. */
		    if (ol < nl) {
		       xspan(i, ol, nl - ol);
		       ol = nl;
		    }
		    if (oh > nh) {
		       xspan(i, nh + 1, oh - nh);
		       oh = nh;
		    }
		    /* Extend existing (possibly trimmed) span to
		       correct new length. */
		    if (nl < ol) {
		       xspan(i, nl, ol - nl);
		    }
		    if (nh > oh) {
		       xspan(i, oh + 1, nh - oh);
		    }
		 }
	      }
	   }
	   otab[i] = wtab[i];
	}
}

/*  USAGE  --  Print how-to-call information.  */

static void usage()
{
	V fprintf(stderr,
           "sunclock  --  The Sun clock.  By John Walker, Autodesk, Inc.\n");
}
