/*****************************************************************************
 *
 * Sun clock version 3.xx by Jean-Pierre Demailly
 *
 * Is derived from the previous versions whose notices appear below.
 * See CHANGES for more explanation on the (quite numerous changes and
 * improvements). Version 3.xx is now published under the GPL.
 */

/*****************************************************************************
 *
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

/*****************************************************************************

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

    modified for interactive maps by

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

    major rewrite of GUI (Jean-Pierre Demailly, demailly@ujf-grenoble.fr)
       3.00  08/26/00  Many bug corrections, GUI improvements (keyboard and
                       mouse controls), new functions, resulting in a much 
                       more powerful program.
       3.04  09/07/00  Final bug fix release of 3.0x
       3.10  10/03/00  Menu window and a lot of new improvements as well.
       3.13  10/26/00  Final bug fix release of 3.1x
       3.20  11/20/00  Drastic GUI and features improvements (color maps,
                       loadable pixmaps,...), now hopefully sunclock looks 
                       nice!
       3.21  12/04/00  Final bug fix release of 3.2x
       3.30  12/04/00  Dockable, multi-window version
*/

#include <unistd.h>
#include <sys/types.h>
#include <sys/timeb.h>
#include <sys/stat.h>
#include <string.h>

#include "sunclock.h"
#include "langdef.h"
#include <X11/xpm.h>

/* 
 *  external routines
 */

extern long	time();
#ifdef NEW_CTIME
extern char *	timezone();
#endif

extern double	jtime();
extern double	gmst();
extern void	sunpos();
extern long	jdate();

extern char *   salloc();
extern void     makePixmap();

extern char **get_dir_list();
extern int dup_strcmp();
extern void free_dirlist();

extern int      fill_mode;

char share_i18n[] = SHAREDIR"/i18n/Sunclock.**";
char app_default[] = APPDEF"/Sunclock";
char *share_maps_dir = SHAREDIR"/earthmaps/";
char image_dir[1024];

char *SmallFont_name = SMALLFONT;
char *BigFont_name = BIGFONT;

char **dirtable;
/* char * freq_filter = ".xpm"; */
char * freq_filter = "";

char language[4] = "";

char *	ProgName;
char *  Command = NULL;
char *  rc_file = NULL;

struct Sundata *Seed, *MenuCaller, *SelCaller;

Color	TextBgColor, TextFgColor, DirColor,
        LandColor, WaterColor, ArcticColor, 
	CityColor0, CityColor1, CityColor2,
	MarkColor1, MarkColor2, LineColor, TropicColor, SunColor;

char *		Display_name = "";

char *          CityInit = NULL;

char *          Map_img_file = NULL;
char *          Clock_img_file = NULL;

char *          Map_img_sel = NULL;
char *          Clock_img_sel = NULL;

Display *	dpy;
int		scr;
Colormap        cmap0;

unsigned long	black;
unsigned long	white;

Atom		wm_delete_window;
Atom		wm_protocols;

XFontStruct *	SmallFont;
XFontStruct *	BigFont;

GC		Store_gc;
GC              Invert_gc;
GC		SmallFont_gc;
GC		BigFont_gc;
GC		DirFont_gc;

Window		Menu = 0;
Window		Selector = 0;

struct Geometry MapGeom =   { 0, 30,  30, 720, 360, 320, 160, 20 };
struct Geometry	ClockGeom = { 0, 30,  30, 128,  64,  48,  24, 20 };
struct Geometry	MenuGeom =  { 0, 30, 430, 700,  40, 700,  40,  0 };
struct Geometry	SelGeom =   { 0, 30,  40, 600, 180, 450,  80,  0 };

int             radius[4] = {0, 2, 3, 5};

Flags           gflags;

int             win_type = 0;
int             spot_size = 2;
int             dotted = 0;
int		placement = -1;
int             mono = 0;
int             invert = 0;
int             color_depth;
int             leave;
int      	chwidth;
int		num_lines;
int             num_table_entries;
int		color_scale = 16;

int             horiz_shift = 0;
int             vert_shift = 12;
int		label_shift = 0;
int		selector_shift = 0;

int             horiz_drift = 0;
int             vert_drift =  0;

int             do_selector = 0;
int             do_hint = 0;
int		do_menu = 0;
int             do_private = 0;

int             time_jump = 0;
int             progress_init = 0;

long            last_time = 0;

double		darkness = 0.5;
double		atm_refraction = ATM_REFRACTION;
double		atm_diffusion = ATM_DIFFUSION;

/* Records to hold extra marks 1 and 2 */

City position, *cities = NULL;

void
usage()
{
     fprintf(stderr, "%s: version %s, %s\nUsage:\n"
     "%s [-help] [-listmenu] [-version] [-language name]\n"
     SP"[-display name] [-bigfont name] [-smallfont name] \n"
     SP"[-mono] [-invert] [-private]\n"
     SP"[-placement (random, fixed, center, NW, NE, SW, SE)]\n"
     SP"[-rcfile file] [-sharedir directory]\n"
     SP"[-mapimage file.xpm(.gz)] [-clockimage file.xpm(.gz)]\n"
     SP"[-clock] [-date] [-seconds] [-command string]\n"
     SP"[-map] [-mapgeom +x+y] [-mapmode * <L,C,S,D,E>] [-decimal] [-dms]\n"
     SP"[-city name] [-position latitude longitude]\n"
     SP"[-jump number[s,m,h,d,M,Y]] [-progress number[s,m,h,d,M,Y]]\n"
     SP"[-nonight] [-night] [-terminator] [-twilight] [-luminosity]\n"
     SP"[-shading mode=0,1,2,3,4] [-diffusion value] [-refraction value]\n"
     SP"[-darkness value<=1.0] [-colorscale number<=64]\n"
     SP"[-menu] [-nomenu] [-horizshift h (map<->menu)] [-vertshift v]\n"
     SP"[-coastlines] [-contour] [-landfill] [-fillmode number=0,1,2]\n"
     SP"[-sun] [-nosun] [-cities] [-nocities] [-meridians] [-nomeridians]\n"
     SP"[-parallels] [-noparallels] [-tropics] [-notropics]\n"
     SP"[-spotsize size(0,1,2,3)] [-dottedlines] [-plainlines]\n"
     SP"[-landcolor color] [-watercolor color] [-arcticcolor color]\n"
     SP"[-citycolor0 color] [-citycolor1 color] [-citycolor2 color]\n"
     SP"[-markcolor1 color] [-markcolor2 color] [-suncolor color]\n"
     SP"[-linecolor color] [-tropiccolor color] [-dircolor color]\n"
     SP"[-textbgcolor color] [-textfgcolor color]\n\n", 
        ProgName, VERSION, COPYRIGHT, ProgName);
}

void
listmenu()
{
        int i;

	fprintf(stderr, "%s\n\n", ShortHelp);
	for (i=0; i<N_OPTIONS; i++)
	fprintf(stderr, "%s %c : %s\n", Label[L_KEY], Option[2*i], Help[i]);
        fprintf(stderr, "\n");
}

void
initValues()
{
        gflags.dirty = 1;
	gflags.force_proj = 0;
	gflags.last_hint = -1;
	gflags.firsttime = 1;
        gflags.bottom = 0;
        gflags.map_mode = LEGALTIME;
        gflags.clock_mode = 0;
	gflags.hours_shown = 0;
	gflags.progress = 0;
        gflags.dms = 0;
        gflags.shading = 1;
        gflags.cities = 1;
        gflags.sun = 1;
        gflags.meridian = 0;
        gflags.parallel = 0;
        gflags.tropics = 0;
        gflags.resized = 0;

        strcpy(image_dir, share_maps_dir);
	strcpy(TextBgColor.name, "Grey92");
	strcpy(TextFgColor.name, "Black");
	strcpy(DirColor.name, "Blue");
	strcpy(LandColor.name, "Chartreuse2");
	strcpy(WaterColor.name, "RoyalBlue");
	strcpy(ArcticColor.name, "LemonChiffon");
	strcpy(CityColor0.name, "Orange");
	strcpy(CityColor1.name, "Red");
	strcpy(CityColor2.name, "Red3");
	strcpy(MarkColor1.name, "Pink1");
	strcpy(MarkColor2.name, "Pink2");
	strcpy(LineColor.name, "White");
	strcpy(TropicColor.name, "White");
	strcpy(SunColor.name, "Yellow");

	position.lat = 100;
	position.tz = NULL;
}

void
needMore(argc, argv)
register int			argc;
register char **		argv;
{
	if (argc == 1) {
		fprintf(stderr, "%s: option `%s' requires an argument\n\n",
			ProgName, *argv);
		usage();
		if (leave) 
                        exit(1);
		else
		   fprintf(stderr, 
                        "Error in config file %s. "RECOVER, *argv);
	}
}

void
getGeom(s, g)
register char *			s;
register struct Geometry *	g;
{
	register int		mask;

	mask = XParseGeometry(s, &g->x, &g->y, &g->width, &g->height);
	if (mask == 0) {
		fprintf(stderr, "%s: `%s' is a bad geometry specification\n",
			ProgName, s);
		exit(1);
	}
	if (g->width<g->w_mini) g->width = g->w_mini;
        if (g->height<g->h_mini) g->height = g->h_mini;
	g->mask = mask;
	if ( placement<=RANDOM && (mask & ( XValue | YValue)) ) 
	   placement = FIXED;
}

void
checkRCfile(argc, argv)
register int			argc;
register char **		argv;
{
	int i;

	for (i=1; i<argc-1; i++) {
           if (strcasecmp(argv[i], "-rcfile") == 0)
	      rc_file = argv[i+1];
           if (strcasecmp(argv[i], "-language") == 0)
	      strncpy(language, argv[i+1], 2);
	}
	   
}

char *
RCAlloc(s)
char * s;
{
       char *t;
       if (leave || !s) return s;
       t = (char *) salloc((strlen(s)+1)*sizeof(char));
       strcpy(t, s);
       return t;
}

