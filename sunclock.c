/*
 * Sun clock.  X11 version by John Mackin.
 *
 * This program was derived from, and is still in part identical with, the
 * Suntools Sun clock program whose author's comment appears immediately
 * below.  Please preserve both notices.
 *
 * The X11R3/4 version of this program was written by John Mackin, at the
 * Basser Department of Computer Science, University of Sydney, Sydney,
 * New South Wales, Australia; <john@cs.su.oz.AU>.  This program, like
 * the one it was derived from, is in the public domain: `Love is the
 * law, love under will.'
 */

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

    modified for interactice maps by

	Stephen Martin
	Fujitsu Systems Business of Canada
	smartin@fujitsu.ca

    This  program is in the public domain: "Do what thou wilt shall be the
    whole of the law".  I'd appreciate  receiving  any  bug  fixes  and/or
    enhancements,  which  I'll  incorporate  in  future  versions  of  the
    program.  Please leave the original attribution information intact	so
    that credit and blame may be properly apportioned.

    Revision history:

	1.0  12/21/89  Initial version.
	      8/24/89  Finally got around to submitting.

	1.1   8/31/94  Version with interactive map.
	1.2  10/12/94  Fixes for HP and Solaris, new icon bitmap
	1.3  11/01/94  Timezone now shown in icon

    next minor revisions by Jean-Pierre Demailly (demailly@ujf-grenoble.fr)
	1.4  04/03/99  Change in city management
	1.5  15/03/99  Color support
	1.6  28/03/99  Iconic stuff fixed
	1.7  17/08/99  Calculation of city distances
	1.8  20/08/99  Fixed small bug in mono mode
	1.9  21/01/00  Iconic stuff fixed again (didn't work for all WMHints!)
	2.0  21/04/00  Added XFlush in doTimeout to force update
        2.1  06/06/00  Updated latlong.txt with a better source 
*/

#define	FAILFONT	"fixed"
#define EARTHRADIUS_KM  6378.125
#define EARTHRADIUS_ML  3963.180

#include "sunclock.h"
#include <sys/types.h>
#include <sys/timeb.h>
#include <string.h>

struct sunclock {
	int		s_width;	/* size of pixmap */
	int		s_height;
	Window		s_window;	/* associated window */
	Pixmap		s_pixmap;	/* and pixmap */
	int		s_flags;	/* see below */
	int		s_noon;		/* position of noon */
	short *		s_wtab1;	/* current width table (?) */
	short *		s_wtab;		/* previous width table (?) */
	long		s_increm;	/* increment for fake time */
	long		s_time;		/* time - real or fake, see flags */
	GC		s_gc;		/* GC for writing text into window */
	char *		(*s_tfunc)();	/* function to return the text */
	char		s_text[120];	/* and the current text that's there */
	int		s_textx;	/* where to draw the text */
	int		s_texty;	/* where to draw the text */
	long		s_projtime;	/* last time we projected illumination */
	int		s_timeout;	/* time until next image update */
	struct sunclock * s_next;	/* pointer to next clock context */
};

/* Records to hold cities */

typedef struct City {
    char *city;		/* Name of the city */
    double lat, lon;	/* Latitude and longitude of city */
    char *tz;		/* Timezone of city */
    int mode;
    struct City *next;	/* Pointer to next record */
} City;

City *cptr1, *cptr2, *cities = NULL;

/*
 * bits in s_flags
 */

#define	S_FAKE		01		/* date is fake, don't use actual time */
#define	S_ANIMATE	02		/* do animation based on increment */
#define	S_DIRTY		04		/* pixmap -> window copy required */
#define	S_ICON		010		/* this is the icon window */

void                            SetIconName();
/* char *				strdup(); */
char *				strrchr();
char *				strtok();
long				time();
#ifdef NEW_CTIME
char *				timezone();
#endif

double				jtime();
double				gmst();
char *				salloc();
char *				bigtprint();
char *				smalltprint();

struct sunclock *		makeClockContext();
Bool				evpred();

char app_default[] = "/usr/lib/X11/app-defaults/Sunclock";

char *				Wdayname[] = {
        "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
};
char *				Monname[] = {
	"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul",
	"Aug", "Sep", "Oct", "Nov", "Dec"
};

struct geom {
	int			mask;
	int			x;
	int			y;
};

char *				ProgName;
char *                          CityName = "";
char                            Location[40] = "";
char                            BgColorName[40] = "White";
char                            FgColorName[40] = "Black";
char                            TextBgColorName[40] = "LightSeaGreen";
char                            TextFgColorName[40] = "Black";
char                            CityColorName0[40] = "Orange";
char                            CityColorName1[40] = "Red";
char                            CityColorName2[40] = "Red";

int                             maxlength;
int                             size_spot = 2;
int                             mono = 0;
int                             showcoord = 1;
int                             showdist = 1;
int                             showdelay = 5;
int		                clock_height;
int		                icon_height;
int                             repeatcount;
int                             timedelay = 0;

char *				Display_name = "";
Display *			dpy;
int				scr;
unsigned long			FgColor;
unsigned long			BgColor;
unsigned long			TextBgColor;
unsigned long			TextFgColor;
unsigned long			CityColor0;
unsigned long			CityColor1;
unsigned long			CityColor2;
GC				GC_store;
GC				GC_invert;
GC				GC_citycolor0;
GC				GC_citycolor1;
GC				GC_citycolor2;
GC				GC_bigf;
GC				GC_smallf;
XFontStruct *			SmallFont;
XFontStruct *			BigFont;
Pixmap				Mappix;
Pixmap				Iconpix;
Window				Icon;
Window				Clock;
struct sunclock *		Current;
int                             Quit = 0;
int				Iconic = 0;
struct geom			Geom = { 0, 0, 0 };
struct geom			Icongeom = { 0, 0, 0 };

main(argc, argv)
int				argc;
register char **		argv;
{
	char *			p;
	City *c;		/* Used to process cities */

	/* Read the ~/.sunclockrc file */

	if (readrc())
	    exit(1);
	ProgName = *argv;
	if (p = strrchr(ProgName, '/'))
		ProgName = ++p;
	parseArgs(argc, argv);

	dpy = XOpenDisplay(Display_name);
	if (dpy == (Display *)NULL) {
		fprintf(stderr, "%s: can't open display `%s'\n", ProgName,
			Display_name);
		exit(1);
	}
	scr = DefaultScreen(dpy);

	getColors();
	getFonts();
	makePixmaps();
	makeWindows();
	makeGCs(Clock, Mappix);
	setAllHints(argc, argv);
	makeClockContexts();

	/* Add cities to the map */

	XSelectInput(dpy, Clock, ExposureMask | ButtonPressMask);
	XSelectInput(dpy, Icon, ExposureMask | ButtonPressMask);

	if (Iconic)
          XMapWindow(dpy, Icon);
        else
	  XMapWindow(dpy, Clock);

	eventLoop();

	/*
	 * eventLoop() never returns, but one day it might, if someone adds a
	 * menu for animation or such with a "quit" option.
	 */

	/* shutDown(); */
	exit(0);
}

parseArgs(argc, argv)
register int			argc;
register char **		argv;
{
	while (--argc > 0) {
		++argv;
                if (strcmp(*argv, "-mono") == 0) {
                        strcpy(TextBgColorName, "White");
                        size_spot = 3;
			mono = 1;
		}
		else if (strcmp(*argv, "-nocoord") == 0) {
			showcoord = 0;
		}
		else if (strcmp(*argv, "-nodist") == 0) {
			showdist = 0;
		}
		else if (strcmp(*argv, "-showdelay") == 0 && argc>1) {
			showdelay = atoi(*++argv);
                        if (showdelay<=0) showdist = 0;
                        if (showdelay>=15) showdelay = 15;
                        --argc;
		}
		else if (strcmp(*argv, "-spot") == 0 && argc>1) {
			needMore(argc, argv);
			size_spot = atoi(*++argv);
                        if (size_spot<0) size_spot = 0;
                        if (size_spot>3) size_spot = 3;
                        --argc;
		}
		else if (strcmp(*argv, "-display") == 0 && argc>1) {
			needMore(argc, argv);
			Display_name = *++argv;
			--argc;
		}
		else if (strcmp(*argv, "-bg") == 0 && argc>1) {
			needMore(argc, argv);
			strcpy(BgColorName, *++argv);
			--argc;
		}
		else if (strcmp(*argv, "-fg") == 0 && argc>1) {
			needMore(argc, argv);
			strcpy(FgColorName, *++argv);
			--argc;
		}
		else if (strcmp(*argv, "-textbg") == 0 && argc>1) {
			needMore(argc, argv);
			strcpy(TextBgColorName, *++argv);
			--argc;
		}
		else if (strcmp(*argv, "-textfg") == 0 && argc>1) {
			needMore(argc, argv);
			strcpy(TextFgColorName, *++argv);
			--argc;
		}
		else if (strcmp(*argv, "-city0") == 0 && argc>1) {
			needMore(argc, argv);
			strcpy(CityColorName0, *++argv);
			--argc;
		}
		else if (strcmp(*argv, "-city1") == 0 && argc>1) {
			needMore(argc, argv);
			strcpy(CityColorName1, *++argv);
			--argc;
		}
		else if (strcmp(*argv, "-city2") == 0 && argc>1) {
			needMore(argc, argv);
			strcpy(CityColorName2, *++argv);
			--argc;
		}
		else if (strcmp(*argv, "-iconic") == 0)
			Iconic++;
		else if (strcmp(*argv, "-geometry") == 0 && argc>1) {
			needMore(argc, argv);
			getGeom(*++argv, &Geom);
			--argc;
		}
		else if (strcmp(*argv, "-icongeom") == 0 && argc>1) {
			needMore(argc, argv);
			getGeom(*++argv, &Icongeom);
			--argc;
		}
		else if (strcmp(*argv, "-version") == 0) {
			fprintf(stderr, "%s: version %s patchlevel %d\n",
				ProgName, VERSION, PATCHLEVEL);
			exit(0);
		}
		else
			usage();
	}
}

needMore(argc, argv)
register int			argc;
register char **		argv;
{
	if (argc == 1) {
		fprintf(stderr, "%s: option `%s' requires an argument\n",
			ProgName, *argv);
		usage();
	}
}

getGeom(s, g)
register char *			s;
register struct geom *		g;
{
	register int		mask;
	unsigned int		width;
	unsigned int		height;

	mask = XParseGeometry(s, &g->x, &g->y, &width, &height);
	if (mask == 0) {
		fprintf(stderr, "%s: `%s' is a bad geometry specification\n",
			ProgName, s);
		exit(1);
	}
	if ((mask & WidthValue) || (mask & HeightValue))
		fprintf(stderr, "%s: warning: width/height in geometry `%s' ignored\n",
			ProgName, s);
	g->mask = mask;
}

/*
 * Free resources.
 */

shutDown()
{
	XFreeGC(dpy, GC_store);
	XFreeGC(dpy, GC_invert);
	XFreeGC(dpy, GC_bigf);
	XFreeGC(dpy, GC_smallf);
	XFreeFont(dpy, BigFont);
	XFreeFont(dpy, SmallFont);
	XFreePixmap(dpy, Mappix);
	XFreePixmap(dpy, Iconpix);
	XDestroyWindow(dpy, Clock);
	XDestroyWindow(dpy, Icon);
	XCloseDisplay(dpy);
}

usage()
{
#define SP "         "
	fprintf(stderr, "%s: version %s patchlevel %d\nUsage:\n"
         SP"%s [-display dispname] [-geometry +x+y] [-icongeom +x+y]\n"
         SP"[-mono] [-iconic] [-version] [-nocoord] [-nodist]\n"
         SP"[-showdelay duration(s)] [-spot size]\n"
         SP"[-bg color] [-fg color] [-textbg color] [-textfg color]\n"
         SP"[-city0 color] [-city1 color] [-city2 color]\n", 
         ProgName, VERSION, PATCHLEVEL, ProgName);
	exit(1);
}

/*
 * Set up stuff the window manager will want to know.  Must be done
 * before mapping window, but after creating it.
 */

setAllHints(argc, argv)
int				argc;
char **				argv;
{
	XClassHint		xch;
	XSizeHints		xsh;
	XWMHints		xwmh;
        Atom                    protocols;

	xch.res_name = ProgName;
	xch.res_class = "Sunclock";
	XSetClassHint(dpy, Clock, &xch);
	XSetClassHint(dpy, Icon, &xch);
	XStoreName(dpy, Clock, ProgName);
	XStoreName(dpy, Icon, ProgName);

	XSetCommand(dpy, Clock, argv, argc);
	XSetCommand(dpy, Icon, argv, argc);

	SetIconName();

	xsh.flags = PSize | PMinSize | PMaxSize;
	if (Geom.mask & (XValue | YValue)) {
		xsh.x = Geom.x;
		xsh.y = Geom.y;
		xsh.flags |= USPosition;
	}
	xsh.width = xsh.min_width = xsh.max_width = large_map_width;
	xsh.height = xsh.min_height = xsh.max_height = clock_height;
	XSetNormalHints(dpy, Clock, &xsh);

	xwmh.flags = InputHint | StateHint | IconWindowHint;
	if (Icongeom.mask & (XValue | YValue)) {
		xwmh.icon_x = Icongeom.x;
		xwmh.icon_y = Icongeom.y;
		xwmh.flags |= IconPositionHint;
	}

	xwmh.input = False;
	xwmh.initial_state = Iconic ? IconicState : NormalState;
	xwmh.icon_window = Icon;
	XSetWMHints(dpy, Clock, &xwmh);
	if (Iconic) XSetWMProtocols(dpy, Clock, &protocols, 0);
}

makeWindows()
{
	XSetWindowAttributes	xswa;
	register int		mask;

	clock_height = large_map_height + BigFont->max_bounds.ascent +
	     BigFont->max_bounds.descent +2 ;
	icon_height = icon_map_height + SmallFont->max_bounds.ascent +
	     SmallFont->max_bounds.descent + 2;

	xswa.background_pixel = TextBgColor;
	xswa.border_pixel = FgColor;
	xswa.backing_store = WhenMapped;
	mask = CWBackPixel | CWBorderPixel | CWBackingStore;

	fixGeometry(&Geom, large_map_width, clock_height);
	Clock = XCreateWindow(dpy, RootWindow(dpy, scr), Geom.x, Geom.y,
	      large_map_width, clock_height, 3, CopyFromParent,
	      InputOutput, CopyFromParent, mask, &xswa);

        fixGeometry(&Icongeom, icon_map_width, icon_height);
	Icon = XCreateWindow(dpy, RootWindow(dpy, scr), Icongeom.x, Icongeom.y,
			     icon_map_width, icon_height, 1, CopyFromParent, InputOutput,
			     CopyFromParent, mask, &xswa);
}

fixGeometry(g, w, h)
register struct geom *		g;
register int			w;
register int			h;
{
	if (g->mask & XNegative)
		g->x = DisplayWidth(dpy, scr) - w + g->x;
	if (g->mask & YNegative)
		g->y = DisplayHeight(dpy, scr) - h + g->y;
}

makeGCs(w, p)
register Window			w;
register Pixmap			p;
{
	XGCValues		gcv;

	gcv.background = BgColor;
	gcv.foreground = FgColor;
	GC_store = XCreateGC(dpy, w, GCForeground | GCBackground, &gcv);

	gcv.background = BgColor;
	gcv.foreground = FgColor;
	gcv.function = GXinvert;
	GC_invert = XCreateGC(dpy, p, GCForeground | GCBackground | GCFunction, &gcv);

	gcv.foreground = CityColor0;
	gcv.background = CityColor0;
	gcv.font = BigFont->fid;
	GC_citycolor0 = XCreateGC(dpy, w, GCForeground | GCBackground | GCFont, &gcv);

	gcv.foreground = CityColor1;
	gcv.background = CityColor1;
	gcv.font = BigFont->fid;
	GC_citycolor1 = XCreateGC(dpy, w, GCForeground | GCBackground | GCFont, &gcv);

	gcv.foreground = CityColor2;
	gcv.background = CityColor2;
	gcv.font = BigFont->fid;
	GC_citycolor2 = XCreateGC(dpy, w, GCForeground | GCBackground | GCFont, &gcv);

	gcv.foreground = TextFgColor;
	gcv.background = TextBgColor;
	gcv.font = BigFont->fid;
	GC_bigf = XCreateGC(dpy, w, GCForeground | GCBackground | GCFont, &gcv);
	gcv.font = SmallFont->fid;
	GC_smallf = XCreateGC(dpy, w, GCForeground | GCBackground | GCFont, &gcv);
}

getColors()
{
	XColor			c;
	XColor			e;
	register Status		s;

	s = XAllocNamedColor(dpy, DefaultColormap(dpy, scr), FgColorName, &c, &e);
	if (s != (Status)1) {
		fprintf(stderr, "%s: warning: can't allocate color `%s'\n",
			ProgName, FgColorName);
		FgColor = BlackPixel(dpy, scr);
	}
	else
		FgColor = c.pixel;

	s = XAllocNamedColor(dpy, DefaultColormap(dpy, scr), BgColorName, &c, &e);
	if (s != (Status)1) {
		fprintf(stderr, "%s: can't allocate color `%s'\n", ProgName, BgColorName);
		BgColor = WhitePixel(dpy, scr);
	}
	else
		BgColor = c.pixel;

	s = XAllocNamedColor(dpy, DefaultColormap(dpy, scr), TextBgColorName, &c, &e);
	if (s != (Status)1) {
		fprintf(stderr, "%s: can't allocate color `%s'\n", ProgName, TextBgColorName);
		TextBgColor = BlackPixel(dpy, scr);
	}
	else
		TextBgColor = c.pixel;

	s = XAllocNamedColor(dpy, DefaultColormap(dpy, scr), TextFgColorName, &c, &e);
	if (s != (Status)1) {
		fprintf(stderr, "%s: can't allocate color `%s'\n", ProgName, TextFgColorName);
		TextFgColor = BlackPixel(dpy, scr);
	}
	else
		TextFgColor = c.pixel;

	s = XAllocNamedColor(dpy, DefaultColormap(dpy, scr), CityColorName0, &c, &e);
	if (s != (Status)1) {
		fprintf(stderr, "%s: can't allocate color `%s'\n", ProgName, CityColorName0);
		CityColor0 = BlackPixel(dpy, scr);
	}
	else
		CityColor0 = c.pixel;

	s = XAllocNamedColor(dpy, DefaultColormap(dpy, scr), CityColorName1, &c, &e);
	if (s != (Status)1) {
		fprintf(stderr, "%s: can't allocate color `%s'\n", ProgName, CityColorName1);
		CityColor1 = BlackPixel(dpy, scr);
	}
	else
		CityColor1 = c.pixel;

	s = XAllocNamedColor(dpy, DefaultColormap(dpy, scr), CityColorName2, &c, &e);
	if (s != (Status)1) {
		fprintf(stderr, "%s: can't allocate color `%s'\n", ProgName, CityColorName2);
		CityColor2 = BlackPixel(dpy, scr);
	}
	else
		CityColor2 = c.pixel;


}

getFonts()
{
	BigFont = XLoadQueryFont(dpy, BIGFONT);
	if (BigFont == (XFontStruct *)NULL) {
		fprintf(stderr, "%s: can't open font `%s', using `%s'\n",
			ProgName, BIGFONT, FAILFONT);
		BigFont = XLoadQueryFont(dpy, FAILFONT);
		if (BigFont == (XFontStruct *)NULL) {
			fprintf(stderr, "%s: can't open font `%s', giving up\n",
				ProgName, FAILFONT);
			exit(1);
		}
	}
	SmallFont = XLoadQueryFont(dpy, SMALLFONT);
	if (SmallFont == (XFontStruct *)NULL) {
		fprintf(stderr, "%s: can't open font `%s', using `%s'\n",
			ProgName, SMALLFONT, FAILFONT);
		SmallFont = XLoadQueryFont(dpy, FAILFONT);
		if (SmallFont == (XFontStruct *)NULL) {
			fprintf(stderr, "%s: can't open font `%s', giving up\n",
				ProgName, FAILFONT);
			exit(1);
		}
	}
}

makePixmaps()
{
	Mappix = XCreatePixmapFromBitmapData(dpy, RootWindow(dpy, scr),
				 large_map_bits, large_map_width,
				 large_map_height, 0, 1, 1);

	Iconpix = XCreatePixmapFromBitmapData(dpy, RootWindow(dpy, scr),
				 icon_map_bits, icon_map_width,
				 icon_map_height, 0, 1, 1);
}

makeClockContexts()
{
	register struct sunclock * s;

	s = makeClockContext(large_map_width, large_map_height, Clock, Mappix,
			     GC_bigf, bigtprint, 0,
			     large_map_height + BigFont->max_bounds.ascent +1 );
	Current = s;

	s = makeClockContext(icon_map_width, icon_map_height, Icon, Iconpix,
			     GC_smallf, smalltprint, 6,
			     icon_map_height + SmallFont->max_bounds.ascent + 1);
	Current->s_next = s;
	s->s_flags |= S_ICON;
	s->s_next = Current;
}

struct sunclock *
makeClockContext(wid, ht, win, pix, gc, fun, txx, txy)
int				wid;
int				ht;
Window				win;
Pixmap				pix;
GC				gc;
char *				(*fun)();
int				txx;
int				txy;
{
	register struct sunclock * s;

	s = (struct sunclock *)salloc(sizeof (struct sunclock));
	s->s_width = wid;
	s->s_height = ht;
	s->s_window = win;
	s->s_pixmap = pix;
	s->s_flags = S_DIRTY;
	s->s_noon = -1;
	s->s_wtab = (short *)salloc((int)(ht * sizeof (short *)));
	s->s_wtab1 = (short *)salloc((int)(ht * sizeof (short *)));
	s->s_increm = 0L;
	s->s_time = 0L;
	s->s_gc	= gc;
	s->s_tfunc = fun;
	s->s_timeout = 0;
	s->s_projtime = -1L;
	s->s_text[0] = '\0';
	s->s_textx = txx;
	s->s_texty = txy;

	return (s);
}	

/*
 * Someone is sure to wonder why the event loop is coded this way, without
 * using select().  The answer is that this was developed on a System V
 * kernel, which has select() but the call has bugs; so, I was inspired
 * to make it portable to systems without select().  The slight delay in
 * expose event processing that results from using sleep(1) rather than
 * alarm() is a fine payoff for not having to worry about interrupted
 * system calls.
 *
 * I've got to use XCheckIfEvent with a degenerate predicate rather than
 * XCheckMaskEvent with a mask of -1L because the latter won't collect all
 * types of events, notably ClientMessage and Selection events.  Sigh.
 */

eventLoop()
{
	XEvent			ev;

	for (;;) {
		if (XCheckIfEvent(dpy, &ev, evpred, (char *)0))
			switch (ev.type) {
		
			case Expose:
				if (ev.xexpose.count == 0)
					doExpose(ev.xexpose.window);
				break;

			/* Set the timezone on a button press */

			case ButtonPress:
 				set_timezone(ev.xbutton.x, ev.xbutton.y);
                                if (Iconic == 1 && Quit < 2) 
				  {
	                            XWMHints		xwmh;
                                    Quit = 0;
                                    Iconic = 2;
                                    xwmh.input = False;
				    xwmh.initial_state = IconicState;
				    xwmh.icon_window = Icon;
				    XSetWMHints(dpy, Clock, &xwmh);
                                    XMapWindow(dpy,Clock);
                                    XUnmapWindow(dpy,Icon);
				  }
                                else
                                if (Iconic == 2 && Quit) 
				  {
                                    Quit = 0 ;
                                    Iconic = 1;
                                    XUnmapWindow(dpy,Clock);
                                    XMapWindow(dpy,Icon);
				  }
                                if (Quit) return;
				break;

			}
		else {
                        if (timedelay <= 1) 
                            ++timedelay;
                        else
			    timedelay = 1000000;
		        usleep(timedelay);
			doTimeout();
		}
	}
}

Bool
evpred(d, e, a)
register Display *		d;
register XEvent *		e;
register char *			a;
{
	return (True);
}

/*
 * Got an expose event for window w.  Do the right thing if it's not
 * currently the one we're displaying.
 */

doExpose(w)
register Window			w;
{
	if (w != Current->s_window) {
		Current = Current->s_next;
		if (w != Current->s_window) {
			fprintf(stderr,
				"%s: expose event for unknown window, id = 0x%08lx\n",
				ProgName, w);
			exit(1);
		}
		setTimeout(Current);
	}
	updimage(Current);
	Current->s_flags |= S_DIRTY;
	showImage(Current);
}

doTimeout()
{
	if (QLength(dpy))
		return;		/* ensure events processed first */
	if (--Current->s_timeout <= 0) {
		updimage(Current);
		showImage(Current);
		setTimeout(Current);
	}
        XFlush(dpy);
}

setTimeout(s)
register struct sunclock *	s;
{
	long			t;

	if (s->s_flags & S_ICON) {
		time(&t);
		s->s_timeout = 60 - localtime(&t)->tm_sec;
	}
	else
		s->s_timeout = 1;
}

showImage(s)
register struct sunclock *	s;
{
	register char *		p;
	struct tm		lt;
	register struct tm *	gmtp;

	lt = *localtime(&s->s_time);
	gmtp = gmtime(&s->s_time);
	p = (*s->s_tfunc)(&lt, gmtp);

	if (s->s_flags & S_DIRTY) {
		XCopyPlane(dpy, s->s_pixmap, s->s_window, GC_store, 0, 0,
			   s->s_width, s->s_height, 0, 0, 1);
		if (s->s_flags & S_ICON)
			XClearArea(dpy, s->s_window, 0, s->s_height + 1,
				   0, 0, False);
		s->s_flags &= ~S_DIRTY;
	}
	strcpy(s->s_text, p);
	showText(s);
}

showText(s)
register struct sunclock *	s;
{
City *c;

	XDrawImageString(dpy, s->s_window, s->s_gc, s->s_textx,
			 s->s_texty, s->s_text, strlen(s->s_text));
	if (size_spot) for (c = cities; c; c = c->next)
	    place_city(c->mode, c->lat, c->lon, c->city);
}

/* --- */
/*  UPDIMAGE  --  Update current displayed image.  */

updimage(s)
register struct sunclock *	s;
{
	register int		i;
	int			xl;
	struct tm *		ct;
	double			jt;
	double			sunra;
	double			sundec;
	double			sunrv;
	double			sunlong;
	double			gt;
	struct tm		lt;
	short *			wtab_swap;

	/* If this is a full repaint of the window, force complete
	   recalculation. */

	if (s->s_noon < 0) {
		s->s_projtime = 0;
		for (i = 0; i < s->s_height; i++) {
			s->s_wtab1[i] = -1;
		}
	}

	if (s->s_flags & S_FAKE) {
		if (s->s_flags & S_ANIMATE)
			s->s_time += s->s_increm;
		if (s->s_time < 0)
			s->s_time = 0;
	} else
		time(&s->s_time);
	lt = *localtime(&s->s_time);

	ct = gmtime(&s->s_time);

	jt = jtime(ct);
	sunpos(jt, False, &sunra, &sundec, &sunrv, &sunlong);
	gt = gmst(jt);

	/* Projecting the illumination curve  for the current seasonal
           instant is costly.  If we're running in real time, only  do
	   it every PROJINT seconds.  */

	if ((s->s_flags & S_FAKE)
	 || s->s_projtime < 0
	 || (s->s_time - s->s_projtime) > PROJINT) {
		projillum(s->s_wtab, s->s_width, s->s_height, sundec);
		wtab_swap = s->s_wtab;
		s->s_wtab = s->s_wtab1;
		s->s_wtab1 = wtab_swap;
		s->s_projtime = s->s_time;
	}

	sunlong = fixangle(180.0 + (sunra - (gt * 15)));
	xl = sunlong * (s->s_width / 360.0);

	/* If the subsolar point has moved at least one pixel, update
	   the illuminated area on the screen.	*/

	if ((s->s_flags & S_FAKE) || s->s_noon != xl) {
		moveterm(s->s_wtab1, xl, s->s_wtab, s->s_noon, s->s_width,
			 s->s_height, s->s_pixmap);
		s->s_noon = xl;
		s->s_flags |= S_DIRTY;
	}
}

/*  PROJILLUM  --  Project illuminated area on the map.  */

projillum(wtab, xdots, ydots, dec)
short *wtab;
int xdots, ydots;
double dec;
{
	int i, ftf = True, ilon, ilat, lilon, lilat, xt;
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
			ftf = False;
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
			while (True) {
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

xspan(pline, leftp, npix, xdots, p)
register int			pline;
register int			leftp;
register int			npix;
register int			xdots;
register Pixmap			p;
{
	leftp = leftp % xdots;

	if (leftp + npix > xdots) {
		XDrawLine(dpy, p, GC_invert, leftp, pline, xdots - 1, pline);
		XDrawLine(dpy, p, GC_invert, 0, pline,
			  (leftp + npix) - (xdots + 1), pline);
	}
	else
		XDrawLine(dpy, p, GC_invert, leftp, pline,
			  leftp + (npix - 1), pline);
}

/*  MOVETERM  --  Update illuminated portion of the globe.  */

moveterm(wtab, noon, otab, onoon, xdots, ydots, pixmap)
short *wtab, *otab;
int noon, onoon, xdots, ydots;
Pixmap pixmap;
{
	int i, ol, oh, nl, nh;

	for (i = 0; i < ydots; i++) {

		/* If line is off in new width table but is set in
		   the old table, clear it. */

		if (wtab[i] < 0) {
			if (otab[i] >= 0) {
				xspan(i, ((onoon - otab[i]) + xdots) % xdots,
				    otab[i] * 2, xdots, pixmap);
			}
		} else {

			/* Line is on in new width table.  If it was off in
			   the old width table, just draw it. */

			if (otab[i] < 0) {
				xspan(i, ((noon - wtab[i]) + xdots) % xdots,
				    wtab[i] * 2, xdots, pixmap);
			} else {

				/* If both the old and new spans were the entire
				   screen, they're equivalent. */

				if (otab[i] == wtab[i] && wtab[i] == (xdots / 2))
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
					xspan(i, ol, (oh - ol) + 1, xdots, pixmap);
					xspan(i, nl, (nh - nl) + 1, xdots, pixmap);
				} else {
					/* Clear portion(s) of old span that extend
					   beyond end of new span. */
					if (ol < nl) {
						xspan(i, ol, nl - ol, xdots, pixmap);
						ol = nl;
					}
					if (oh > nh) {
						xspan(i, nh + 1, oh - nh, xdots, pixmap);
						oh = nh;
					}
					/* Extend existing (possibly trimmed) span to
					   correct new length. */
					if (nl < ol) {
						xspan(i, nl, ol - nl, xdots, pixmap);
					}
					if (nh > oh) {
						xspan(i, oh + 1, nh - oh, xdots, pixmap);
					}
				}
			}
		}
		otab[i] = wtab[i];
	}
}

char *
salloc(nbytes)
register int			nbytes;
{
	register char *		p;

	p = malloc((unsigned)nbytes);
	if (p == (char *)NULL) {
		fprintf(stderr, "%s: out of memory\n", ProgName);
		exit(1);
	}
	return (p);
}

char *
bigtprint(ltp, gmtp)
register struct tm *		ltp;
register struct tm *		gmtp;
{
	static char		s[120];
        static double           dist;
#ifdef NEW_CTIME
	struct timeb		tp;

	if (ftime(&tp) == -1) {
		fprintf(stderr, "%s: ftime failed: ", ProgName);
		perror("");
		exit(1);
	}
#endif
         
        if(cptr1 && cptr2 && repeatcount >= 0)
	   {
           if (repeatcount == 0)
	     {
             dist = sin(dtr(cptr1->lat)) * sin(dtr(cptr2->lat))
                    + cos(dtr(cptr1->lat)) * cos(dtr(cptr2->lat))
                           * cos(dtr(cptr1->lon-cptr2->lon));
             dist = acos(dist);
             sprintf(s, " %s (%.2f %.2f) <--> %s (%.2f %.2f)     "
                        "%d km  =  %d miles                              ", 
               cptr1->city, cptr1->lat, cptr1->lon,
               cptr2->city, cptr2->lat, cptr2->lon, 
               (int)(EARTHRADIUS_KM*dist), (int)(EARTHRADIUS_ML*dist));
	     }
           ++repeatcount;
           if (repeatcount == showdelay+2) 
              {
              repeatcount = -1;
              cptr1->mode = 0;
	      }
	   }
        else
	   sprintf(s,
		" %s %s%02d:%02d:%02d %s %s %02d %s %04d    %02d:%02d:%02d UTC %s %02d %s %04d                ",
                CityName, Location,
		ltp->tm_hour, ltp->tm_min,
		ltp->tm_sec,
#ifdef NEW_CTIME
		ltp->tm_zone,
#else
		tzname[ltp->tm_isdst],
#endif
		Wdayname[ltp->tm_wday], ltp->tm_mday,
		Monname[ltp->tm_mon], 1900 + ltp->tm_year ,
		gmtp->tm_hour, gmtp->tm_min,
		gmtp->tm_sec, Wdayname[gmtp->tm_wday], gmtp->tm_mday,
		Monname[gmtp->tm_mon], 1900 + gmtp->tm_year);

	return (s);
}

char *
smalltprint(ltp, gmtp)
register struct tm *		ltp;
register struct tm *		gmtp;
{
	static char		s[80];

	sprintf(s, "%02d:%02d %s %02d %s %02d", ltp->tm_hour, ltp->tm_min,
		Wdayname[ltp->tm_wday], ltp->tm_mday, Monname[ltp->tm_mon],
		ltp->tm_year % 100);

	return (s);
}

/*
 * readrc() - Read the user's ~/.sunclockrc file.
 */

int readrc()
{
    /*
     * Local Variables
     */

    char *fname;	/* Path to .sunclockrc file */
    FILE *rc;		/* File pointer for rc file */
    char buf[128];	/* Buffer to hold input lines */
    char *city, *lat, *lon, *tz; /* Information about a place */
    City *crec;		/* Pointer to new city record */

    /*
     * External Functions
     */

    char *tildepath();	/* Returns path to ~/<file> */

    /*
     * Get the path to the rc file
     */

    if ((fname = tildepath("~/.sunclockrc")) == NULL) {
        fprintf(stderr, 
             "Unable to get path to ~/.sunclockrc\n");
        return(1);
    }

    /* Open the RC file */

    if ((rc = fopen(fname, "r")) == NULL) {
        fname = app_default;
        if ((rc = fopen(fname, "r")) == NULL)
	  {
          fprintf(stderr, 
             "Unable to find ~/.sunclockrc or app-default Sunclock\n");
	  return(1);
	  }
    }

    /* Read and parse lines from the file */

    while (fgets(buf, 128, rc)) {

	/* Get the city name looking for blank lines and comments */

	if (((city = strtok(buf, ":\n")) == NULL) ||
	    (city[0] == '#') || (city[0] == '\0'))
	    continue;

	/* Get the latitude, longitude and timezone */

	if ((lat = strtok(NULL, " 	\n")) == NULL) {
	    fprintf(stderr, "Error in .sunclockrc for city %s\n", city);
	    continue;
	}

	if ((lon = strtok(NULL, " 	\n")) == NULL) {
	    fprintf(stderr, "Error in .sunclockrf for city %s\n", city);
	    continue;
	}

	if ((tz = strtok(NULL, " 	\n")) == NULL) {
	    fprintf(stderr, "Error in .sunclockrc for city %s\n", city);
	    continue;
	}

	/* Create the record for the city */

	if ((crec = (City *) calloc(1, sizeof(City))) == NULL) {
	    fprintf(stderr, "Memory allocation failure\n");
	    return(1);
	}

	/* Set up the record */

	crec->city = strdup(city);
	crec->lat = atof(lat);
	crec->lon = atof(lon);
	crec->mode = 0;
	crec->tz = strdup(tz);

	/* Link it into the list */

	crec->next = cities;
	cities = crec;
    }

    /* Done */

    return(0);
}

/*
 * place_city() - Put a city on the map.
 */

place_city(mode, lat, lon, name)
int    mode;
double lat, lon;		/* Latitude and longtitude of the city */
char *name;			/* Name of the city */
{
    /*
     * Local Variables
     */

    int ilat, ilon; 		/* Screen coordinates of the city */
    int twidth,theight;         /* Width and height of the text */
    int i;
    GC gc;
    Window w = Clock;

    if (mode < 0) return;
    if (mode == 0) { gc = GC_citycolor0; --mode; }
    if (mode == 1)   gc = GC_citycolor1;
    if (mode == 2)   gc = GC_citycolor2;

    if (mono) { w = Mappix ; gc = GC_invert; }

    ilat = large_map_height - (lat + 90) * (large_map_height / 180.0);
    ilon = (180.0 + lon) * (large_map_width / 360.0);

    if (size_spot == 1)
       XDrawArc(dpy, w, gc, ilon-2, ilat-2, 4, 4, 0, 360 * 64);
    if (size_spot == 2)
       XFillArc(dpy, w, gc, ilon-3, ilat-3, 6, 6, 0, 360 * 64);    
    if (size_spot == 3)
        {
        XDrawArc(dpy, w, gc, ilon-5, ilat-5, 10, 10, 0, 360 * 64);
        XFillArc(dpy, w, gc, ilon-3, ilat-3, 6, 6, 0, 360 * 64);
        }
    if (mono) XFlush(dpy);
}

/*
 * set_timezone() - This is kind of a cheesy way to do it but it works. What happens
 *                  is that when a different city is picked, the TZ environment
 *                  variable is set to the timezone of the city and then tzset().
 *                  is called to reset the system.
 */

set_timezone(x, y)
int x, y;      /* Screen co-ordinates of mouse */
{
    /*
     * Local Variables
     */

    City *cptr;    /* Used to search for a city */
    City *oldcptr; /* Former marked pointer */
    int done;      /* City already found? */
    int cx, cy;    /* Screen coordinates of the city */
    static char buf[64];  /* Used to set the env variable */

    if (y > large_map_height)
      {
        Quit = 1;
        return;
      }

    if (Iconic == 1 && y > icon_map_height)
      {
        Quit = 2;
        return;
      }

    /* Loop through the cities until on close to the pointer is found */
  
    for (cptr = cities; cptr; cptr = cptr->next) {

        /* Convert the latitude and longtitude of the cites to integer */

        cx = (180.0 + cptr->lon) * (large_map_width / 360.0);
	cy = large_map_height - (cptr->lat + 90) * (large_map_height / 180.0);

	/* Check to see if we are close enough */

	if ((((cx - 3) <= x) && ((cx + 3) >= x)) &&
	    (((cy - 3) <= y) && ((cy + 3) >= y))) {

	    /* We are at this city, lets set the timezone */
    
            timedelay = 0;
            sprintf(buf, "TZ=%s", cptr->tz);
	    putenv(buf);
	    tzset();
	    SetIconName();
            CityName = cptr->city;
            if (showcoord) 
              sprintf(Location, "(%.2f %.2f)    ", cptr->lat, cptr->lon);
            else
              strcpy(Location,"   ");
            if (cptr != cptr2)
              {
              if (showdist)
		{
                repeatcount = 0;
                if (cptr1) cptr1->mode = 0;
                cptr1 = cptr2;
                if (cptr1) cptr1->mode = 1;
		}
              else
                if (cptr2) cptr2->mode = 0;
              cptr2 = cptr;
              cptr2->mode = 2;
	      }
            break ;
	}
    }
}

/*
 * SetIconName()
 */

void
SetIconName()
{
  /*
   * Local Variables
   */

    char name[80];/* Used to change icon name */

    sprintf(name, "%s %s", ProgName, VERSION);
    XSetIconName(dpy, Icon, name);
}