int
parseArgs(argc, argv)
register int			argc;
register char **		argv;
{
	int	opt;

	while (--argc > 0) {
		++argv;
		if (strcasecmp(*argv, "-display") == 0) {
			needMore(argc, argv);
			Display_name = RCAlloc(*++argv); 
			--argc;
		} 
		else if (strcasecmp(*argv, "-bigfont") == 0) {
			needMore(argc, argv);
			BigFont_name = RCAlloc(*++argv); 
			--argc;
		} 
		else if (strcasecmp(*argv, "-smallfont") == 0) {
			needMore(argc, argv);
			SmallFont_name = RCAlloc(*++argv); 
			--argc;
		} 
		else if (strcasecmp(*argv, "-language") == 0) {
			needMore(argc, argv);
			strncpy(language, *++argv, 2);
			--argc;
		} 
                else if (strcasecmp(*argv, "-rcfile") == 0) {
			needMore(argc, argv);
			++argv;  /* already done in checkRCfile */
			--argc;
		}
                else if (strcasecmp(*argv, "-sharedir") == 0) {
			needMore(argc, argv);
			share_maps_dir = RCAlloc(*++argv);
			strncpy(image_dir, *argv, 1020);
			--argc;
		}
                else if (strcasecmp(*argv, "-mapimage") == 0) {
			needMore(argc, argv);
			Map_img_file = RCAlloc(*++argv);
			--argc;
		}
                else if (strcasecmp(*argv, "-clockimage") == 0) {
			needMore(argc, argv);
			Clock_img_file = RCAlloc(*++argv);
			--argc;
		}
		else if (strcasecmp(*argv, "-clockgeom") == 0) {
			needMore(argc, argv);
			getGeom(*++argv, &ClockGeom);
			--argc;
		}
		else if (strcasecmp(*argv, "-mapgeom") == 0) {
			needMore(argc, argv);
			getGeom(*++argv, &MapGeom);
			--argc;
		}
		else if (strcasecmp(*argv, "-mapmode") == 0) {
			needMore(argc, argv);
                        if (!strcasecmp(*++argv, "c")) gflags.map_mode = COORDINATES;
                        if (!strcasecmp(*argv, "d")) gflags.map_mode = DISTANCES;
                        if (!strcasecmp(*argv, "e")) gflags.map_mode = EXTENSION;
                        if (!strcasecmp(*argv, "l")) {
			  CityInit = NULL;
			  gflags.map_mode = LEGALTIME;
			}
                        if (!strcasecmp(*argv, "s")) gflags.map_mode = SOLARTIME;
			--argc;
		}
		else if (strcasecmp(*argv, "-spotsize") == 0) {
			needMore(argc, argv);
			spot_size = atoi(*++argv);
                        if (spot_size<0) spot_size = 0;
                        if (spot_size>3) spot_size = 3;
                        --argc;
		}
		else if (strcasecmp(*argv, "-fillmode") == 0) {
			needMore(argc, argv);
			fill_mode = atoi(*++argv);
                        if (fill_mode<0) fill_mode = 0;
                        if (fill_mode>3) fill_mode = 3;
                        --argc;
		}
		else if (strcasecmp(*argv, "-darkness") == 0) {
			needMore(argc, argv);
			darkness = 1.0 - atof(*++argv);
                        if (darkness<0.0) darkness = 0.0;
                        if (darkness>1.0) darkness = 1.0;
                        --argc;
		}
		else if (strcasecmp(*argv, "-diffusion") == 0) {
			needMore(argc, argv);
			atm_diffusion = atof(*++argv);
                        if (atm_diffusion<0.0) atm_diffusion = 0.0;
                        --argc;
		}
		else if (strcasecmp(*argv, "-refraction") == 0) {
			needMore(argc, argv);
			atm_refraction = atof(*++argv);
                        if (atm_refraction<0.0) atm_refraction = 0.0;
                        --argc;
		}
		else if (strcasecmp(*argv, "-colorscale") == 0) {
			needMore(argc, argv);
			color_scale = atoi(*++argv);
                        if (color_scale<=0) color_scale = 1;
                        if (color_scale>COLORSTEPS) color_scale = COLORSTEPS;
                        --argc;
		}
		else if (strcasecmp(*argv, "-landcolor") == 0) {
			needMore(argc, argv);
			strncpy(LandColor.name, *++argv, COLORLENGTH);
			--argc;
		}
		else if (strcasecmp(*argv, "-watercolor") == 0) {
			needMore(argc, argv);
			strncpy(WaterColor.name, *++argv, COLORLENGTH);
			--argc;
		}
		else if (strcasecmp(*argv, "-arcticcolor") == 0) {
			needMore(argc, argv);
			strncpy(ArcticColor.name, *++argv, COLORLENGTH);
			--argc;
		}
 		else if (strcasecmp(*argv, "-textbgcolor") == 0) {
			needMore(argc, argv);
			strncpy(TextBgColor.name, *++argv, COLORLENGTH);
			--argc;
		}
		else if (strcasecmp(*argv, "-textfgcolor") == 0) {
			needMore(argc, argv);
			strncpy(TextFgColor.name, *++argv, COLORLENGTH);
			--argc;
		}
		else if (strcasecmp(*argv, "-dircolor") == 0) {
			needMore(argc, argv);
			strncpy(DirColor.name, *++argv, COLORLENGTH);
			--argc;
		}
		else if (strcasecmp(*argv, "-citycolor0") == 0) {
			needMore(argc, argv);
			strncpy(CityColor0.name, *++argv, COLORLENGTH);
			--argc;
		}
		else if (strcasecmp(*argv, "-citycolor1") == 0) {
			needMore(argc, argv);
			strncpy(CityColor1.name, *++argv, COLORLENGTH);
			--argc;
		}
		else if (strcasecmp(*argv, "-citycolor2") == 0) {
			needMore(argc, argv);
			strncpy(CityColor2.name, *++argv, COLORLENGTH);
			--argc;
		}
		else if (strcasecmp(*argv, "-markcolor1") == 0) {
			needMore(argc, argv);
			strncpy(MarkColor1.name, *++argv, COLORLENGTH);
			--argc;
		}
		else if (strcasecmp(*argv, "-markcolor2") == 0) {
			needMore(argc, argv);
			strncpy(MarkColor2.name, *++argv, COLORLENGTH);
			--argc;
		}
		else if (strcasecmp(*argv, "-linecolor") == 0) {
			needMore(argc, argv);
			strncpy(LineColor.name, *++argv, COLORLENGTH);
			--argc;
		}
		else if (strcasecmp(*argv, "-tropiccolor") == 0) {
			needMore(argc, argv);
			strncpy(TropicColor.name, *++argv, COLORLENGTH);
			--argc;
		}
		else if (strcasecmp(*argv, "-suncolor") == 0) {
			needMore(argc, argv);
			strncpy(SunColor.name, *++argv, COLORLENGTH);
			--argc;
		}
		else if (strcasecmp(*argv, "-position") == 0) {
			needMore(argc, argv);
			position.lat = atof(*++argv);
			if (fabs(position.lat)>90.0) position.lat = 100.0;
	                --argc;
			needMore(argc, argv);
			position.lon = atof(*++argv);
		        position.lon = fixangle(position.lon + 180.0) - 180.0;
			--argc;
		}
		else if (strcasecmp(*argv, "-city") == 0) {
			needMore(argc, argv);
			CityInit = RCAlloc(*++argv);
	                gflags.map_mode = COORDINATES;
			--argc;
		}
		else if (strcasecmp(*argv, "-placement") == 0) {
			needMore(argc, argv);
			if (strcasecmp(*++argv, "random")==0)
                           placement = RANDOM;
			if (strcasecmp(*argv, "fixed")==0) {
                           placement = FIXED;
	                   MapGeom.mask = XValue | YValue | 
                                          WidthValue | HeightValue;
			}
			if (strcasecmp(*argv, "center")==0)
                           placement = CENTER;
			if (strcasecmp(*argv, "nw")==0)
                           placement = NW;
			if (strcasecmp(*argv, "ne")==0)
                           placement = NE;
			if (strcasecmp(*argv, "sw")==0)
                           placement = SW;
			if (strcasecmp(*argv, "se")==0)
                           placement = SE;
			--argc;
		}
		else if (strcasecmp(*argv, "-horizshift") == 0) {
			needMore(argc, argv);
                        horiz_shift = atoi(*++argv);
			--argc;
		}
		else if (strcasecmp(*argv, "-vertshift") == 0) {
			needMore(argc, argv);
                        vert_shift = atoi(*++argv);
			if (vert_shift<6) vert_shift = 6;
			--argc;
		}
		else if (strcasecmp(*argv, "-command") == 0) {
			needMore(argc, argv);
			Command = *++argv;
			--argc;
		}
		else if (strcasecmp(*argv, "-shading") == 0) {
			needMore(argc, argv);
			gflags.shading = atoi(*++argv);
			if (gflags.shading < 0) gflags.shading = 0;
			if (gflags.shading > 4) gflags.shading = 4;
			--argc;
		}
		else if ((opt = (strcasecmp(*argv, "-progress") == 0)) ||
                         (strcasecmp(*argv, "-jump") == 0)) {
		        char *str, c;
			long value;
			needMore(argc, argv);
			value = atol(str=*++argv);
			c = str[strlen(str)-1];
                        if (c>='0' && c<='9') c='s';
			switch(c) {
			  case 's': break;
			  case 'm': value *= 60 ; break;
			  case 'h': value *= 3600 ; break;
			  case 'd': value *= 86400 ; break;
			  case 'M': value *= 2592000 ; break;
			  case 'y':
			  case 'Y': value *= 31536000 ; break;
			  default : c = ' '; break;
			}
			if (c == ' ') usage();
			if (opt)
                           progress_init = abs(value); 
			else 
			   time_jump = value;
			--argc;
		}
		else if (strcasecmp(*argv, "-mono") == 0) {
                        spot_size = 3;
                        mono = 1;
			invert = 1;
			fill_mode = 1;
                }
		else if (strcasecmp(*argv, "-invert") == 0) {
		        invert = 1;
			fill_mode = 1;
		}
		else if (strcasecmp(*argv, "-private") == 0)
		        do_private = 1;
		else if (strcasecmp(*argv, "-clock") == 0)
			win_type = 0;
		else if (strcasecmp(*argv, "-map") == 0)
			win_type = 1;
		else if (strcasecmp(*argv, "-menu") == 0)
			do_menu = 1;
		else if (strcasecmp(*argv, "-nomenu") == 0)
			do_menu = 0;
		else if (strcasecmp(*argv, "-coastlines") == 0)
		        fill_mode = 0;
		else if (strcasecmp(*argv, "-contour") == 0)
		        fill_mode = 1;
		else if (strcasecmp(*argv, "-landfill") == 0)
		        fill_mode = 2;
		else if (strcasecmp(*argv, "-dottedlines") == 0)
		        dotted = 0;
		else if (strcasecmp(*argv, "-plainlines") == 0)
		        dotted = 1;
		else if (strcasecmp(*argv, "-date") == 0)
                        gflags.clock_mode = 0;
		else if (strcasecmp(*argv, "-seconds") == 0)
                        gflags.clock_mode = 1;
		else if (strcasecmp(*argv, "-decimal") == 0)
                        gflags.dms = 0;
		else if (strcasecmp(*argv, "-dms") == 0)
                        gflags.dms = gflags.dms = 1;
		else if (strcasecmp(*argv, "-parallels") == 0) {
			gflags.parallel = 1;
		}
		else if (strcasecmp(*argv, "-cities") == 0) {
			gflags.cities = 1;
		}
		else if (strcasecmp(*argv, "-nonight") == 0) {
			gflags.shading = 0;
		}
		else if (strcasecmp(*argv, "-night") == 0) {
			gflags.shading = 1;
		}
		else if (strcasecmp(*argv, "-terminator") == 0) {
			gflags.shading = 2;
		}
		else if (strcasecmp(*argv, "-twilight") == 0) {
			gflags.shading = 3;
		}
		else if (strcasecmp(*argv, "-luminosity") == 0) {
			gflags.shading = 4;
		}
		else if (strcasecmp(*argv, "-sun") == 0) {
			gflags.sun = 1;
		}
		else if (strcasecmp(*argv, "-nomeridians") == 0) {
			gflags.meridian = 0;
		}
		else if (strcasecmp(*argv, "-notropics") == 0) {
			gflags.tropics = 0;
		}
		else if (strcasecmp(*argv, "-noparallels") == 0) {
			gflags.parallel = 0;
		}
		else if (strcasecmp(*argv, "-nocities") == 0) {
			gflags.cities = 0;
		}
		else if (strcasecmp(*argv, "-nosun") == 0) {
			gflags.sun = 0;
		}
		else if (strcasecmp(*argv, "-meridians") == 0) {
			gflags.meridian = 1;
		}
		else if (strcasecmp(*argv, "-tropics") == 0) {
			gflags.tropics = 1;
		}
		else if (strcasecmp(*argv, "-listmenu") == 0) {
		        listmenu();
			if (leave) 
			  exit(0);
			else
			  return(0);
		}
		else if (strcasecmp(*argv, "-version") == 0) {
			fprintf(stderr, "%s: version %s, %s\n",
				ProgName, VERSION, COPYRIGHT);
			if (leave) 
			  exit(0);
			else
			  return(0);
		} else {
                  fprintf(stderr, "%s: unknown option !!\n\n", *argv);
                  if (leave) {
		    usage();
	            exit(1);
		  }
		  else {
                    fprintf(stderr, RECOVER);
		    return(0);
		  }
		}
	}
	return(0);
}

/*
 * readrc() - Read the user's ~/.sunclockrc file and app-defaults
 */

int 
readrc()
{
    /*
     * Local Variables
     */

    char *fname;	/* Path to .sunclockrc file */
    FILE *rc;		/* File pointer for rc file */
    char buf[128];	/* Buffer to hold input lines */
    char option[3][128]; /* Pointers to options */
    char *args[3];      /* Pointers to options */
    char *city, *lat, *lon, *tz; /* Information about a place */
    City *crec;		/* Pointer to new city record */
    int  first_step=1;  /* Are we parsing options in rc file ? */
    int  i, j;		/* indices */

    /*
     * External Functions
     */

    char *tildepath();	/* Returns path to ~/<file> */

    /*
     * Initialize ListOptions
     */

    for (i=0; i<N_OPTIONS; i++) {
      ListOptions[4*i+1] = Option[2*i];
      ListOptions[4*i] = ListOptions[4*i+2] = ListOptions[4*i+3] = ' ';
    }
    ListOptions[4*N_OPTIONS] = '\0';

    /*
     * Get the path to the rc file
     */

    if (rc_file) fname = rc_file;
        else
    if ((fname = tildepath("~/.sunclockrc")) == NULL) {
        fprintf(stderr, 
             "Unable to get path to ~/.sunclockrc\n");
        return(1);
    }

    
    /* Open the RC file */

    if ((rc = fopen(fname, "r")) == NULL) {
        char *def_name = app_default;
        if ((rc = fopen(def_name, "r")) == NULL) {
          fprintf(stderr, 
             "Unable to find %s or %s\n", fname, def_name);
	  return(1);
	} else {
	  if (fname == rc_file)
          fprintf(stderr, 
             "Unable to find %s, returning to app-default\n   %s\n", 
             fname, def_name);
	  fname = app_default;
	} 
    }

    /* Read and parse lines from the file */

    option[0][0] = '-';
    for (i=0; i<=2; i++) args[i] = option[i];

    while (fgets(buf, 128, rc)) {

        /* Look for blank lines or comments */

        if ((buf[0] == '#') || (buf[0] == '\0')) continue;

        if (strstr(buf, "[Cities]")) {
           first_step = 0;
	   continue;
	}

        if (strstr(buf, "[Options]")) {
	   first_step = 1;
	   continue;
	}

	if (first_step) {
	   i = sscanf(buf, "%s %s %s\n", option[0]+1, option[1], option[2]);
           j = strlen(option[0])-1;
	   if (i==1 || option[0][j]==':') {
           fflush(stdout);
	     if (option[0][j]==':') option[0][j]='\0';
	     leave = 0;
	     i = parseArgs(i+1, args-1);
	   }
	   if (i>0) {
	     fprintf(stderr, 
                     "Error in %s at line %s in [options] !!\n\n", 
                     fname, option[0]+1);
	     usage();
	   }
	   continue;
	}

	/* Get the city name looking for blank lines and comments */

	if (((city = strtok(buf, ":\n")) == NULL))
	    continue;

	/* Get the latitude, longitude and timezone */

	if ((lat = strtok(NULL, " 	\n")) == NULL) {
	    fprintf(stderr, "Error in %s for city %s\n", fname, city);
	    continue;
	}

	if ((lon = strtok(NULL, " 	\n")) == NULL) {
	    fprintf(stderr, "Error in %s for city %s\n", fname, city);
	    continue;
	}

	if ((tz = strtok(NULL, " 	\n")) == NULL) {
	    fprintf(stderr, "Error in %s for city %s\n", fname, city);
	    continue;
	}

	/* Create the record for the city */

	if ((crec = (City *) calloc(1, sizeof(City))) == NULL) {
	    fprintf(stderr, "Memory allocation failure\n");
	    return(1);
	}

	/* Set up the record */

	crec->name = strdup(city);
	crec->lat = atof(lat);
	crec->lon = atof(lon);
	crec->mode = 0;
	crec->tz = strdup(tz);

	/* Link it into the list */

	crec->next = cities;
	cities = crec;
    }

    /*
     * Read language files
     */

    fname = share_i18n;

    i = strlen(share_i18n);
       
    if (!*language && getenv("LANG"))
       strncpy(language, getenv("LANG"), 2);
    if (!*language)
       strcpy (language,"en");

    for (j=0; j<=1; j++) share_i18n[i+j-2] = tolower(language[j]);

    if ((rc = fopen(fname, "r")) != NULL) {
      int j=0, k=0, l=0, m=0, n=0, p;
      while (fgets(buf, 128, rc)) {
        if (buf[0] != '#') {
	        p = strlen(buf)-1;
        	if (p>=0 && buf[p]=='\n') buf[p] = '\0';
	        if (j<7) { strcpy(Day_name[j], buf); j++; } else
        	if (k<12) { strcpy(Month_name[k], buf); k++; } else
		if (l<L_END) { strcpy(Label[l], buf); l++; } else 
		if (m<=N_OPTIONS) { strcpy(Help[m], buf); m++; } else 
                {
                   if (n==0) *ShortHelp = '\0';
		   strcat(ShortHelp, buf); 
                   strcat(ShortHelp, "\n"); 
                   n++;
                }
	}
      }
      fclose(rc);
    } else
        fprintf(stderr, 
             "Unable to open language in %s\n", share_i18n);

    /* Done */

    return(0);
}

int
getPlacement(win, x, y, w, h)
Window win;
int *x, *y;
unsigned int *w, *h;
{
   int xp, yp;
   unsigned int b, d, n;
   Window junk, root, parent, *children;

   XQueryTree(dpy, win, &root, &parent, &children, &n);

   if (!parent) return 1;
	
   XGetGeometry(dpy, parent, &root, x, y, w, h, &b, &d);

   XTranslateCoordinates(dpy, win, parent, 0, 0, x, y, &junk);
   if (2*(*x) < *w) *w -= 2*(*x);
   if ((*x)+(*y) < *h) *h -= (*x) + (*y);
   XTranslateCoordinates(dpy, win, root, 0, 0, x, y, &junk);
   XTranslateCoordinates(dpy, parent, root, 0, 0, &xp, &yp, &junk);

   horiz_drift = *x - xp;
   vert_drift = *y - yp;
   return 0;
}

void
checkGeom(Context, bool)
struct Sundata * Context;
int bool;
{
	int a, b;

        a = DisplayWidth(dpy, scr) - Context->geom.width - horiz_drift - 5;
	b = DisplayHeight(dpy, scr) - Context->geom.height
              -Context->geom.strip - vert_drift - 5;
        if (Context->geom.x > a) Context->geom.x = a;
        if (Context->geom.y > b) Context->geom.y = b;
	if (Context->geom.x < 0) Context->geom.x = 5;
	if (Context->geom.y < 0) Context->geom.y = 5;
        if (bool) Context->geom.mask = 
                      XValue | YValue | WidthValue | HeightValue;
}

void
adjustGeom(Context, which)
struct Sundata * Context;
int which;
{
        int x, y, dx=0, dy=0, diff;
        unsigned int w, h;

        if (getPlacement(Context->win, &x, &y, &w, &h)) return;

	if (which) {
	   if (placement == CENTER) 
              dx = (Context->mapgeom.width - Context->clockgeom.width)/2; else
           if (placement >= NW) {
	      if (placement & 1) 
                 dx = 0; else 
                 dx = Context->mapgeom.width - Context->clockgeom.width;
           }

	   diff= Context->mapgeom.height + Context->mapgeom.strip
                    - Context->clockgeom.height - Context->clockgeom.strip;

           if (placement == CENTER) dy = diff/2; else
           if (placement >= NW) {
	      if (placement <= NE) dy = 0; else dy = diff;
           }
	}

        if (placement) {
	    if (Context->wintype) {
	       dx = -dx;
	       dy = -dy;
	    }
	    if (placement >= CENTER) {
	       Context->geom.x = x + dx - horiz_drift;
	       Context->geom.y = y + dy - vert_drift;
	       checkGeom(Context, 1);
	    }
	}
}

Colormap
newColormap()
{
        if (do_private && DefaultDepth(dpy,scr)<=8)
	    return XCreateColormap(dpy, RootWindow(dpy, scr), 
	                           DefaultVisual(dpy, scr), AllocNone);
	else
            return DefaultColormap(dpy, scr);
}


struct Sundata *
getContext(win)
Window win;
{
   struct Sundata * Context;
   if (win==Menu) 
     return MenuCaller;
   if (win==Selector)
     return SelCaller;

   Context = Seed;
   while (Context && Context->win != win) Context = Context->next;
   return Context;
}

struct Sundata *
parentContext(Context)
struct Sundata * Context;
{
   struct Sundata * ParentContext;

   ParentContext = Seed;
   while (ParentContext && ParentContext->next != Context) 
     ParentContext = ParentContext->next;
   return ParentContext;
}

/*
 * Free resources.
 */

void
shutDown(Context, all)
struct Sundata * Context;
int all;
{
        struct Sundata * ParentContext = NULL;

	if (all<0)
	   Context = Seed;

      repeat:
        ParentContext = Context->next;

        if (Context->pwtab) {
           free(Context->pwtab);
	   Context->pwtab = NULL;
	}
	if (Context->cwtab) {
           free(Context->cwtab);
           Context->cwtab = NULL;
	}
	if (Context->ximdata) {
	   free(Context->ximdata);
	   Context->ximdata = NULL;
	}
	if (Context->xim) {
           XDestroyImage(Context->xim); 
           Context->xim = NULL;
	}
	if (Context->attrib) {
           XpmFreeAttributes(Context->attrib);
	   Context->attrib = NULL;
	}
        if (Context->darkpixel) {
	   free(Context->darkpixel);
	   Context->darkpixel = 0;
        }
	if (Context->pix) {
           XFreePixmap(dpy, Context->pix);
	   Context->pix = 0;
	}

	if (do_private && Context->cmap != cmap0) { 
           XFreeColormap(dpy, Context->cmap);
	   Context->cmap = cmap0;
	}

	XDestroyWindow(dpy, Context->win);
	Context->flags.hours_shown = 0;
	Context->flags.firsttime = 1;

        if (all) {
	   last_time = 0;
           if (Context->clock_img_file != Clock_img_file ) {
              free(Context->clock_img_file);
	   }
           if (Context->clock_img_file != Map_img_file ) {
              free(Context->map_img_file);
	   }
	   if (all<0) {
	      free(Context);
	      Context = ParentContext;
	      if (Context)
	         goto repeat;
	      else {
	        endup:
                 if (invert) {
                    XFreeGC(dpy, Invert_gc);
         	    XFreeGC(dpy, Store_gc);
         	 } 
         	 if (dirtable) free(dirtable);
         	 XFreeFont(dpy, BigFont);
         	 XFreeFont(dpy, SmallFont);
         	 if (!mono) {
         	    XFreeGC(dpy, CityColor0.gc);
         	    XFreeGC(dpy, CityColor1.gc);
         	    XFreeGC(dpy, CityColor2.gc);
         	    XFreeGC(dpy, MarkColor1.gc);
         	    XFreeGC(dpy, MarkColor2.gc);
         	    XFreeGC(dpy, LineColor.gc);
         	    XFreeGC(dpy, TropicColor.gc);
         	    XFreeGC(dpy, SunColor.gc);
                    XFreeGC(dpy, DirFont_gc);
         	 }
         	 XFreeGC(dpy, BigFont_gc);
         	 XFreeGC(dpy, SmallFont_gc);
		 if (do_private) XFreeColormap(dpy, cmap0);
                 XCloseDisplay(dpy);
         	 exit(0);
 	      }
 	   }
           ParentContext = parentContext(Context);
	   if (ParentContext) {
	     ParentContext->next = Context->next;
	   } else
             Seed = Context->next;
	   free(Context);
           if (Seed == NULL) goto endup;
	}
}


/*
 * Set up stuff the window manager will want to know.  Must be done
 * before mapping window, but after creating it.
 */

void
setAllHints(Context, num)
struct Sundata *                Context;
int				num;
{
	XSizeHints		xsh;
	struct Geometry *       Geom = NULL;
	Window			win = 0;

        if (num<=1) {
	    win = Context->win;
	    Geom = &Context->geom;
	    xsh.flags = PSize | PMinSize;
	    if (Geom->mask & (XValue | YValue)) {
		xsh.x = Geom->x;
		xsh.y = Geom->y;
                xsh.flags |= USPosition; 
	    }
	    xsh.width = Geom->width;
	    xsh.height = Geom->height + Geom->strip;
	    if (Context->xim &&
                 ( (Context->wintype==0 && Context->clock_img_file) ||
                   (Context->wintype==1 && Context->map_img_file) ) ) {
                xsh.max_width = xsh.min_width = xsh.width;
                xsh.max_height = xsh.min_height = xsh.height;
                xsh.flags |= PMaxSize;
	    } else {
                xsh.min_width = Geom->w_mini;
                xsh.min_height = Geom->h_mini;
	    }
	} else 
	if (num>=2) {
	    if (num==2) {
	      win = Menu;
	      Geom = &MenuGeom;
	    }
	    if (num==3) {
	      win = Selector;
	      Geom = &SelGeom;
	    }
	    xsh.flags = PSize | USPosition;
	    xsh.x = Geom->x;
	    xsh.y = Geom->y;
	    xsh.width = Geom->width;
	    xsh.height = Geom->height;
	    if (num==3) {
	      xsh.flags |= PMinSize;
	      xsh.min_width = Geom->w_mini;
              xsh.min_height = Geom->h_mini;
	    }
	}

	if (!win) return;
	XSetNormalHints(dpy, win, &xsh);
}

void
setProtocols(Context, num)
struct Sundata * Context;
int				num;
{
	XClassHint		xch;
	Window			win = 0;
	int                     mask;
        char 			name[80];	/* Used to change icon name */
	char *instance[4] =     { "clock", "map", "menu", "selector" };

	mask = ExposureMask | ButtonPressMask | KeyPressMask;

        switch(num) {

	   case 0:
	   case 1:
		win = Context->win;
		mask |= ResizeRedirectMask;
		break;

	   case 2:
		win = Menu;
		mask |= PointerMotionMask;
		break;

	   case 3:
	        win = Selector;
		mask |= ResizeRedirectMask;
		break;
	}

	if (!win) return;

        sprintf(name, "%s %s", ProgName, VERSION);
        XSetIconName(dpy, win, name);
	xch.res_name = ProgName;
	xch.res_class = "Sunclock";
	XSetClassHint(dpy, win, &xch);
        sprintf(name, "%s / %s", ProgName, instance[num]);
	XStoreName(dpy, win, name);
       	XSelectInput(dpy, win, mask);
	XSetWMProtocols(dpy, win, &wm_delete_window, 1);
}

void
fixGeometry(g)
register struct Geometry *		g;
{
	if (g->mask & XNegative)
		g->x = DisplayWidth(dpy, scr) - g->width + g->x;
	if (g->mask & YNegative)
		g->y = DisplayHeight(dpy, scr) - g->height - g->strip + g->y;
}

void
createWindow(Context, num)
struct Sundata * Context;
int num;
{
	XSetWindowAttributes	xswa;
	Window			root = RootWindow(dpy, scr);
	Colormap                cmap = cmap0;
	struct Geometry *	Geom = NULL;
        Window *		win = NULL;
	register int		mask;

	xswa.background_pixel = (mono)? white : TextBgColor.pix;
	xswa.border_pixel = (mono)? black : TextFgColor.pix;
	xswa.backing_store = WhenMapped;

	mask = CWBackPixel | CWBorderPixel | CWBackingStore;

        switch (num) {

	   case 0:
	        win = &Context->win;
		Geom = &Context->geom;
		cmap = Context->cmap;
		break;

	   case 1:
	        win = &Context->win;
		Geom = &Context->geom;
		cmap = Context->cmap;
		break;

	   case 2:
	        if (!Menu) win = &Menu;
		Geom = &MenuGeom;
		break;

	   case 3:
	        if (!Selector) win = &Selector;
		Geom = &SelGeom;
		break;
	}

	if (num<=1) fixGeometry(&Context->geom);

        if (win) {
	     *win = XCreateWindow(dpy, root,
		      Geom->x, Geom->y,
		      Geom->width, Geom->height+Geom->strip, 0,
		      CopyFromParent, InputOutput, 
		      CopyFromParent, mask, &xswa);
	     XSetWindowColormap(dpy, *win, cmap);
	}
}

void
createGCs(Context)
struct Sundata * Context;
{
	XGCValues		gcv;
	Window                  root = RootWindow(dpy, scr);

	if (invert) {
           gcv.background = white;
           gcv.foreground = black;
	   Store_gc = XCreateGC(dpy, Context->win, 
                             GCForeground | GCBackground, &gcv);
           gcv.function = GXinvert;
           Invert_gc = XCreateGC(dpy, Context->pix, 
                             GCForeground | GCBackground | GCFunction, &gcv);
	}

        if (!mono) {
	   gcv.background = TextBgColor.pix;
	   gcv.foreground = TextFgColor.pix;
	}

	gcv.font = SmallFont->fid;
	SmallFont_gc = XCreateGC(dpy, root, GCForeground | GCBackground | GCFont, &gcv);
	gcv.font = BigFont->fid;
	BigFont_gc = XCreateGC(dpy, root, GCForeground | GCBackground | GCFont, &gcv);
        
        if (mono) {
	   DirFont_gc = BigFont_gc;
	   return;
	}

	gcv.foreground = DirColor.pix;
	gcv.font = BigFont->fid;
	DirFont_gc = XCreateGC(dpy, root, GCForeground | GCBackground | GCFont, &gcv);

	gcv.foreground = CityColor0.pix;
	gcv.background = CityColor0.pix;
	CityColor0.gc = XCreateGC(dpy, root, GCForeground | GCBackground, &gcv);

	gcv.foreground = CityColor1.pix;
	gcv.background = CityColor1.pix;
	CityColor1.gc = XCreateGC(dpy, root, GCForeground | GCBackground, &gcv);

	gcv.foreground = CityColor2.pix;
	gcv.background = CityColor2.pix;
	CityColor2.gc = XCreateGC(dpy, root, GCForeground | GCBackground, &gcv);

	gcv.foreground = MarkColor1.pix;
	gcv.background = MarkColor1.pix;
	MarkColor1.gc = XCreateGC(dpy, root, GCForeground | GCBackground, &gcv);

	gcv.foreground = MarkColor2.pix;
	gcv.background = MarkColor2.pix;
	MarkColor2.gc = XCreateGC(dpy, root, GCForeground | GCBackground, &gcv);

	gcv.foreground = LineColor.pix;
	gcv.background = LineColor.pix;
	LineColor.gc = XCreateGC(dpy, root, GCForeground | GCBackground, &gcv);

	gcv.foreground = TropicColor.pix;
	gcv.background = TropicColor.pix;
	TropicColor.gc = XCreateGC(dpy, root, GCForeground | GCBackground, &gcv);

	gcv.foreground = SunColor.pix;
	gcv.background = SunColor.pix;
	gcv.font = BigFont->fid;
	SunColor.gc = XCreateGC(dpy, root, GCForeground | GCBackground | GCFont, &gcv);

}

unsigned long 
getColor(name, other, cmap)
char *           name;
unsigned long    other;
Colormap         cmap;
{

	XColor			c;
	XColor			e;
	register Status		s;

	s = XAllocNamedColor(dpy, cmap, name, &c, &e);
	if (s != (Status)1) {
		fprintf(stderr, "%s: warning: can't allocate color `%s'\n",
			ProgName, name);
		return(other);
	}
	else
	        return(c.pixel);
}

void
getColors(cmap)
Colormap cmap;
{
	black = BlackPixel(dpy, scr);
	white = WhitePixel(dpy, scr);

        if (mono) return;

        LandColor.pix   = getColor(LandColor.name, white, cmap);
        WaterColor.pix  = getColor(WaterColor.name, black, cmap);
        ArcticColor.pix = getColor(ArcticColor.name, black, cmap);

        TextBgColor.pix = getColor(TextBgColor.name, white, cmap);
        TextFgColor.pix = getColor(TextFgColor.name, black, cmap);
        DirColor.pix    = getColor(DirColor.name, black, cmap);
        CityColor0.pix  = getColor(CityColor0.name, black, cmap);
        CityColor1.pix  = getColor(CityColor1.name, black, cmap);
        CityColor2.pix  = getColor(CityColor2.name, black, cmap);
	MarkColor1.pix  = getColor(MarkColor1.name, black, cmap);
	MarkColor2.pix  = getColor(MarkColor2.name, black, cmap);
	LineColor.pix   = getColor(LineColor.name, black, cmap);
	TropicColor.pix = getColor(TropicColor.name, black, cmap);
	SunColor.pix    = getColor(SunColor.name, white, cmap);
}

void
getFonts()
{
	BigFont = XLoadQueryFont(dpy, BigFont_name);
	if (BigFont == (XFontStruct *)NULL) {
		fprintf(stderr, "%s: can't open font `%s', using `%s'\n",
			ProgName, BigFont_name, FAILFONT);
		BigFont = XLoadQueryFont(dpy, FAILFONT);
		if (BigFont == (XFontStruct *)NULL) {
		   fprintf(stderr, "%s: can't open font `%s', giving up\n",
				ProgName, FAILFONT);
		   exit(1);
		}
	}
	SmallFont = XLoadQueryFont(dpy, SmallFont_name);
	if (SmallFont == (XFontStruct *)NULL) {
		fprintf(stderr, "%s: can't open font `%s', using `%s'\n",
			ProgName, SmallFont_name, FAILFONT);
		SmallFont = XLoadQueryFont(dpy, FAILFONT);
		if (SmallFont == (XFontStruct *)NULL) {
			fprintf(stderr, "%s: can't open font `%s', giving up\n",
				ProgName, FAILFONT);
			exit(1);
		}
	}

        MapGeom.strip = BigFont->max_bounds.ascent + 
                        BigFont->max_bounds.descent + 6;

        chwidth = MapGeom.strip + 5;

        ClockGeom.strip = SmallFont->max_bounds.ascent +
	                  SmallFont->max_bounds.descent + 4;

        MenuGeom.width = MENU_WIDTH * MapGeom.strip;
        MenuGeom.height = 2 * MapGeom.strip;

        SelGeom.width = SEL_WIDTH * MapGeom.strip;
        SelGeom.height = (11 + 4*SEL_HEIGHT)*MapGeom.strip/5;
}

void
clearStrip(Context)
struct Sundata * Context;
{
        XClearArea(dpy, Context->win, 0, Context->geom.height, 
                 Context->geom.width, Context->geom.strip, True);

	Context->count = PRECOUNT;
	if (Context->flags.bottom) --Context->flags.bottom;
}

void
clearSelector()
{
	XClearArea(dpy, Selector, 0, MapGeom.strip+1, 
                SelGeom.width-2, SelGeom.height-MapGeom.strip-2, True);
}

void
exposeMap(Context)
struct Sundata * Context;
{
        XClearArea(dpy, Context->win, Context->geom.width-1, Context->geom.height-1, 1, 1, True);
}

void
updateMap(Context)
struct Sundata * Context;
{
	Context->count = PRECOUNT;
        exposeMap(Context);
}

/*
 *  Set the timezone of selected location
 */

void
setTZ(cptr)
City	*cptr;
{
#ifndef linux
	char buf[80];
#endif

   	if (cptr && cptr->tz) {
#ifdef linux
           setenv("TZ", cptr->tz, 1);
#else
	   sprintf(buf, "TZ=%s", cptr->tz);
 	   putenv(buf);
#endif
	   } 
	else
#ifdef linux
           unsetenv("TZ");
#else
	   {
	   /* This is supposed to reset timezone to default localzone */
	   strcpy(buf, "TZ");
	   /* Another option would be to set LOCALZONE adequately and put:
           strcpy(buf, "TZ="LOCALZONE); */
	   putenv(buf);
	   }
#endif
	tzset();
}

/*
 *   Sets sun position (longitude, declination) and returns
 *   solartime at given city
 *   CAUTION: sunlong is in fact given as longitude+180 in interval 0..360
 */

time_t
sunParams(gtime, sunlong, sundec, city)
time_t gtime;
double *sunlong;
double *sundec;
City *city;
{
	struct tm		ctp, stp;
	time_t			stime;
	double                  jt, gt;
	double			sunra, sunrv;
	long                    diff;

	ctp = *gmtime(&gtime);
	jt = jtime(&ctp);
	sunpos(jt, False, &sunra, sundec, &sunrv, sunlong);
	gt = gmst(jt);
        *sunlong = fixangle(180.0 + (sunra - (gt * 15))); 
	
	if (city) {
           stime = (long) ((city->lon - *sunlong)*240.0);
           stp = *gmtime(&stime);
           diff = stp.tm_sec-ctp.tm_sec
                  +60*(stp.tm_min-ctp.tm_min)+3600*(stp.tm_hour-ctp.tm_hour);
	   if (city->lon>0.0) while(diff<-40000) diff += 86400;
	   if (city->lon<0.0) while(diff>40000) diff -= 86400;
	   stime = gtime+diff;
	} else
	   stime = 0;
	return(stime);
}

/* get "daylength" at a location of latitude lat, when it is noon at
   GMT time gt at that location */

double
dayLength(gtime, lat)
time_t  gtime;
double	lat;
{
	double			duration;
	double			sundec, junk;
	double                  sinsun, sinpos, sinapp, num;

        sinpos = sin(dtr(lat));

        /* Get Sun declination */
        (void) sunParams(gtime, &junk, &sundec, NULL);
        sinsun = sin(dtr(sundec));

        /* Correct for the sun apparent diameter and atmospheric diffusion */
        sinapp = sin(dtr(SUN_APPRADIUS + atm_refraction));

        num = 1 - sinsun*sinsun - sinpos*sinpos - sinapp*sinapp
                - 2*sinsun*sinpos*sinapp;
        if (num<=0) {
           if (sinsun*sinpos>0) 
	     duration = 24.0;
	   else
	     duration = 0.0;
        } else
             duration = 12.0 + 24.0*atan((sinsun*sinpos+sinapp)/sqrt(num))/PI;

        return duration*3600;
}

void
setDayParams(Context)
struct Sundata * Context;
{
        struct tm               gtm, ltm;
	double                  duration, junk;
	time_t			gtime, stime, sr, ss, dl;

        /* Context->count = PRECOUNT;
	   Context->flags.force_proj = 1; */
	Context->flags.last_hint = -1;

	if (!Context->mark1.city) return;

  	gtime = Context->time + Context->jump;

        /* Get local time at given location */
	setTZ(Context->mark1.city);
        ltm = *localtime(&gtime);

        /* Get solar time at given location */
        stime = sunParams(gtime, &junk, &junk, Context->mark1.city);

	/* Go to time when it is noon in solartime at Context->mark1.city */
        gtime += 43200 - (stime % 86400);
        gtm = *gmtime(&gtime);

        if (gtm.tm_mday < ltm.tm_mday) gtime +=86400;
        if (gtm.tm_mday > ltm.tm_mday) gtime -=86400;
       
        /* Iterate, just in case of a day shift */
        stime = sunParams(gtime, &junk, &junk, Context->mark1.city);

        gtime += 43200 - (stime % 86400);

        /* get day length at that time and location*/
        duration = dayLength(gtime, Context->mark1.city->lat);
  
	/* compute sunrise and sunset in legal time */
	sr = gtime - (time_t)(0.5*duration);
	ss = gtime + (time_t)(0.5*duration);
        dl = ss-sr;
        Context->mark1.full = 1;
	if (dl<=0) {dl = 0; Context->mark1.full = 0;}
	if (dl>86380) {dl=86400; Context->mark1.full = 0;}

	Context->mark1.dl = *gmtime(&dl);
	Context->mark1.dl.tm_hour += (Context->mark1.dl.tm_mday-1) * 24;

	setTZ(Context->mark1.city);
	Context->mark1.sr = *localtime(&sr);
	Context->mark1.ss = *localtime(&ss);
}

char *
num2str(value, string, dms)
double value;
char *string;
short dms;
{
	int eps, d, m, s;

	if (dms) {
 	  if (value<0) {
	    value = -value; 
	    eps = -1;
	  } else
	    eps = 1;
	  d = (int) value;
	  value = 60 * (value - d);
	  m = (int) value;
	  value = 60 * (value - m);	  
	  s = (int) value;
	  sprintf(string, "%d°%02d'%02d\"", eps*d, m, s);
	} else
	  sprintf(string, "%.3f", value); 
	return string;
}

/*
 *  Produce bottom strip of hours
 */

void
show_hours(Context)
struct Sundata * Context;
{
	int i;
	char s[128];

	if (Context->flags.hours_shown) return;
        XClearArea(dpy, Context->win, 0, Context->geom.height, 
                 Context->geom.width, MapGeom.strip, True);
	for (i=0; i<24; i++) {
	    sprintf(s, "%d", i); 
   	    XDrawImageString(dpy, Context->win, BigFont_gc, 
              ((i*Context->geom.width)/24 + 2*Context->geom.width - chwidth*strlen(s)/8 +
               (int)(Context->sunlon*Context->geom.width/360.0))%Context->geom.width +1,
              BigFont->max_bounds.ascent + Context->geom.height + 3, s, strlen(s));
	}
        Context->flags.hours_shown = 1;
}

void
writeStrip(Context, gtime)
struct Sundata * Context;
time_t gtime;
{
	register struct tm 	ltp;
	register struct tm 	gtp;
	register struct tm 	stp;
	time_t                  stime;
 	int			i, l;
	char		s[128];
	char		slat[20], slon[20], slatp[20], slonp[20];
        double		dist;
#ifdef NEW_CTIME
	struct timeb		tp;

	if (ftime(&tp) == -1) {
		fprintf(stderr, "%s: ftime failed: ", ProgName);
		perror("");
		exit(1);
	}
#endif

	if (!Context->wintype) {
		setTZ(NULL);
		ltp = *localtime(&gtime);

		if (Context->flags.clock_mode)
		   sprintf(s, "%02d:%02d:%02d %s", 
                	ltp.tm_hour, ltp.tm_min, ltp.tm_sec,
#ifdef NEW_CTIME
			ltp.tm_zone);
#else
			tzname[ltp.tm_isdst]);
#endif
        	else
		   sprintf(s, "%02d:%02d %s %02d %s %02d", ltp.tm_hour, ltp.tm_min,
			Day_name[ltp.tm_wday], ltp.tm_mday, Month_name[ltp.tm_mon],
			ltp.tm_year % 100);

	        l = strlen(s);
		if (l<72) {
		  for (i=l; i<72; i++) s[i] = ' ';
		  s[72] = '\0';
		  l = 72;
		}

                goto draw;
        }

        switch(Context->flags.map_mode) {

	case LEGALTIME:
           gtp = *gmtime(&gtime);
           setTZ(Context->mark1.city);
	   ltp = *localtime(&gtime);
	   sprintf(s,
		" %s %02d:%02d:%02d %s %s %02d %s %04d    %s %02d:%02d:%02d UTC %s %02d %s %04d",
                Label[L_LEGALTIME], ltp.tm_hour, ltp.tm_min,
		ltp.tm_sec,
#ifdef NEW_CTIME
		ltp.tm_zone,
#else
		tzname[ltp.tm_isdst],
#endif
		Day_name[ltp.tm_wday], ltp.tm_mday,
		Month_name[ltp.tm_mon], 1900 + ltp.tm_year ,
                Label[L_GMTTIME],
		gtp.tm_hour, gtp.tm_min,
		gtp.tm_sec, Day_name[gtp.tm_wday], gtp.tm_mday,
		Month_name[gtp.tm_mon], 1900 + gtp.tm_year);
	   break;

	case COORDINATES:
           setTZ(Context->mark1.city);
	   ltp = *localtime(&gtime);
	   if (ltp.tm_mday != Context->local_day) 
              setDayParams(Context);
	   Context->local_day = ltp.tm_mday;
           if ((Context->mark1.city) && Context->mark1.full)
	   sprintf(s,
		" %s (%s,%s)  %02d:%02d:%02d %s %s %02d %s %04d   %s %02d:%02d:%02d   %s %02d:%02d:%02d",
                Context->mark1.city->name, 
                num2str(Context->mark1.city->lat, slat, Context->flags.dms), 
                num2str(Context->mark1.city->lon, slon, Context->flags.dms),
		ltp.tm_hour, ltp.tm_min, ltp.tm_sec,
#ifdef NEW_CTIME
		ltp.tm_zone,
#else
		tzname[ltp.tm_isdst],
#endif
		Day_name[ltp.tm_wday], ltp.tm_mday,
		Month_name[ltp.tm_mon], 1900 + ltp.tm_year ,
                Label[L_SUNRISE],
		Context->mark1.sr.tm_hour, Context->mark1.sr.tm_min, Context->mark1.sr.tm_sec,
                Label[L_SUNSET], 
		Context->mark1.ss.tm_hour, Context->mark1.ss.tm_min, Context->mark1.ss.tm_sec);
	        else
           if ((Context->mark1.city) && !Context->mark1.full)
	   sprintf(s,
		" %s (%s,%s)  %02d:%02d:%02d %s %s %02d %s %04d   %s %02d:%02d:%02d",
                Context->mark1.city->name, 
                num2str(Context->mark1.city->lat, slat, Context->flags.dms), 
                num2str(Context->mark1.city->lon, slon, Context->flags.dms),
		ltp.tm_hour, ltp.tm_min, ltp.tm_sec,
#ifdef NEW_CTIME
		ltp.tm_zone,
#else
		tzname[ltp.tm_isdst],
#endif
		Day_name[ltp.tm_wday], ltp.tm_mday,
		Month_name[ltp.tm_mon], 1900 + ltp.tm_year ,
                Label[L_DAYLENGTH],
		Context->mark1.dl.tm_hour, Context->mark1.dl.tm_min, Context->mark1.dl.tm_sec);
	        else
                  sprintf(s," %s", Label[L_CLICKCITY]);
	        break;

	case SOLARTIME:
	   if (Context->mark1.city) {
	     double junk;
	     stime = sunParams(gtime, &junk, &junk, Context->mark1.city);
	     stp = *gmtime(&stime);
	     if (stp.tm_mday != Context->solar_day) 
                setDayParams(Context);
	     Context->solar_day = stp.tm_mday;
	     sprintf(s, " %s (%s,%s)  %s %02d:%02d:%02d   %s %02d %s %04d   %s %02d:%02d:%02d", 
                  Context->mark1.city->name, 
                  num2str(Context->mark1.city->lat,slat, Context->flags.dms), 
		  num2str(Context->mark1.city->lon,slon, Context->flags.dms),
                  Label[L_SOLARTIME],
                  stp.tm_hour, stp.tm_min, stp.tm_sec,
                  Day_name[stp.tm_wday], stp.tm_mday,
		  Month_name[stp.tm_mon], 1900 + stp.tm_year,
                  Label[L_DAYLENGTH], 
 		  Context->mark1.dl.tm_hour, Context->mark1.dl.tm_min, Context->mark1.dl.tm_sec);
           } else
                  sprintf(s," %s", Label[L_CLICKLOC]);
	   break;

	case DISTANCES:
	   if(Context->mark1.city && Context->mark2.city) {
             dist = sin(dtr(Context->mark1.city->lat)) * sin(dtr(Context->mark2.city->lat))
                    + cos(dtr(Context->mark1.city->lat)) * cos(dtr(Context->mark2.city->lat))
                           * cos(dtr(Context->mark1.city->lon-Context->mark2.city->lon));
	     if (dist >= 1.0) 
                dist = 0.0;
	     else
	     if (dist <= -1.0) 
	        dist = M_PI;
	     else
                dist = acos(dist);
             sprintf(s, " %s (%s,%s) --> %s (%s,%s)     "
                      "%d km  =  %d miles", 
               Context->mark2.city->name, 
               num2str(Context->mark2.city->lat,slatp, Context->flags.dms), 
	       num2str(Context->mark2.city->lon, slonp, Context->flags.dms),
               Context->mark1.city->name, 
               num2str(Context->mark1.city->lat, slat, Context->flags.dms), 
               num2str(Context->mark1.city->lon, slon, Context->flags.dms),
               (int)(EARTHRADIUS_KM*dist), (int)(EARTHRADIUS_ML*dist));
	   } else
	     sprintf(s, " %s", Label[L_CLICK2LOC]);
	   break;

	case EXTENSION:
	   show_hours(Context);
           return;

	default:
	   break;
	}

        l = strlen(s);
	if (l<125) {
	  for (i=l; i<125; i++) s[i] = ' ';
	  s[125] = '\0';
	  l = 125;
	}

      draw:
	XDrawImageString(dpy, Context->win, 
             (Context->wintype)? BigFont_gc : SmallFont_gc, 
             Context->textx, Context->texty, s, l-label_shift);
}

void
fixTextPosition(Context)
struct Sundata * Context;
{
	Context->textx = 4;
        if (Context->wintype)
           Context->texty = Context->geom.height +
                            BigFont->max_bounds.ascent + 3;
        else
           Context->texty = Context->geom.height +
 	                    SmallFont->max_bounds.ascent + 2;
}

void
makeContext(Context, build)
struct Sundata * Context;
int build;
{
        if (build) {
	   Context->cmap = cmap0;
	   Context->mapgeom = MapGeom;
	   Context->clockgeom = ClockGeom;
           if (Context->wintype)
              Context->geom = MapGeom;
           else
              Context->geom = ClockGeom;
           Context->flags = gflags;
           Context->jump = time_jump;
           Context->progress = (progress_init)? progress_init : 60;
           Context->clock_img_file = Clock_img_file;
           Context->map_img_file = Map_img_file;
           Context->next = NULL;
	   Context->mark1.city = NULL;
	   Context->mark1.status = 0;
           Context->pos1.tz = NULL;
	   Context->mark2.city = NULL;
	   Context->mark2.status = 0;
           Context->pos2.tz = NULL;
           if (position.lat<=90.0) {
              Context->pos1 = position;
              Context->mark1.city = &Context->pos1;
           }
        }

        fixTextPosition(Context);
        Context->local_day = -1;
        Context->solar_day = -1;
        Context->xim = NULL;
        Context->ximdata = NULL;
        Context->attrib = NULL;
        Context->darkpixel = NULL;
        Context->pix = 0;
        Context->bits = 0;
	Context->noon = -1;
	Context->pwtab = (short *)salloc((int)(Context->geom.height * sizeof (short *)));
	Context->cwtab = (short *)salloc((int)(Context->geom.height * sizeof (short *)));
	Context->time = 0L;
	Context->timeout = 0;
	Context->projtime = -1L;

}	

/*
 *  Select Window and GC for monochrome mode
 */ 

void
checkMono(Context, pw, pgc)
struct Sundata * Context;
Window *pw;
GC     *pgc;
{
	if (mono) { 
           *pw = Context->pix ; 
           *pgc = Invert_gc; 
        } else
	   *pw = Context->win;
}

/*
 * placeSpot() - Put a spot (city or mark) on the map.
 */

void
placeSpot(Context, mode, lat, lon)
struct Sundata * Context;
int    mode;
double lat, lon;		/* Latitude and longtitude of the city */
{
    /*
     * Local Variables
     */

    int ilat, ilon; 		/* Screen coordinates of the city */
    int rad;
    GC gc = CityColor0.gc;
    Window w;

    if (mode < 0) return;
    if (!mono && !Context->flags.cities && mode <= 2) return;

    if (mode == 0) { gc = CityColor0.gc; --mode; }
    if (mode == 1)   gc = CityColor1.gc;
    if (mode == 2)   gc = CityColor2.gc;
    if (mode == 3)   gc = MarkColor1.gc;
    if (mode == 4)   gc = MarkColor2.gc;
    if (mode == 5)   gc = SunColor.gc;

    checkMono(Context, &w, &gc);

    ilat = Context->geom.height - (lat + 90) * (Context->geom.height / 180.0);
    ilon = (180.0 + lon) * (Context->geom.width / 360.0);

    rad = radius[spot_size];

    if (mode == 5)
       {
       int rad=6, diag=4;
       XFillArc(dpy, w, gc, ilon-rad/2, ilat-rad/2, rad, rad, 0, 360 * 64);
       XDrawLine(dpy, w, gc, ilon, ilat-rad, ilon, ilat+rad);
       XDrawLine(dpy, w, gc, ilon-rad, ilat, ilon+rad, ilat);
       XDrawLine(dpy, w, gc, ilon-diag, ilat-diag, ilon+diag, ilat+diag);
       XDrawLine(dpy, w, gc, ilon-diag, ilat+diag, ilon+diag, ilat-diag);
       return;
       }

    if (spot_size == 1)
       XDrawArc(dpy, w, gc, ilon-rad, ilat-rad, 2*rad, 2*rad, 0, 360 * 64);
    if (spot_size == 2)
       XFillArc(dpy, w, gc, ilon-rad, ilat-rad, 2*rad, 2*rad, 0, 360 * 64);  
    if (spot_size == 3)
       {
       XDrawArc(dpy, w, gc, ilon-rad, ilat-rad, 2*rad, 2*rad, 0, 360 * 64);
       XFillArc(dpy, w, gc, ilon-3, ilat-3, 6, 6, 0, 360 * 64);
       }
}

void
drawCities(Context)
struct Sundata * Context;
{
City *c;
        if (!Context->wintype) return; 
        for (c = cities; c; c = c->next)
	    placeSpot(Context, c->mode, c->lat, c->lon);
}

void
drawMarks(Context)
struct Sundata * Context;
{
        if (mono || !Context->wintype) return; 

        /* code for color mode */
        if (Context->mark1.city == &Context->pos1)
	  placeSpot(Context, 3, Context->mark1.city->lat, Context->mark1.city->lon);
        if (Context->mark2.city == &Context->pos2)
	  placeSpot(Context, 4, Context->mark2.city->lat, Context->mark2.city->lon);
}

/*
 * draw_parallel() - Draw a parallel line
 */

void
draw_parallel(Context, gc, lat, step, thickness)
struct Sundata * Context;
GC gc;
double lat;
int step;
int thickness;
{
        Window w = Context->win;
	int ilat, i0, i1, i;

        if (!Context->wintype) return; 

        checkMono(Context, &w, &gc);

	ilat = Context->geom.height - (lat + 90) * (Context->geom.height / 180.0);
	i0 = Context->geom.width/2;
	i1 = 1+i0/step;
	for (i=-i1; i<i1; i+=1)
  	  XDrawLine(dpy, w, gc, i0+step*i-thickness, ilat, 
                                i0+step*i+thickness, ilat);
}

void
draw_parallels(Context)
struct Sundata * Context;
{
	int     i;

        for (i=-8; i<=8; i++) if (i!=0 || !Context->flags.tropics)
	  draw_parallel(Context, LineColor.gc, i*10.0, 3, dotted);
}

void
draw_tropics(Context)
struct Sundata * Context;
{
	static  double val[5] = { -66.55, -23.45, 0.0, 23.45, 66.55 };
	int     i;

        if (mono && Context->flags.parallel)
	  draw_parallel(Context, LineColor.gc, 0.0, 3, dotted);

        for (i=0; i<5; i++)
	  draw_parallel(Context, TropicColor.gc, val[i], 3, 1);
}	


/*
 * draw_meridian() - Draw a meridian line
 */

void
draw_meridian(Context, lon, step,thickness)
struct Sundata * Context;
double lon;
int step;
int thickness;
{
        Window w = Context->win;
        GC gc = LineColor.gc;
	int ilon, i0, i1, i;

        if (!Context->wintype) return; 
	
        checkMono(Context, &w, &gc);

	ilon = (180.0 + lon) * (Context->geom.width / 360.0);
	i0 = Context->geom.height/2;
	i1 = 1+i0/step;
	for (i=-i1; i<i1; i+=1)
	  XDrawLine(dpy, w, gc, ilon, i0+step*i-thickness, 
                                ilon, i0+step*i+thickness);
}

void
draw_meridians(Context)
struct Sundata * Context;
{
	int     i;

        for (i=-11; i<=11; i++)
	  draw_meridian(Context, i*15.0, 3, dotted);
}

/*
 * draw_sun() - Draw Sun at position where it stands at zenith
 */

void
draw_sun(Context)
struct Sundata * Context;
{
	placeSpot(Context, 5, Context->sundec, Context->sunlon);
}

void
drawSun(Context)
struct Sundata * Context;
{
	if (Context->flags.sun)
  	  draw_sun(Context);
}

void
drawBottomline(Context)
struct Sundata * Context;
{
        Window w = Context->win;
	GC gc = BigFont_gc;

        checkMono(Context, &w, &gc);
        XDrawLine(dpy, w, gc, 0, Context->geom.height-1, 
	           Context->geom.width-1, Context->geom.height-1);
}

void
drawLines(Context)
struct Sundata * Context;
{
	if (Context->flags.meridian)
	  draw_meridians(Context);
	if (Context->flags.parallel)
	  draw_parallels(Context);
	if (Context->flags.tropics)
	  draw_tropics(Context);
	if (!mono)
	  drawBottomline(Context);
}

void
drawAll(Context)
struct Sundata * Context;
{
        drawLines(Context);
        drawCities(Context);
	drawMarks(Context);
}

void
pulseMarks(Context)
struct Sundata * Context;
{
int     done = 0;
        
        if (!Context->wintype) return; 
	if (Context->mark1.city && Context->mark1.status<0) {
           if (Context->mark1.pulse) {
	     placeSpot(Context, 0, Context->mark1.save_lat, Context->mark1.save_lon);
	     done = 1;
	   }
	   Context->mark1.save_lat = Context->mark1.city->lat;
	   Context->mark1.save_lon = Context->mark1.city->lon;
           if (Context->mark1.city == &Context->pos1) {
	      done = 1;
              placeSpot(Context, 0, Context->mark1.save_lat, Context->mark1.save_lon);
	      Context->mark1.pulse = 1;
	   } else
	      Context->mark1.pulse = 0;
	   Context->mark1.status = 1;
	}
	else
        if (Context->mark1.status>0) {
	   if (Context->mark1.city|| Context->mark1.pulse) {
              placeSpot(Context, 0, Context->mark1.save_lat, Context->mark1.save_lon);
	      Context->mark1.pulse = 1-Context->mark1.pulse;
	      done = 1;
	   }
	   if (Context->mark1.city == NULL) Context->mark1.status = 0;
	}

	if (Context->mark2.city && Context->mark2.status<0) {
           if (Context->mark2.pulse) {
	     placeSpot(Context, 0, Context->mark2.save_lat, Context->mark2.save_lon);
	     done = 1;
	   }
	   Context->mark2.save_lat = Context->mark2.city->lat;
	   Context->mark2.save_lon = Context->mark2.city->lon;
           if (Context->mark2.city == &Context->pos2) {
              placeSpot(Context, 0, Context->mark2.save_lat, Context->mark2.save_lon);
	      done = 1;
	      Context->mark2.pulse = 1;
	   } else
	      Context->mark2.pulse = 0;
	   Context->mark2.status = 1;
	}
	else
        if (Context->mark2.status>0) {
	   if (Context->mark2.city || Context->mark2.pulse) {
              placeSpot(Context, 0, Context->mark2.save_lat, Context->mark2.save_lon);
              Context->mark2.pulse = 1 - Context->mark2.pulse;
	      done = 1;
	   }
	   if (Context->mark2.city == NULL) Context->mark2.status = 0;
	}
	if (done) exposeMap(Context);
}

/*  PROJILLUM  --  Project illuminated area on the map.  */

void
projillum(Context)
struct Sundata * Context;
{
	int i, ftf = True, ilon, ilat, lilon = 0, lilat = 0, xt;
	double m, x, y, z, th, lon, lat, s, c;

	/* Clear unoccupied cells in width table */

	
	if (Context->flags.shading)
   	   for (i = 0; i < Context->geom.height; i++)
		Context->pwtab[i] = -1;
	else {
   	   for (i = 0; i < Context->geom.height; i++)
		Context->pwtab[i] = Context->geom.width;
	   return;
	}

	/* Build transformation for declination */

	s = sin(-dtr(Context->sundec));
	c = cos(-dtr(Context->sundec));

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

		ilat = Context->geom.height 
                       - (lat + 90.0) * (Context->geom.height / 180.0);
		ilon = lon * (Context->geom.width / 180.0);

		if (ftf) {

			/* First time.  Just save start co-ordinate. */

			lilon = ilon;
			lilat = ilat;
			ftf = False;
		} else {

			/* Trace out the line and set the width table. */

			if (lilat == ilat) {
				Context->pwtab[(Context->geom.height - 1) - ilat] = (ilon == 0)? 1 : ilon;
			} else {
				m = ((double) (ilon - lilon)) / (ilat - lilat);
				for (i = lilat; i != ilat; i += sgn(ilat - lilat)) {
					xt = lilon + floor((m * (i - lilat)) + 0.5);
					Context->pwtab[(Context->geom.height - 1) - i] = (xt == 0)? 1 : xt;
				}
			}
			lilon = ilon;
			lilat = ilat;
		}
	}

	/* Now tweak the widths to generate full illumination for
	   the correct pole. */

	if (Context->sundec < 0.0) {
		ilat = Context->geom.height - 1;
		lilat = -1;
	} else {
		ilat = 0;
		lilat = 1;
	}

	for (i = ilat; i != Context->geom.height / 2; i += lilat) {
		if (Context->pwtab[i] != -1) {
			while (True) {
				Context->pwtab[i] = Context->geom.width;
				if (i == ilat)
					break;
				i -= lilat;
			}
			break;
		}
	}
}

void
SwitchPixel(Context, x, y, t)
struct Sundata * Context;
register int x;
register int y;
register int t;
{
        int i;
 	Pixel p;
	char *data;

	data = Context->xim->data;
	Context->xim->data = Context->ximdata;
	p = XGetPixel(Context->xim, x, y);
        Context->xim->data = data;
        if (t>=0) {
	  for (i=0; i<Context->attrib->npixels; i++)
	    if (Context->attrib->pixels[i] == p) {     
	        XPutPixel(Context->xim, x, y, Context->darkpixel[i+t*Context->attrib->npixels]);
		break;
	    }
        } else {
	  XPutPixel(Context->xim, x, y, p);
	}
}

/*  XSPAN  --  Complement a span of pixels.  Called with line in which
	       pixels are contained, leftmost pixel in the  line,  and
	       the number of pixels to update.  Handles wrap-around at 
               the right edge of the screen.
               
               The 'state' parameter is a flag with first  3  bits set 
               as follows:
	       bit 0: (0/1) don't use/use progressive shading at  left 
                      border of line
	       bit 1: (0/1) don't use/use progressive shading at right
                      border of line
	       bit 2: (0/1) draw line in dark/light (or invert).
*/

void
xspan(Context, pline, orig, npix, color)
struct Sundata * Context;
register int	pline;
register int	orig;
register int	npix;
register int	color;
{
        int i;

        orig = (orig + Context->geom.width) % Context->geom.width;

	if (orig + npix > Context->geom.width) {
	  if (invert) {
	    XDrawLine(dpy, Context->pix, Invert_gc, 0, pline,
		               (orig + npix) - (Context->geom.width + 1), pline);
 	    XDrawLine(dpy, Context->pix, Invert_gc, orig, pline, 
                                              Context->geom.width - 1, pline);
	  } else {
            for (i=0; i<(orig + npix) - Context->geom.width; i++)
	        SwitchPixel(Context, i, pline, -color);
            for (i=orig; i<Context->geom.width; i++)
	        SwitchPixel(Context, i, pline, -color);
	    }
	}
	else {
	  if (invert)
	     XDrawLine(dpy, Context->pix, Invert_gc, orig, pline,
			  orig + (npix - 1), pline);
	  else {
             for (i=orig; i<orig + npix ; i++)
	        SwitchPixel(Context, i, pline, -color);
	  }
	}
}

/*  MOVETERM  --  Update illuminated portion of the globe.  */

void
moveterm(Context, cnoon)
struct Sundata * Context;
int cnoon;
{
      int i, pl, pr, cl, cr;

      Context->flags.hours_shown = 0;

      if (Context->flags.shading >= 2) {
	 int j, k;
         double light, cp, sp;
         double f1 = 2.0*M_PI/(double)Context->geom.width;
         double f2 = 1.0;

	 if (Context->flags.shading == 2)
            f2 = 180.0 / (M_PI * (SUN_APPRADIUS + atm_refraction));
	 else
            f2 = 180.0 / (M_PI * (SUN_APPRADIUS + atm_diffusion));

	 for (i = 0; i < (int)Context->geom.height; i++) {
	    sp = M_PI*(double)((int)Context->geom.height/2
                               -i)/((double) Context->geom.height);
	    cp = cos(sp)*cos(dtr(Context->sundec));
            sp = sin(sp)*sin(dtr(Context->sundec));
            for (j=0; j<Context->geom.width; j++) {
  	       light = cos(f1*(double)(j-cnoon))*cp+sp;
	       if (Context->flags.shading<4 || light<0) light *= f2;
               k = (int) (0.5*(1.0+light)*color_scale);
               if (k < 0) k = 0; 
               if (k >= color_scale) k = - 1;
	       SwitchPixel(Context, j, i, k);
	    }
	 }
	 return;
      }

      for (i = 0; i < Context->geom.height; i++) {

	     /* If line is off in new width table but is set in
	        the old table, clear it. */

	 if (Context->cwtab[i] < 0) {
	    if (Context->pwtab[i] >= 0)
	       xspan(Context, i, ((Context->noon-Context->pwtab[i]/2)+Context->geom.width)%Context->geom.width,
                        Context->pwtab[i], 0);
	 } else {

	     /* Line is on in new width table.  If it was off in
	        the old width table, just draw it. */

	    if (Context->pwtab[i] < 0) {
	       xspan(Context, i, ((cnoon-Context->cwtab[i]/2)+Context->geom.width)%Context->geom.width,
                        Context->cwtab[i], 1);
	    } else {
	     /* If both the old and new spans were the entire
	       screen, they're equivalent. */
  	       if (Context->pwtab[i] == Context->cwtab[i] && Context->cwtab[i] == Context->geom.width) continue;

	     /* The line was on in both the old and new width
	        tables.  We must adjust the difference in the span.  */

  	       pl = ((Context->noon - Context->pwtab[i]/2) + Context->geom.width) % Context->geom.width;
	       pr = pl + Context->pwtab[i];
	       cl = ((cnoon - Context->cwtab[i]/2) + Context->geom.width) % Context->geom.width;
	       cr = cl + Context->cwtab[i];

	     /* Check whether old or new line spans the entire screen */

               if (Context->cwtab[i] == Context->geom.width) {
		  xspan(Context, i, pr, Context->geom.width - Context->pwtab[i], 1);
                  goto done;
	       }

               if (Context->pwtab[i] == Context->geom.width) {
		  xspan(Context, i, cr, Context->geom.width - Context->cwtab[i], 0);
		  goto done;
	       }
	      
	     /* If spans are disjoint, erase old span and set new span. */

	       if (pr <= cl || cr <= pl) {
	          xspan(Context, i, pl, pr - pl, 0);
		  xspan(Context, i, cl, cr - cl, 1);
	       } else {

               /* Clear portion(s) of old span that extend 
                  beyond end of new span. */
		  if (pl < cl)
		     xspan(Context, i, pl, cl - pl, 0);
	          if (pr > cr)
	             xspan(Context, i, cr, pr - cr, 0);

	       /* Extend existing (possibly trimmed) span to
		  correct new length. */
	          if (cl < pl)
	             xspan(Context, i, cl, pl - cl, 1);
	          if (cr > pr)
		     xspan(Context, i, pr, cr - pr, 1);
	       }
	    }
	 }
       done:
	 Context->pwtab[i] = Context->cwtab[i];
      }
}

/* --- */
/*  UPDIMAGE  --  Update current displayed image.  */

void
updateImage(Context)
struct Sundata * Context;
{
	register int		i;
	int			xl;
	short *			wtab_swap;

	/* If this is a full repaint of the window, force complete
	   recalculation. */

	if (Context->noon < 0) {
		Context->projtime = 0;
		if (invert)
		for (i = 0; i < Context->geom.height; i++)
			Context->cwtab[i] = -1;
		  else
		for (i = 0; i < Context->geom.height; i++)
			Context->cwtab[i] = Context->geom.width;
	}

	time(&Context->time);
	
	if (mono && !Context->flags.firsttime) drawSun(Context);

  	(void) sunParams(Context->time + Context->jump, &Context->sunlon, &Context->sundec, NULL);
	xl = Context->sunlon * (Context->geom.width / 360.0);
	Context->sunlon -= 180.0;

        if (mono) {
	  drawSun(Context);
	  if (Context->flags.firsttime) {
	    drawAll(Context);
	    drawBottomline(Context);
	    Context->flags.firsttime = 0;
	  }
	}

	/* Projecting the illumination curve  for the current seasonal
           instant is costly.  If we're running in real time, only  do
	   it every PROJINT seconds.  */

	if (Context->projtime < 0
	    || (Context->time - Context->projtime) > PROJINT 
            || Context->flags.force_proj) {
		  projillum(Context);
		  wtab_swap = Context->pwtab;
		  Context->pwtab = Context->cwtab;
		  Context->cwtab = wtab_swap;
		  Context->projtime = Context->time;
	}

	/* If the subsolar point has moved at least one pixel, update
	   the illuminated area on the screen.	*/

	if (Context->noon != xl || Context->flags.force_proj) {
		moveterm(Context, xl);
		Context->noon = xl;
		Context->flags.dirty = 1;
		Context->flags.force_proj = 0;
	}
}

void
showImage(Context)
struct Sundata * Context;
{
	time_t                  gtime;

	if (Context->flags.dirty) {
	   if (invert)
	       XCopyPlane(dpy, Context->pix, Context->win, Store_gc, 
		    0, 0, Context->geom.width, Context->geom.height, 0, 0, 1);
	   else
               XPutImage(dpy, Context->win, BigFont_gc, Context->xim, 
                    0, 0, 0, 0,
                    Context->geom.width, Context->geom.height);

	   Context->flags.dirty = 0;
	   if (!mono) {
	       drawAll(Context);
	       drawSun(Context);
	   }
	}
        gtime = Context->time + Context->jump;
	writeStrip(Context, gtime);
}

/*
 * processPoint() - This is kind of a cheesy way to do it but it works. What happens
 *                  is that when a different city is picked, the TZ environment
 *                  variable is set to the timezone of the city and then tzset().
 *                  is called to reset the system.
 */

void
processPoint(Context, x, y)
struct Sundata * Context;
int x, y;      /* Screen co-ordinates of mouse */
{
    /*
     * Local Variables
     */

    City *cptr;    /* Used to search for a city */
    int cx, cy;    /* Screen coordinates of the city */

    /* Loop through the cities until on close to the pointer is found */

    for (cptr = cities; cptr; cptr = cptr->next) {

        /* Convert the latitude and longitude of the cities to integer */

        cx = (180.0 + cptr->lon) * (Context->geom.width / 360.0);
	cy = Context->geom.height - (cptr->lat + 90.0) * (Context->geom.height / 180.0);

	/* Check to see if we are close enough */

	if ((cx >= x - 3) && (cx <= x + 3) && (cy >= y - 3) && (cy <= y + 3 ))
           break;
    }

    if (Context->flags.map_mode == LEGALTIME) {
      if (cptr)
      	Context->flags.map_mode = COORDINATES;
      else
	Context->flags.map_mode = SOLARTIME;
    }

    switch(Context->flags.map_mode) {

      case COORDINATES:
      case EXTENSION:
	if (cptr) {
   	   if (Context->mark1.city) Context->mark1.city->mode = 0;
           Context->mark1.city = cptr;
           cptr->mode = 1;
	   updateMap(Context);
	}
	break;

      case DISTANCES:
        if (Context->mark2.city) Context->mark2.city->mode = 0;
	if (Context->mark1.city == &Context->pos1) {
	    Context->pos2 = Context->pos1;
	    Context->mark2.city = &Context->pos2;
	} else
            Context->mark2.city = Context->mark1.city;
        if (Context->mark2.city) Context->mark2.city->mode = 2;
        if (cptr) {
          Context->mark1.city = cptr;
          Context->mark1.city->mode = 1;
        } else {
          Context->pos1.name = Label[L_POINT];
	  Context->pos1.lat = 90.0-((double)y/(double)Context->geom.height)*180.0 ;
          Context->pos1.lon = ((double)x/(double)Context->geom.width)*360.0-180.0 ;
	  Context->mark1.city= &Context->pos1;
	}
        updateMap(Context);
        break;

      case SOLARTIME:
        if (Context->mark1.city) 
           Context->mark1.city->mode = 0;
        if (cptr) {
          Context->mark1.city= cptr;
          Context->mark1.city->mode = 1;
        } else {
          Context->pos1.name = Label[L_POINT];
	  Context->pos1.lat = 90.0-((double)y/(double)Context->geom.height)*180.0 ;
          Context->pos1.lon = ((double)x/(double)Context->geom.width)*360.0-180.0 ;
	  Context->mark1.city = &Context->pos1;
	}
        updateMap(Context);
        break;

      default:
	break;
    }

    setDayParams(Context);

    if (mono) {
      if (Context->mark1.city) Context->mark1.status = -1;
      if (Context->mark2.city) Context->mark2.status = -1;
    }
}

void
initMenu()
{
	char s[2];
	char *ptr[2] = { Label[L_ESCAPE], Label[L_CONTROLS] };
	int i, j, b, d;

        s[1]='\0';
	d = (5*chwidth)/12;
	for (i=0; i<N_OPTIONS; i++) {
	      b = (Option[2*i+1]==';');
	      for (j=(i+1)*chwidth-b; j<=(i+1)*chwidth+b; j++)
	          XDrawLine(dpy, Menu, BigFont_gc, j, 0, j, MapGeom.strip);
	      s[0]=Option[2*i];
	      XDrawImageString(dpy, Menu, BigFont_gc, d+i*chwidth, 
                  BigFont->max_bounds.ascent + 3, s, 1);
	}
	for (i=0; i<=1; i++)
	   XDrawImageString(dpy, Menu, BigFont_gc, d +(1-i)*N_OPTIONS*chwidth,
                     BigFont->max_bounds.ascent + i*MapGeom.strip + 3, 
		     ptr[i], strlen(ptr[i]));
        XDrawLine(dpy, Menu, BigFont_gc, 0, MapGeom.strip, 
                        MENU_WIDTH * MapGeom.strip, MapGeom.strip);
}


void
PopMenu(Context)
struct Sundata * Context;
{
	int    w, h, a, b;
	
	if (!Context->wintype) return;

	do_menu = 1 - do_menu;

        if (!do_menu) 
	  {
	  XUnmapWindow(dpy, Menu);
	  if (Context == MenuCaller) {
	     MenuCaller = NULL;
	     return;
	     }
          do_menu = 1;
	  }

        MenuCaller = Context;
        Context->flags.last_hint = -1;
	if (!getPlacement(Context->win, &Context->geom.x, &Context->geom.y, &w, &h)) {
	   MenuGeom.x = Context->geom.x + horiz_shift - horiz_drift;
	   /* To center: + (Context->geom.width - MenuGeom.width)/2; */
	   a = Context->geom.y + Context->geom.height + MapGeom.strip + vert_shift;
           b = Context->geom.y - MenuGeom.height - vert_shift - 2*vert_drift;
           if (b < (int) MenuGeom.height ) b = MenuGeom.height;
           if (a > (int) DisplayHeight(dpy,scr) 
                   - 2*MenuGeom.height -vert_drift -20)
              a = b;
	   MenuGeom.y = (placement<=NE)? a : b;              
	}
        setAllHints(NULL, 2);
        if (do_private) XSetWindowColormap(dpy, Menu, MenuCaller->cmap);
        XMoveWindow(dpy, Menu, MenuGeom.x, MenuGeom.y);
	if (do_selector<=1)
           XMapRaised(dpy, Menu);
	else
           XMapWindow(dpy, Menu);
        XMoveWindow(dpy, Menu, MenuGeom.x, MenuGeom.y);
        XFlush(dpy);
}

void
initSelector()
{
	int i, d, p, q, h, skip;
	char *s;
	char *banner[9] = { "home", "share", "  /", "  .", "", "", 
	                    Label[L_ESCAPE], "", Label[L_VECTMAP] };

	d = chwidth/4;

	if (do_selector==1) {

	  for (i=0; i<9; i++)
             XDrawImageString(dpy, Selector, BigFont_gc, d+2*i*chwidth,
                BigFont->max_bounds.ascent + 3, banner[i], strlen(banner[i]));
 
          for (i=1; i<=8; i++) if (i!=7) {
	     h = 2*i*chwidth;
             XDrawLine(dpy, Selector, BigFont_gc, h, 0, h, MapGeom.strip);
	  }

	  /* Drawing small triangular icons */
	  p = MapGeom.strip/4;
	  q = 3*MapGeom.strip/4;
	  h = 9*chwidth;
	  for (i=0; i<=q-p; i++)
	      XDrawLine(dpy,Selector, BigFont_gc,
                    h-i, p+i, h+i, p+i);
	  h = 11*chwidth;
	  for (i=0; i<= q-p; i++)
	      XDrawLine(dpy,Selector, BigFont_gc, h-i, q-i, h+i, q-i);

	  h = MapGeom.strip;
          XDrawLine(dpy, Selector, BigFont_gc, 0, h, SelGeom.width, h);
	}
	
        do_selector = 2;

        XDrawImageString(dpy, Selector, DirFont_gc,
                d, BigFont->max_bounds.ascent + MapGeom.strip + 3, 
                image_dir, strlen(image_dir));

        h = 2*MapGeom.strip;
        XDrawLine(dpy, Selector, BigFont_gc, 0, h, SelGeom.width, h);

	dirtable = get_dir_list(image_dir, &num_table_entries);
	if (dirtable)
           qsort(dirtable, num_table_entries, sizeof(char *), dup_strcmp);
	else {
	   char error[] = "Directory inexistent or inaccessible !!!";
           XDrawImageString(dpy, Selector, DirFont_gc, d, 3*MapGeom.strip,
		     error, strlen(error));
	   return;
	}

	skip = (4*MapGeom.strip)/5;
	num_lines = (SelGeom.height-2*MapGeom.strip)/skip;
        for (i=0; i<num_table_entries-selector_shift; i++) 
	  if (i<num_lines) {
	  s = dirtable[i+selector_shift];
	  h = (s[strlen(s)-1]=='/');
          XDrawImageString(dpy, Selector, h? DirFont_gc : BigFont_gc,
              d, BigFont->max_bounds.ascent + 
              2*MapGeom.strip + i*skip + 3, 
              s, strlen(s));
	}

}

void
PopSelector(Context)
struct Sundata * Context;
{
        int a, b, w, h;

	if (do_selector)
            do_selector = 0;
	else
	    do_selector = 1;

        if (!do_selector) 
	  {
	  XUnmapWindow(dpy, Selector);
	  if (dirtable) free_dirlist(dirtable);
	  dirtable = NULL;
	  if (Context == SelCaller) {
	     SelCaller = NULL;
	     return;
	     }
	  do_selector = 1;
	  }

	SelCaller = Context;
	selector_shift = 0;

	if (!getPlacement(Context->win, &Context->geom.x, &Context->geom.y, &w, &h)) {
	   SelGeom.x = Context->geom.x + horiz_shift - horiz_drift;
	   /* + (Context->geom.width - SelGeom.width)/2; */
	   a = Context->geom.y + Context->geom.height + MapGeom.strip + vert_shift;
           if (Context->wintype && do_menu && Context == MenuCaller) 
               a += MenuGeom.height + vert_drift + vert_shift;
           b = Context->geom.y - SelGeom.height - vert_shift - 2*vert_drift;
           if (b < (int) MenuGeom.strip ) b = MenuGeom.height;
           if (a > (int) DisplayHeight(dpy,scr) 
                   - SelGeom.height - vert_drift -20)
              a = b;
	   SelGeom.y = (placement<=NE)? a : b;              
	}

        setAllHints(NULL, 3);
        if (do_private) XSetWindowColormap(dpy, Selector, SelCaller->cmap);
        XMoveWindow(dpy, Selector, SelGeom.x, SelGeom.y);
        XMapRaised(dpy, Selector);
        XMoveWindow(dpy, Selector, SelGeom.x, SelGeom.y);
	XFlush(dpy);
}

void
showMenuHint(Context, num)
struct Sundata * Context;
int num;
{
	char hint[128], more[128];
	int i,j,k,l;

	if (num == Context->flags.last_hint) return;
        Context->flags.last_hint = num;
        sprintf(hint, " %s", Help[num]); 
        *more = '\0';
	if (num >=4 && num <=6)
	    sprintf(more, " (%s)", Label[L_DEGREE]);
	if (num >=9 && num <=12) {
	    char prog_str[30];
            if (Context->progress == 60) 
                sprintf(prog_str, "1 %s", Label[L_MIN]);
	    else
	    if (Context->progress == 3600)
                sprintf(prog_str, "1 %s", Label[L_HOUR]);
	    else
	    if (Context->progress == 86400) 
                sprintf(prog_str, "1 %s", Label[L_DAY]);
	    else
	    if (Context->progress == 604800) 
                sprintf(prog_str, "7 %s", Label[L_DAYS]);
	    else
	    if (Context->progress == 2592000) 
                sprintf(prog_str, "30 %s", Label[L_DAYS]);
      	    else
                sprintf(prog_str, "%ld %s", Context->progress, Label[L_SEC]);
            sprintf(more, " ( %s %c%s   %s %.3f %s )", 
		  Label[L_PROGRESS], (num==6)? '-':'+', prog_str, 
                  Label[L_TIMEJUMP], Context->jump/86400.0,
		  Label[L_DAYS]);
	}
	if (Option[2*num] == 'H') {
	    sscanf(RELEASE, "%d %d %d", &i, &j, &k);
	    sprintf(more, " (%s %s, %d %s %d, %s)", 
                      ProgName, VERSION, i, Month_name[j-1], k, COPYRIGHT);
	}
	if (Option[2*num] == 'X') {
	    sprintf(more, " : %s", (Command)?Command:"(null)");
	}
        if (*more) strncat(hint, more, 120 - strlen(hint));
        l = strlen(hint);
	if (l<120) {
	    for (i=l; i<120; i++) hint[i] = ' ';
	    hint[120] = '\0';
	}
	XDrawImageString(dpy, Menu, BigFont_gc, 4, 
              BigFont->max_bounds.ascent + MapGeom.strip + 3, 
              hint, strlen(hint));
}

void
createPixmap(Context)
struct Sundata * Context;
{
   char Image_file[1024]="";

   if (invert) {
     makePixmap(Context);
     Context->pix = XCreatePixmapFromBitmapData(dpy, RootWindow(dpy, scr),
          Context->bits, Context->geom.width,
          Context->geom.height, 0, 1, 1);
     free(Context->bits);
     return;
   }

   if (Context->wintype) {
      if (Context->map_img_file!=NULL) {
	if (*Context->map_img_file != '/' && *Context->map_img_file != '.' )
	  sprintf(Image_file, "%s%s", image_dir, Context->map_img_file);
	else
	  strcpy(Image_file, Context->map_img_file);
      }
   } else {
      if (Context->clock_img_file!=NULL) {
	if (*Context->clock_img_file != '/' && *Context->clock_img_file != '.' )
	  sprintf(Image_file, "%s%s", image_dir, Context->clock_img_file);
	else
	  strcpy(Image_file, Context->clock_img_file);
      }
   }


   Context->attrib = (XpmAttributes *) salloc(sizeof(XpmAttributes));
   Context->attrib->valuemask = XpmColormap | XpmReturnPixels;
   Context->attrib->colormap = Context->cmap;

   if (*Image_file) {
      struct stat buf;
      char tmp[80] = "";
      stat(Image_file, &buf);
      if (!S_ISREG(buf.st_mode))
         fprintf(stderr, "File %s doesn't seem to exist !!\n", Image_file);
      else {
         if (do_private) {
            Context->cmap = newColormap();
            getColors(Context->cmap);
         }
         Context->attrib->colormap = Context->cmap;
	 if (!strstr(Image_file, ".xpm") || (color_depth<=8)) {
            char convert[2048];
	    sprintf(tmp, "%s.xpm", tempnam(NULL, "suncl"));
	    sprintf(convert, "convert %s %s xpm:- > %s", 
                Image_file, (color_depth<=8)? "-colors 96":"", tmp);
	    fprintf(stderr, 
               "Using the ImageMagick \"convert\" tool:\n%s\n", convert);
	    system(convert);
	    strcpy(Image_file, tmp);
	 }
         if (XpmReadFileToImage(dpy, Image_file, 
            &Context->xim, NULL, Context->attrib)) {
            fprintf(stderr, "Cannot read XPM file %s,\nalthough it exists !!\n"
		    "Colormap possibly full. Retry with -private option.\n",
                    Image_file);
	    if (*tmp) unlink(tmp);
	 } else {
	    if (*tmp) unlink(tmp);
	    Context->geom.width = Context->attrib->width;
	    Context->geom.height = Context->attrib->height; 
            fixTextPosition(Context);
	    return;
	 }
      }
   }

   /* Otherwise (no pixmap file specified) use default vector map  */
   makePixmap(Context);
   XpmCreateImageFromData(dpy, Context->xpmdata, &Context->xim, 
      NULL, Context->attrib);
   free(Context->xpmdata[0]);
   free(Context->xpmdata);
   if (Context->xim==0) {
      fprintf(stderr, "Cannot create map. Colormap possibly full. "
                     "Retry with -private option.\n");
      shutDown(Context, -1);
   }
}

void
setDarkPixels(Context, a, b)
struct Sundata * Context;
int a, b;
{
     int i, j, k, full=1;
     XColor color, colorp, approx;
     unsigned int red[256], green[256], blue[256];
     double steps[COLORSTEPS];

     if (invert || a==b) return;

     for (j=0; j<color_scale; j++) 
          steps[j] = 1.0 -
              (1.0 - ((double)j)/((double) color_scale))*(1.0 - darkness);

     for (j=a; j<b; j++) {
          for (i=0; i<Context->attrib->npixels; i++) { 
	      color.pixel = Context->attrib->pixels[i];
              XQueryColor(dpy, Context->cmap, &color);
              k = i + j * Context->attrib->npixels;
              colorp.red = (unsigned int) ((double) color.red * steps[j]);
              colorp.green = (unsigned int) ((double) color.green * steps[j]);
              colorp.blue = (unsigned int) ((double) color.blue * steps[j]);
	      colorp.flags = DoRed | DoGreen | DoBlue;
	      if (XAllocColor(dpy, Context->cmap, &colorp))
                 Context->darkpixel[k] = colorp.pixel;
	      else {
		 if (full) {
		   int l;
                   fprintf(stderr, 
                      "Colormap full: cannot allocate color series %d/%d!\n"
		      "Trying to allocate other sufficiently close color.\n"
		      "You'll get a pretty ugly shading anyway...\n",
                      j, color_scale);
		   full = 0;
		   for (l=0; l<256; l++) {
		      approx.pixel = l;
		      XQueryColor(dpy, Context->cmap, &approx);
		      red[l] = approx.red;
		      green[l] = approx.red;
		      blue[l] = approx.blue;
		   }
		 }
		 if (color_depth == 8) {
		    int delta, delta0 = 1000000000, l, l0 = 0;
		    for (l=0; l<256; l++) {
		        int u, v, w;
		        u = ((int)colorp.red - (int)red[l])/16;
                        v = ((int)colorp.green - (int)green[l])/16;
                        w = ((int) colorp.blue - (int)blue[l])/16;
			delta = u*u+v*v+w*w;
			if (delta < delta0) { delta0 = delta; l0 = l;}
		    }
		    Context->darkpixel[k] = l0;
		 } else
		 Context->darkpixel[k] = black;
	      }
	  }
     }
}

void 
createWorkImage(Context)
struct Sundata * Context;
{
   int size;
   if (Context->xim) {
     Context->darkpixel = (Pixel *) 
          salloc(color_scale * (Context->attrib->npixels) * sizeof(Pixel));
     Context->flags.alloc_level = (Context->flags.shading>=2)?color_scale:1;
     setDarkPixels(Context, 0, Context->flags.alloc_level);
     size = Context->xim->width*Context->xim->height*(Context->xim->bitmap_pad/8);
     Context->ximdata = (char *)salloc(size);
     memcpy(Context->ximdata, Context->xim->data, size); 
   }
}

void
checkAuxilWins(Context)
struct Sundata * Context;
{
      if (do_menu || do_selector)
	 usleep(TIMESTEP);

      if (do_selector) 
	 do_selector = 2;

      if (do_menu) {
	     if (Context->wintype) {
	        do_menu = 0;
   	        PopMenu(Context);
	     } else
                XUnmapWindow(dpy, Menu);
      } 
      if (do_selector) {
	  do_selector = 0;
	  PopSelector(Context);
      }
}

void checkLocation();

void 
buildMap(Context, wintype, build)
struct Sundata * Context;
int wintype, build;
{
   if (build) {
      struct Sundata * NewContext;
      NewContext = (struct Sundata *)salloc(sizeof(struct Sundata));
      if (Context) 
         Context->next = NewContext;
      else
         Seed = NewContext;
      Context = NewContext;
      Context->wintype = wintype;
   }
   makeContext(Context, build);
   createPixmap(Context);
   checkGeom(Context, 0);
   createWindow(Context, wintype);
   createWorkImage(Context);
   checkLocation(Context, CityInit);
   setProtocols(Context, wintype);
   setAllHints(Context, wintype);
   XMapWindow(dpy, Context->win);
   if (build)
      Context->prevgeom = Context->geom;
   else
      XMoveWindow(dpy, Context->win, Context->geom.x,Context->geom.y);
   if (Context->wintype)
      Context->mapgeom = Context->geom;
   else
      Context->clockgeom = Context->geom;
   XFlush(dpy);
}

void RaiseAndFocus(win)
Window win;
{
     XFlush(dpy);
     XRaiseWindow(dpy, win);
     XSetInputFocus(dpy, win, RevertToPointerRoot, CurrentTime);
}

void
doTimeout();

/*
 *  Process key events in eventLoop
 */

void
processKey(win, key)
Window  win;
char	key;
{
	int i, old_mode;
	struct Sundata * Context = NULL;

	Context = getContext(win);
	if (!Context) return;

        Context->count = PRECOUNT;

	old_mode = Context->flags.map_mode;

	/* printf("Test: <%c> %u\n", key, key); */
	if (win==Selector) {
	   switch(key) {
	     case '\033':
                if (do_selector) 
	          PopSelector(Context);
		return;
	     case 'U':
	        if (selector_shift == 0) return;
	        selector_shift -= num_lines/2;
	        if (selector_shift <0) selector_shift = 0;
		break;
	     case 'V':
	        if (num_table_entries-selector_shift<num_lines) return;
	        selector_shift += num_lines/2;
		break;
	     case 'R':
	        if (selector_shift == 0) return;
	        selector_shift -= 1;
		break;
	     case 'T':
	        if (num_table_entries-selector_shift<num_lines) return;
	        selector_shift += 1;
		break;
	     case 'P':
	        if (selector_shift == 0) return;
	        selector_shift = 0;
		break;
	     case 'W':
	        if (num_table_entries-selector_shift<num_lines) return;
	        selector_shift = num_table_entries - num_lines+2;
		break;
	     case 'h':
	     case 'q':
	     case 'w':
	        goto general;
	     default :
	        return;
	   }
	   clearSelector();
	   return;
	}
	 
 general:
        switch(key) {
	   case '\011':
	     Context->flags.dms = 1 -Context->flags.dms;
	     break;
	   case '\033':
	     if (do_menu) PopMenu(Context);
	     break;
	   case '<':
	     if (Context->flags.resized) {
		Context->geom = Context->prevgeom;
                if (!invert) {
                  XDestroyImage(Context->xim);
		  Context->xim=0;
	    	}
                adjustGeom(Context, 0);
                shutDown(Context, 0);
	        buildMap(Context, Context->wintype, 0);
		Context->flags.resized = 0;
	     }
             break;
	   case 'P':
	     label_shift = 0;
	     break;
	   case 'W':
	     label_shift = 50;
	     clearStrip(Context);
	     break;
	   case 'U': 
	     if (label_shift>0)
               --label_shift;
	     break;
	   case 'V': 
	     if (label_shift<50)
	       ++label_shift;
	     break;
	   case ' ':
             if (do_menu) PopMenu(Context);
             if (do_selector) PopSelector(Context);
             Context->wintype = 1 - Context->wintype;
             if (Context->wintype)
                Context->geom = MapGeom;
             else
                Context->geom = ClockGeom;
	     adjustGeom(Context, 1);
             shutDown(Context, 0);
             buildMap(Context, Context->wintype, 0);
	     return;
	   case 'a': 
	     Context->jump += Context->progress;
	     Context->flags.force_proj = 1;
	     Context->flags.last_hint = -1;
	     break;
	   case 'b': 
	     Context->jump -= Context->progress;
	     Context->flags.force_proj = 1;
	     Context->flags.last_hint = -1;
	     break;
	   case 'c': 
	     if (Context->flags.map_mode != COORDINATES) 
	       Context->flags.dms = gflags.dms;
	     else
	       Context->flags.dms = 1 - Context->flags.dms;
	     Context->flags.map_mode = COORDINATES;
	     if (Context->mark1.city == &Context->pos1 || Context->mark2.city == &Context->pos2) 
                updateMap(Context);
             if (Context->mark1.city == &Context->pos1) Context->mark1.city = NULL;
	     if (Context->mark1.city)
               setDayParams(Context);
             if (Context->mark2.city) Context->mark2.city->mode = 0;
             Context->mark2.city = NULL;
	     break;
	   case 'd': 
	     if (Context->flags.map_mode != DISTANCES) 
	       Context->flags.dms = gflags.dms;
	     else
	       Context->flags.dms = 1 - Context->flags.dms;
	     Context->flags.map_mode = DISTANCES;
	     break;
	   case 'e': 
	     Context->flags.map_mode = EXTENSION;
	     old_mode = EXTENSION;
	     Context->flags.hours_shown = 0;
	     show_hours(Context);
	     break;
	   case 'f':
	     if (!do_selector)
	        PopSelector(Context);
	     else
                RaiseAndFocus(Selector);
	     break;
	   case 'g': 
	     if (!Context->wintype) break;
	     if (!do_menu)
	       PopMenu(Context);
             else {
	       Context->flags.last_hint = -1; 
               if (Context->flags.progress) {
                 Context->progress = 60;
		 Context->flags.progress = 0;
		 break;
	       }
	       if (Context->progress == 60) Context->progress = 3600;
	         else
	       if (Context->progress == 3600) Context->progress = 86400;
	         else
	       if (Context->progress == 86400) Context->progress = 604800;
	         else
	       if (Context->progress == 604800) Context->progress = 2592000;
       	         else 
	       if (Context->progress == 2592000) {
		 if (progress_init) {
		    Context->progress = progress_init;
                    Context->flags.progress = 1;
		 } else
	            Context->progress = 60;
	       } else
		    Context->progress = 60;
	     }
	     break;
	   case 'h':
	     if (!do_menu) 
	        PopMenu(Context);
	     else
	        if (Context->wintype)
	           RaiseAndFocus(Menu);
	     break;
	   case 'i': 
	     if (Context->wintype && do_menu) PopMenu(Context);
             XIconifyWindow(dpy, Context->win, scr);
	     break;
	   case 'k':
             if (do_menu) PopMenu(Context);
	     if (do_selector) PopSelector(Context);
	     if (Context==Seed && Seed->next==NULL)
	        shutDown(Context, -1);
	     else
	        shutDown(Context, 1);
	     return;
	   case 'l':
	     Context->flags.map_mode = LEGALTIME;
             if (Context->mark1.city) Context->mark1.city->mode = 0;
             if (Context->mark2.city) Context->mark2.city->mode = 0;
             if (Context->mark1.city == &Context->pos1 || Context->mark2.city == &Context->pos2) 
                updateMap(Context);
             Context->mark1.city = NULL;
             Context->mark2.city = NULL;
	     break;
	   case 'm':
             Context->flags.meridian = 1 - Context->flags.meridian;
             if (mono) draw_meridians(Context);
	     exposeMap(Context);
	     break;
	   case 'n':
	     if (invert || (color_scale == 1))
	        Context->flags.shading = 1 - Context->flags.shading;
	     else {
	        int size;
	        Context->flags.shading = (Context->flags.shading+1) % 5;
                if (Context->flags.shading>=2) {
                   setDarkPixels(Context, Context->flags.alloc_level, color_scale);
		   Context->flags.alloc_level = color_scale;
		}
		for (i = 0; i < Context->geom.height; i++)
		    Context->cwtab[i] = Context->geom.width;
                size = Context->xim->width*Context->xim->height*(Context->xim->bitmap_pad/8);
                memcpy(Context->xim->data, Context->ximdata, size); 
	     }
	     Context->flags.force_proj = 1;
	     exposeMap(Context);
	     break;
	   case 'o':
             Context->flags.sun = 1 - Context->flags.sun;
	     if (mono) draw_sun(Context);
	     exposeMap(Context);
             break;
	   case 'p':
             Context->flags.parallel = 1 - Context->flags.parallel;
	     if (mono) draw_parallels(Context);
	     exposeMap(Context);
	     break;
	   case 'q': 
	     shutDown(Context, -1);
	     break;
	   case 's': 
	     if (Context->flags.map_mode != SOLARTIME) 
	       Context->flags.dms = gflags.dms;
	     else
	       Context->flags.dms = 1 - Context->flags.dms;
	     Context->flags.map_mode = SOLARTIME;
             if (Context->mark2.city == &Context->pos2) exposeMap(Context);
             if (Context->mark2.city) Context->mark2.city->mode = 0;
             Context->mark2.city = NULL;
	     if (Context->mark1.city)
	       setDayParams(Context);
	     break;
	   case 't':
             Context->flags.tropics = 1 - Context->flags.tropics;
	     if (mono) draw_tropics(Context);
             exposeMap(Context);
	     break;
	   case 'u':
             if (mono && Context->flags.cities) drawCities(Context);
	     Context->flags.cities = 1 - Context->flags.cities;
             if (mono && Context->flags.cities) drawCities(Context);
             exposeMap(Context);
	     break;
	   case 'w': 
             if (do_menu) PopMenu(Context);
             if (do_selector) PopSelector(Context);
	     if (Context->time<=last_time+2)
		return;
             buildMap(Context, 1, 1);
	     last_time = Context->time;
	     return;
	   case 'r':
	     clearStrip(Context);
	     updateMap(Context);
	     break;
	   case 'x':
	     if (Command) system(Command);
	     break;
	   case 'z':
	     Context->jump = 0;
	     Context->flags.force_proj = 1;
	     Context->flags.last_hint = -1;
	     break;
           default:
	     if (!Context->wintype) {
	       Context->flags.clock_mode = 1-Context->flags.clock_mode;
	       updateMap(Context);
	     }
	     break ;
	}

        if (old_mode == EXTENSION && Context->flags.map_mode != old_mode) clearStrip(Context);

	if (do_menu) {
          for (i=0; i<N_OPTIONS; i++)
	      if (key == Option[2*i]+32) showMenuHint(Context, i);
	}
}

/*
 *  Process mouse events in eventLoop
 */

/*
 *  Process mouse motion events in eventLoop
 */

void
processMotion(win, x, y)
Window  win;
int	x, y;
{
int	click_pos;

	if (win != Menu) return;
	if (y>MapGeom.strip) return;
        click_pos = x/chwidth;
	if (click_pos >= N_OPTIONS) click_pos = N_OPTIONS;
        showMenuHint(MenuCaller, click_pos);
}

void freeImagefiles(Context, bool)
struct Sundata * Context;
int * bool;
{
        if (Context->wintype) {
	   if (Map_img_sel || Context->map_img_file) *bool = 1;
	   if (Map_img_sel) free(Map_img_sel);
           Map_img_sel = NULL;
	   Context->map_img_file = NULL;
        } else {
	   if (Clock_img_sel || Context->clock_img_file) *bool = 1;
	   if (Clock_img_sel) free(Clock_img_sel);
           Clock_img_sel = NULL;
	   Context->clock_img_file = NULL;
	}
}

void 
processSelAction(Context, a, b)
struct Sundata * Context;
int a;
int b;
{
	int  bool;

        if (b <= MapGeom.strip) {
	  a = a/(2*chwidth);
	  if (a==0 && getenv("HOME"))
	     sprintf(image_dir, "%s/", getenv("HOME")); 
	  if (a==1)
	     strcpy(image_dir, share_maps_dir);
	  if (a==2)
	     strcpy(image_dir, "/"); 
	  if (a==3 && getcwd(NULL,1024))
	     sprintf(image_dir, "%s/", getcwd(NULL,1024));
	  if (a<=3)
	     selector_shift = 0;
	  if (a==4) {
	     if (selector_shift == 0) return;
	     selector_shift -= num_lines/2;
	     if (selector_shift <0) selector_shift = 0;
	  }
	  if (a==5) {
	     if (num_table_entries-selector_shift<num_lines) return;
	     selector_shift += num_lines/2;
	  }
	  if (a>=6 && a<=7) {
	     XUnmapWindow(dpy, Selector);
	     do_selector = 0;
	     return;
	  }
	  if (a>=8) {
	      bool=0;
	      freeImagefiles(Context, &bool);
	      if (bool) {
                 if (do_menu) PopMenu(Context);
		 adjustGeom(Context, 0);
	         shutDown(Context, 0);
	         buildMap(Context, Context->wintype, 0);
		 if (do_selector) {
                    RaiseAndFocus(Selector);
		    if (do_private) 
                       XSetWindowColormap(dpy, Selector, SelCaller->cmap);
		 }
	      }
	      return;
	  }
	  clearSelector();
	  return;
	}
        if (b <= 2*MapGeom.strip) {
	  selector_shift = 0;
	  clearSelector();
	  return;
	}
	b = (b-2*MapGeom.strip-3)/(4*MapGeom.strip/5) + selector_shift;
	if (b<num_table_entries) {
	   char *s;
	   s = dirtable[b];
	   if (s==NULL || *s=='\0') return;
	   if (a > XTextWidth (BigFont, s, strlen(s))+chwidth/4) return;
	   b = strlen(s)-1;
           if (s[b] == '/') {
	      int l;
	      if (!strcmp(s, "../")) {
	        l=strlen(image_dir)-1;
		if (l==0) return;
                image_dir[l--] = '\0';
	        while (l>=0 && image_dir[l] != '/')
		   image_dir[l--] = '\0';
		s = "";
	      }
              strcat(image_dir, s);
	      l=strlen(image_dir);
              if (image_dir[l-1] != '/') {
                 image_dir[l] = 'l';
                 image_dir[++l] = '\0';
	      }
	      if (dirtable) free_dirlist(dirtable);
	      dirtable = NULL;
	      selector_shift = 0;
	      num_table_entries=0;
              clearSelector();
	      return;
	   } else {
	      char *f, *Image_file;
	      Image_file = (Context->wintype)? Context->map_img_file : Context->clock_img_file;
	      f = (char *)
                salloc((strlen(image_dir)+strlen(s)+2)*sizeof(char));
	      sprintf(f, "%s%s", image_dir, s);
	      if (!Image_file || strcmp(f, Image_file)) {
	         freeImagefiles(Context, &bool);
                 if (do_menu) PopMenu(Context);
		 adjustGeom(Context, 0);
	         shutDown(Context, 0);
		 if (Context->wintype) 
                    Map_img_sel = Context->map_img_file = f;
		 else
                    Clock_img_sel = Context->clock_img_file = f;
	         buildMap(Context, Context->wintype, 0);
		 if (do_selector) {
                    RaiseAndFocus(Selector);
		    if (do_private) 
                       XSetWindowColormap(dpy, Selector, SelCaller->cmap);
		 }
	      } else free(f);
	   }
	}
}

void
processClick(win, x, y, button)
Window  win;
int	x, y;
int     button;
{
char	         key;
int	         click_pos;
struct Sundata * Context = (struct Sundata *) NULL;

        RaiseAndFocus(win);

        if (win == Menu) {
	   if (y>MapGeom.strip) return;
           click_pos = x/chwidth;
	   if (do_menu && click_pos >= N_OPTIONS) {
	      PopMenu(MenuCaller);
	      return;
	   }
           key = tolower(Option[2*click_pos]);
	   processKey(win, key);
	   return;
	}

        if (win == Selector) {
	   processSelAction(SelCaller, x, y);
	   return;
	}

	Context = getContext(win);
	if (!Context) return;
 
        /* Click on bottom strip of window */
        if (Context->wintype) {
          if(y >= Context->geom.height) {
	     PopMenu(Context);
             return;
	  }

	  if (button==2) {
	     PopSelector(Context);
	     return;
	  }
	  if (button==3) {
	     processKey(win, 'w');
	     return;
	  } 
	} else {
	  if (button==1) {
	     if (y>Context->geom.height) {
	        Context->flags.clock_mode = 1-Context->flags.clock_mode;
	        updateMap(Context);
	     } else
		processKey(win, 'x');
	  }
	  if (button==2)
	     PopSelector(Context);
	  if (button==3)
	     processKey(win, 'w');
	  return;
        }

        Context->count = PRECOUNT;

        /* Erase bottom strip to clean-up spots overlapping bottom strip */
	if (Context->flags.bottom) clearStrip(Context);

        /* Set the timezone, marks, etc, on a button press */
        processPoint(Context, x, y);

        /* if spot overlaps bottom strip, set flag */
 	if (y >= Context->geom.height - radius[spot_size]) {
	   if (Context->flags.map_mode == SOLARTIME)
              Context->flags.bottom = 1;
	   if (Context->flags.map_mode == DISTANCES)
              Context->flags.bottom = 2;
	}
}

void 
processResize(win)
Window win;
{
	   int x, y, w, h;
           struct Sundata * Context = NULL;

	   if (win == Selector) {
	      if (getPlacement(win, &x, &y, &w, &h)) return;
              if (w==SelGeom.width && h==SelGeom.height) return;
	      if (w<SelGeom.w_mini) w = SelGeom.w_mini;
	      if (h<SelGeom.h_mini) h = SelGeom.h_mini;
	      SelGeom.width = w;
	      SelGeom.height = h;
	      XDestroyWindow(dpy, Selector);
              Selector = 0;
	      do_selector = 0;
	      createWindow(NULL, 3);
	      setProtocols(NULL, 3);
	      PopSelector(Context);
	   }

	   if (win == Menu) return;

           Context = getContext(win);
	   if(!Context) return;

	   if (getPlacement(win, &x, &y, &w, &h)) return;
           Context->prevgeom = Context->geom;
           h -= Context->geom.strip;
           if (w==Context->geom.width && h==Context->geom.height) return;
	   if (w<Context->geom.w_mini) w = Context->geom.w_mini;
	   if (h<Context->geom.h_mini) h = Context->geom.h_mini;
	   Context->geom.width = w;
	   Context->geom.height = h;
           if (!invert && Context->xim) {
              XDestroyImage(Context->xim);
              Context->xim=0;
    	   }
           adjustGeom(Context, 0);
           shutDown(Context, 0);
	   buildMap(Context, Context->wintype, 0);
	   Context->flags.resized = 1;
	   checkAuxilWins(Context);
}

/*
 * Got an expose event for window w.  Do the right thing if it's not
 * currently the one we're displaying.
 */

void
doTimeout(Context)
struct Sundata * Context;
{
        if (!Context) return;

	if (QLength(dpy))
		return;		/* ensure events processed first */

        Context->count = (Context->count+1) % TIMECOUNT;

	if (--Context->timeout <= 0 && (Context->count==0)) {
		updateImage(Context);
		showImage(Context);
		Context->timeout = 1;
                if (mono) pulseMarks(Context);
	}
}

void
doExpose(w)
register Window	w;
{
        struct Sundata * Context = (struct Sundata *)NULL;

        if (w == Menu) {
	   initMenu(Context);
	   return;
	}

        if (w == Selector) {
           initSelector(Context);
	   return;
	}

        Context = getContext(w);
	if (!Context) return;

        updateImage(Context);
	Context->flags.dirty = 1;
	showImage(Context);
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

Bool
evpred(d, e, a)
register Display *		d;
register XEvent *		e;
register char *			a;
{
	return (True);
}

void
eventLoop()
{
	XEvent			ev;
	int			key;

	for (;;) {
		if (XCheckIfEvent(dpy, &ev, evpred, (char *)0))

			switch(ev.type) {
		
			case Expose:
				if (ev.xexpose.count == 0)
					doExpose(ev.xexpose.window);
				break;

			case ClientMessage:
				if (ev.xclient.message_type == wm_protocols &&
				    ev.xclient.format == 32 &&
				    ev.xclient.data.l[0] == wm_delete_window) {
					if (ev.xexpose.window == Menu)
					   PopMenu(MenuCaller);
					else
					if (ev.xexpose.window == Selector)
					   PopSelector(SelCaller);
					else
					   shutDown(
                                         getContext(ev.xexpose.window), 1);
				}
				break;

			case KeyPress:
                                key = XKeycodeToKeysym(dpy,ev.xkey.keycode,0);
				processKey(ev.xexpose.window, key & 255);
                                break;

			case ButtonPress:
			        processClick(ev.xexpose.window, 
                                             ev.xbutton.x, ev.xbutton.y,
					     ev.xbutton.button);
				break;

			case MotionNotify:
			        processMotion(ev.xexpose.window, 
                                              ev.xbutton.x, ev.xbutton.y);
			        break;

			case ResizeRequest: 
			        processResize(ev.xexpose.window);
			        break;

			} else {
			        usleep(TIMESTEP);
				doTimeout(getContext(ev.xexpose.window));
			}
	}
}

void checkLocation(Context, name)
struct Sundata * Context;
char *	name;
{
City *c;

	if (CityInit)
        for (c = cities; c; c = c->next)
	    if (!strcasecmp(c->name, CityInit)) {
		Context->mark1.city = c;
		if (mono) Context->mark1.status = -1;
		c->mode = 1;
	        doExpose(Context->win);
		return;
            }

	if (Context->mark1.city == &Context->pos1) {
		Context->flags.map_mode = SOLARTIME;
		Context->mark1.city = NULL;
		setTZ(NULL);
		doTimeout(Context);
		Context->mark1.city = &Context->pos1;
                if (mono) pulseMarks(Context);
		Context->pos1.name = Label[L_POINT];
        }	
}

int
main(argc, argv)
int				argc;
register char **		argv;
{
	char * p;
	int    i;

	ProgName = *argv;

	/* Set default values */
        initValues();

	/* Read the configuation file */

        checkRCfile(argc, argv);
	if (readrc()) exit(1);

	if ((p = strrchr(ProgName, '/'))) ProgName = ++p;

	leave = 1;
	(void) parseArgs(argc, argv);

	dpy = XOpenDisplay(Display_name);
	if (dpy == (Display *)NULL) {
		fprintf(stderr, "%s: can't open display `%s'\n", ProgName,
			Display_name);
		exit(1);
	}

	scr = DefaultScreen(dpy);
        color_depth = DefaultDepth(dpy, scr);
        wm_protocols = XInternAtom(dpy, "WM_PROTOCOLS", False);
        wm_delete_window = XInternAtom(dpy, "WM_DELETE_WINDOW", False);

        /* Correct some option parameters */
	if (placement<0) placement = NW;
	if (invert && (gflags.shading>=2)) gflags.shading = 1;
	if (color_depth>8) do_private = 0;

	cmap0 = newColormap();
	getColors(cmap0);

	getFonts();
	for (i=2; i<=3; i++) {
	   createWindow(NULL, i);
	   setProtocols(NULL, i);
	   setAllHints(NULL, i);
	}
        buildMap(NULL, win_type, 1);
        createGCs(Seed);
	eventLoop();
	exit(0);
}
