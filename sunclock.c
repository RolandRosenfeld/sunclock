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
                       loadable xpm maps,...), now hopefully sunclock looks 
                       nice!
       3.21  12/04/00  Final bug fix release of 3.2x x<5
       3.25  12/21/00  Dockable, multi-window version
       3.28  01/15/01  Final bug fix release of 3.2x x>=5
       3.30  02/20/01  Sunclock now loads JPG images, and images are resizable
*/

#include <unistd.h>
#include <sys/types.h>
#include <sys/timeb.h>
#include <sys/stat.h>
#include <string.h>

#include "sunclock.h"
#include "langdef.h"

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
extern int	readVMF();
extern int	readJPEG();
extern int	readXPM();

extern char **get_dir_list();
extern int dup_strcmp();
extern void free_dirlist();


char share_i18n[] = SHAREDIR"/i18n/Sunclock.**";
char app_default[] = SHAREDIR"/Sunclockrc";
char Default_vmf[] = SHAREDIR"/earthmaps/vmf/standard.vmf";
char *share_maps_dir = SHAREDIR"/earthmaps/";
char *Clock_img_file;
char *Map_img_file;

char image_dir[1024];

char *SmallFont_name = SMALLFONT;
char *BigFont_name = BIGFONT;

char **dirtable;
char * freq_filter = "";

char language[4] = "";

char *  rc_file = NULL;
char *	ProgName;
char *  Command = NULL;

char    StdFormats[] = STDFORMATS;
char *  ListFormats = StdFormats;
char ** DateFormat;

struct Sundata *Seed, *MenuCaller, *SelCaller, *ZoomCaller;

char	TextBgColor[COLORLENGTH], TextFgColor[COLORLENGTH], 
        DirColor[COLORLENGTH], ImageColor[COLORLENGTH],
	CityColor0[COLORLENGTH], CityColor1[COLORLENGTH], 
        CityColor2[COLORLENGTH],
	MarkColor1[COLORLENGTH], MarkColor2[COLORLENGTH], 
        LineColor[COLORLENGTH], TropicColor[COLORLENGTH], 
        SunColor[COLORLENGTH];

char *		Display_name = "";

char *          CityInit = NULL;

Display *	dpy;
Visual		visual;
Colormap        cmap0 = 0;
GClist          gcl0;
Pixlist         pxl0;

int		scr;
int             color_depth;
int             color_pad;
int		bytes_per_pixel;

unsigned long	black;
unsigned long	white;

Atom		wm_delete_window;
Atom		wm_protocols;

XFontStruct *	SmallFont;
XFontStruct *	BigFont;

Window		Menu = 0;
Window		Selector = 0;
Window		Zoom = 0;

struct Geometry MapGeom =   { 0, 30,  30, 768, 384, 320, 160, 20 };
struct Geometry	ClockGeom = { 0, 30,  30, 128,  64,  48,  24, 20 };

struct Geometry	MenuGeom =  { 0, 30, 430, 700,  40, 700,  40,  0 };
struct Geometry	SelGeom =   { 0, 30,  40, 600, 180, 450,  80,  0 };
struct Geometry	ZoomGeom =  { 0, 30,  40, 500, 320, 375, 150,  0 };

int             radius[5] = {0, 2, 2, 3, 5};

Flags           gflags;

int             win_type = 0;
int             spot_size = 3;
int             dotted = 0;
int		placement = -1;
int             mono = 0;
int             invert = 0;
int             private = 0;
int             always_private = 0;
int             color_alloc_failed = 0;
int		fill_mode = 2;
int             num_formats;
int             leave;
int             verbose = 0;
int      	chwidth;
int		num_lines;
int             num_table_entries;

int             horiz_shift = 0;
int             vert_shift = 12;
int		label_shift = 0;
int		selector_shift = 0;
int		zoom_shift = 0;

int             horiz_drift = 0;
int             vert_drift =  0;

int		do_menu = 0;
int             do_selector = 0;
int		do_zoom = 0;

int             do_hint = 0;
int             do_dock = 0;
int             do_sync = 0;
int             do_title = 1;

int             time_jump = 0;
int             progress_init = 0;

unsigned int    adjust_dark;
unsigned int	color_scale = 16;

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
     "%s [-help] [-listmenu] [-version] [-verbose] [-title] [-notitle]\n"
     SP"[-display name] [-bigfont name] [-smallfont name] [-language name]\n"
     SP"[-mono] [-invert] [-private] [-dock] [-synchro] [-nosynchro]\n"
     SP"[-placement (random, fixed, center, NW, NE, SW, SE)]\n"
     SP"[-rcfile file] [-sharedir directory]\n"
     SP"[-mapimage file] [-clockimage file]\n"
     SP"[-clock] [-dateformat string1|string2|...] [-command string]\n"
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
     SP"[-spotsize size(0,1,2,3,4)] [-dottedlines] [-plainlines]\n"
     SP"[-textbgcolor color] [-textfgcolor color]\n"
     SP"[-dircolor color] [-imagecolor color]\n"
     SP"[-suncolor color] [-linecolor color] [-tropiccolor color]\n"
     SP"[-citycolor0 color] [-citycolor1 color] [-citycolor2 color]\n"
     SP"[-markcolor1 color] [-markcolor2 color]\n\n", 
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
        gflags.sunpos = 1;
        gflags.meridian = 0;
        gflags.parallel = 0;
        gflags.tropics = 0;
        gflags.resized = 0;

        strcpy(image_dir, share_maps_dir);
        Clock_img_file = Default_vmf;
        Map_img_file = Default_vmf;

	strcpy(TextBgColor, "Grey92");
	strcpy(TextFgColor, "Black");
	strcpy(DirColor, "Blue");
	strcpy(ImageColor, "Magenta");
	strcpy(CityColor0, "Orange");
	strcpy(CityColor1, "Red");
	strcpy(CityColor2, "Red3");
	strcpy(MarkColor1, "Pink1");
	strcpy(MarkColor2, "Pink2");
	strcpy(LineColor, "White");
	strcpy(TropicColor, "White");
	strcpy(SunColor, "Yellow");

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
parseFormats(char * format)
{
        int i, j, l;
	
	l = strlen(format);

	j = 1;
	while (j>0) {
	   j = 0;
	   if (format[0]=='|') { ++format; --l; j = 1;}
	   if (l>=2 && format[l-1]=='|' && format[l-2]!= '%') 
                               { format[l-1]='\0'; --l; j = 1; }
	}

	num_formats = 1;
	for (i=1; i<l-1; i++) 
            if (format[i]=='|' && format[i-1]!='%') ++num_formats;

        DateFormat = (char **) salloc(num_formats*sizeof(char *));

	DateFormat[0] = format;
        j = 0;

	for (i=1; i<l-1; i++) 
          if (format[i]=='|' && format[i-1]!='%') {
	    ++j;
	    format[i] = '\0';
	    DateFormat[j] = format+i+1;
	  }
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
                        if (spot_size>4) spot_size = 4;
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
			darkness = atof(*++argv);
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
 		else if (strcasecmp(*argv, "-textbgcolor") == 0) {
			needMore(argc, argv);
			strncpy(TextBgColor, *++argv, COLORLENGTH);
			--argc;
		}
		else if (strcasecmp(*argv, "-textfgcolor") == 0) {
			needMore(argc, argv);
			strncpy(TextFgColor, *++argv, COLORLENGTH);
			--argc;
		}
		else if (strcasecmp(*argv, "-dircolor") == 0) {
			needMore(argc, argv);
			strncpy(DirColor, *++argv, COLORLENGTH);
			--argc;
		}
		else if (strcasecmp(*argv, "-imagecolor") == 0) {
			needMore(argc, argv);
			strncpy(ImageColor, *++argv, COLORLENGTH);
			--argc;
		}
		else if (strcasecmp(*argv, "-citycolor0") == 0) {
			needMore(argc, argv);
			strncpy(CityColor0, *++argv, COLORLENGTH);
			--argc;
		}
		else if (strcasecmp(*argv, "-citycolor1") == 0) {
			needMore(argc, argv);
			strncpy(CityColor1, *++argv, COLORLENGTH);
			--argc;
		}
		else if (strcasecmp(*argv, "-citycolor2") == 0) {
			needMore(argc, argv);
			strncpy(CityColor2, *++argv, COLORLENGTH);
			--argc;
		}
		else if (strcasecmp(*argv, "-markcolor1") == 0) {
			needMore(argc, argv);
			strncpy(MarkColor1, *++argv, COLORLENGTH);
			--argc;
		}
		else if (strcasecmp(*argv, "-markcolor2") == 0) {
			needMore(argc, argv);
			strncpy(MarkColor2, *++argv, COLORLENGTH);
			--argc;
		}
		else if (strcasecmp(*argv, "-linecolor") == 0) {
			needMore(argc, argv);
			strncpy(LineColor, *++argv, COLORLENGTH);
			--argc;
		}
		else if (strcasecmp(*argv, "-tropiccolor") == 0) {
			needMore(argc, argv);
			strncpy(TropicColor, *++argv, COLORLENGTH);
			--argc;
		}
		else if (strcasecmp(*argv, "-suncolor") == 0) {
			needMore(argc, argv);
			strncpy(SunColor, *++argv, COLORLENGTH);
			--argc;
		}
		else if (strcasecmp(*argv, "-position") == 0) {
			needMore(argc, argv);
			CityInit = NULL;
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
			Command = RCAlloc(*++argv);
			--argc;
		}
		else if (strcasecmp(*argv, "-dateformat") == 0) {
			needMore(argc, argv);
			ListFormats = RCAlloc(*++argv);
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
                        spot_size = 4;
                        mono = 1;
			invert = 1;
			fill_mode = 1;
                }
		else if (strcasecmp(*argv, "-invert") == 0) {
		        invert = 1;
			fill_mode = 1;
		}
		else if (strcasecmp(*argv, "-private") == 0)
		        always_private = 1;
		else if (strcasecmp(*argv, "-verbose") == 0)
		        verbose = 1;
		else if (strcasecmp(*argv, "-dock") == 0)
		        do_dock = 1;
		else if (strcasecmp(*argv, "-synchro") == 0)
		        do_sync = 1;
		else if (strcasecmp(*argv, "-nosynchro") == 0)
		        do_sync = 0;
		else if (strcasecmp(*argv, "-title") == 0)
		        do_title = 1;
		else if (strcasecmp(*argv, "-notitle") == 0)
		        do_title = 0;
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
			gflags.sunpos = 1;
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
			gflags.sunpos = 0;
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

        j=0;
	while (j<128 && isspace(buf[j]) && buf[j] != '0') ++j; 
        if ((buf[j] == '#') || (buf[j] == '\0')) continue;

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
    if (!(language[0] && language[1]))
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


struct Sundata *
getContext(win)
Window win;
{
   struct Sundata * Context;
   if (win==Menu) 
     return MenuCaller;
   if (win==Selector)
     return SelCaller;
   if (win==Zoom)
     return ZoomCaller;

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
destroyGCs(gclist)
GClist gclist;
{
 	 XFreeGC(dpy, gclist.bigfont);
 	 XFreeGC(dpy, gclist.smallfont);

         if (invert)
 	    XFreeGC(dpy, gclist.store);

 	 if (!mono) {
 	    XFreeGC(dpy, gclist.citycolor0);
 	    XFreeGC(dpy, gclist.citycolor1);
 	    XFreeGC(dpy, gclist.citycolor2);
 	    XFreeGC(dpy, gclist.markcolor1);
 	    XFreeGC(dpy, gclist.markcolor2);
 	    XFreeGC(dpy, gclist.linecolor);
 	    XFreeGC(dpy, gclist.tropiccolor);
 	    XFreeGC(dpy, gclist.suncolor);
            XFreeGC(dpy, gclist.dirfont);
 	 }
}

void PopMenu();
void PopSelector();
void PopZoom();

void
shutDown(Context, all)
struct Sundata * Context;
int all;
{
        struct Sundata * ParentContext, *NextContext;

	if (all<0)
	   Context = Seed;

      repeat:
        NextContext = Context->next;
        ParentContext = parentContext(Context);

	if (Context->xim) {
           XDestroyImage(Context->xim); 
           Context->xim = NULL;
	}
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
	if (Context->pix) {
           XFreePixmap(dpy, Context->pix);
	   Context->pix = 0;
	}
        if (Context->gclist.invert) {
	   XFreeGC(dpy, Context->gclist.invert);
	   Context->gclist.invert = 0;
	}
        if (Context->cmap!=cmap0) {
	   XFreeColormap(dpy, Context->cmap);
           destroyGCs(Context->gclist);
	   Context->cmap = 0;
	}  

	Context->flags.hours_shown = 0;
	Context->flags.firsttime = 1;

        if (all) {
	   last_time = 0;

           if (Context->win) {
	      if (do_menu && Context == MenuCaller) PopMenu(Context);
	      if (do_selector && Context == SelCaller) PopSelector(Context);
	      if (do_zoom && Context == ZoomCaller) PopZoom(Context);
	      XDestroyWindow(dpy, Context->win);
  	      Context->win = 0;
	   }
           if (Context->daypixel) {
	      free(Context->daypixel);
	      Context->daypixel = NULL;
           }
           if (Context->nightpixel) {
	      free(Context->nightpixel);
	      Context->nightpixel = NULL;
           }
           if (Context->clock_img_file)
              free(Context->clock_img_file);
           if (Context->map_img_file)
              free(Context->map_img_file);
	   if (all<0) {
	      free(Context);
	      if (NextContext) {
	         Context = NextContext;
	         goto repeat;
	      }
	      else {
	        endup:
                 if (cmap0) destroyGCs(gcl0);
         	 XDestroyWindow(dpy, Menu);
         	 XDestroyWindow(dpy, Selector);
         	 XDestroyWindow(dpy, Zoom);
         	 XFreeFont(dpy, BigFont);
         	 XFreeFont(dpy, SmallFont);
                 XCloseDisplay(dpy);
         	 if (dirtable) free(dirtable);
         	 exit(0);
 	      }
 	   }
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
            xsh.min_width = Geom->w_mini;
            xsh.min_height = Geom->h_mini;
	} else 
	if (num>=2) {
	    xsh.flags = PSize | USPosition | PMinSize;
	    if (num==2) {
	      win = Menu;
	      Geom = &MenuGeom;
	      xsh.flags |= PMaxSize;
	      xsh.min_width = xsh.max_width = Geom->width;
              xsh.min_height = xsh.max_height = Geom->height;
	    }
	    if (num==3) {
	      win = Selector;
	      Geom = &SelGeom;
	      xsh.min_width = Geom->w_mini;
              xsh.min_height = Geom->h_mini;
	    }
	    if (num==4) {
	      win = Zoom;
	      Geom = &ZoomGeom;
	      xsh.min_width = Geom->w_mini;
              xsh.min_height = Geom->h_mini;
	    }
	    xsh.x = Geom->x;
	    xsh.y = Geom->y;
	    xsh.width = Geom->width;
	    xsh.height = Geom->height;
	}

	if (!win) return;
	XSetNormalHints(dpy, win, &xsh);
}

void
setClassHints(win, num)
Window win;
int    num;
{
        XClassHint xch;
        char 			name[80];	/* Used to change icon name */
	char *instance[5] =     { "clock", "map", "menu", "selector", "zoom" };

        sprintf(name, "%s %s", ProgName, VERSION);
        XSetIconName(dpy, win, name);
        xch.res_name = ProgName;
        xch.res_class = "Sunclock";
        XSetClassHint(dpy, win, &xch);
        sprintf(name, "%s / %s", ProgName, instance[num]);
        XStoreName(dpy, win, name);
}

void
setProtocols(Context, num)
struct Sundata * Context;
int				num;
{
	Window			win = 0;
	int                     mask;

	mask = VisibilityChangeMask | 
               ExposureMask | ButtonPressMask | KeyPressMask;

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

	   case 4:
	        win = Zoom;
		mask |= ResizeRedirectMask;
		break;
	}

	if (!win) return;

       	XSelectInput(dpy, win, mask);
	XSetWMProtocols(dpy, win, &wm_delete_window, 1);
	if (num>=2 || !do_title) setClassHints(win, num);
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
	struct Geometry *	Geom = NULL;
        Window *		win = NULL;
	register int		mask;

	xswa.background_pixel = white;
	xswa.border_pixel = black;
	xswa.backing_store = WhenMapped;

	mask = CWBackPixel | CWBorderPixel | CWBackingStore;

        switch (num) {

	   case 0:
	        win = &Context->win;
		Geom = &Context->geom;
		break;

	   case 1:
	        win = &Context->win;
		Geom = &Context->geom;
		break;

	   case 2:
	        if (!Menu) win = &Menu;
		Geom = &MenuGeom;
		break;

	   case 3:
	        if (!Selector) win = &Selector;
		Geom = &SelGeom;
		break;

	   case 4:
	        if (!Zoom) win = &Zoom;
		Geom = &ZoomGeom;
		break;
	}

	if (num<=1) fixGeometry(&Context->geom);

        if (win) {
	     *win = XCreateWindow(dpy, root,
		      Geom->x, Geom->y,
		      Geom->width, Geom->height+Geom->strip, 0,
		      CopyFromParent, InputOutput, 
		      CopyFromParent, mask, &xswa);
	}
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
		color_alloc_failed = 1;
		return(other);
	}
	else
	        return(c.pixel);
}

void
createGCs(Context)
struct Sundata * Context;
{
	XGCValues		gcv;
	Window                  root = RootWindow(dpy, scr);
	Colormap                cmap;

	cmap = Context->cmap;
        Context->gclist.invert = 0;

        if (mono) {
           Context->pixlist.white = white;
           Context->pixlist.black = black;
	} else {
           Context->pixlist.black = getColor("Black", white, cmap);
           Context->pixlist.white = getColor("White", white, cmap);
           Context->pixlist.textfgcolor = getColor(TextFgColor, black, cmap);
           Context->pixlist.textbgcolor = getColor(TextBgColor, white, cmap);
           Context->pixlist.textfgcolor = getColor(TextFgColor, black, cmap);
           Context->pixlist.dircolor    = getColor(DirColor, black, cmap);
           Context->pixlist.imagecolor  = getColor(ImageColor, black, cmap);
           Context->pixlist.citycolor0  = getColor(CityColor0, black, cmap);
           Context->pixlist.citycolor1  = getColor(CityColor1, black, cmap);
           Context->pixlist.citycolor2  = getColor(CityColor2, black, cmap);
	   Context->pixlist.markcolor1  = getColor(MarkColor1, black, cmap);
	   Context->pixlist.markcolor2  = getColor(MarkColor2, black, cmap);
	   Context->pixlist.linecolor   = getColor(LineColor, black, cmap);
	   Context->pixlist.tropiccolor = getColor(TropicColor, black, cmap);
	   Context->pixlist.suncolor    = getColor(SunColor, white, cmap);
	}

	if (color_alloc_failed) return;

	if (invert) {
           gcv.background = white;
           gcv.foreground = black;
	   Context->gclist.store = XCreateGC(dpy, root, 
                             GCForeground | GCBackground, &gcv);
	}

        if (!mono) {
	   gcv.background = Context->pixlist.textbgcolor;
	   gcv.foreground = Context->pixlist.textfgcolor;
	}

	gcv.font = SmallFont->fid;
	Context->gclist.smallfont = XCreateGC(dpy, root, GCForeground | GCBackground | GCFont, &gcv);
	gcv.font = BigFont->fid;
	Context->gclist.bigfont = XCreateGC(dpy, root, GCForeground | GCBackground | GCFont, &gcv);
        
        if (mono) {
	   Context->gclist.dirfont = Context->gclist.bigfont;
	   Context->gclist.imagefont = Context->gclist.bigfont;
	   return;
	}

	gcv.foreground = Context->pixlist.dircolor;
	gcv.font = BigFont->fid;
	Context->gclist.dirfont = XCreateGC(dpy, root, GCForeground | GCBackground | GCFont, &gcv);

	gcv.foreground = Context->pixlist.imagecolor;
	gcv.font = BigFont->fid;
	Context->gclist.imagefont = XCreateGC(dpy, root, GCForeground | GCBackground | GCFont, &gcv);

	gcv.foreground = Context->pixlist.citycolor0;
	Context->gclist.citycolor0 = XCreateGC(dpy, root, GCForeground, &gcv);

	gcv.foreground = Context->pixlist.citycolor1;
	Context->gclist.citycolor1 = XCreateGC(dpy, root, GCForeground, &gcv);

	gcv.foreground = Context->pixlist.citycolor2;
	Context->gclist.citycolor2 = XCreateGC(dpy, root, GCForeground, &gcv);

	gcv.foreground = Context->pixlist.markcolor1;
	Context->gclist.markcolor1 = XCreateGC(dpy, root, GCForeground, &gcv);

	gcv.foreground = Context->pixlist.markcolor2;
	Context->gclist.markcolor2 = XCreateGC(dpy, root, GCForeground, &gcv);

	gcv.foreground = Context->pixlist.linecolor;
	Context->gclist.linecolor = XCreateGC(dpy, root, GCForeground, &gcv);

	gcv.foreground = Context->pixlist.tropiccolor;
	Context->gclist.tropiccolor = XCreateGC(dpy, root, GCForeground, &gcv);

	gcv.foreground = Context->pixlist.suncolor;
	gcv.font = BigFont->fid;
	Context->gclist.suncolor = XCreateGC(dpy, root, GCForeground, &gcv);
}

void
clearStrip(Context)
struct Sundata * Context;
{
        XSetWindowBackground(dpy, Context->win, 
           (mono)? Context->pixlist.white : Context->pixlist.textbgcolor);
        XClearArea(dpy, Context->win, 0, Context->geom.height, 
                 Context->geom.width, Context->geom.strip, True);
        XSetWindowBackground(dpy, Context->win, 
           (invert)? Context->pixlist.white : Context->pixlist.textbgcolor);
	Context->count = PRECOUNT;
	if (Context->flags.bottom) --Context->flags.bottom;
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
 *  Set the timezone of selected location.
 *  This is definitely the most tricky point in the whole sunclock stuff
 *  because of the incompatible Unix implementations !
 */

void
setTZ(cptr)
City	*cptr;
{
#ifndef _OS_LINUX_
	char buf[80];
#endif

   	if (cptr && cptr->tz) {
#ifdef _OS_LINUX_
           setenv("TZ", cptr->tz, 1);
#else
	   sprintf(buf, "TZ=%s", cptr->tz);
#ifdef _OS_HPUX_
           putenv(strdup(buf));
#else
 	   putenv(buf);
#endif
#endif
	   } 
	else
#ifdef _OS_LINUX_
           unsetenv("TZ");
#else
	   {
	   /* This is supposed to reset timezone to default localzone */
	   strcpy(buf, "TZ");
	   /* Another option would be to set LOCALZONE adequately and put:
           strcpy(buf, "TZ="LOCALZONE); */
#ifdef _OS_HPUX_
           putenv(strdup(buf));
#else
 	   putenv(buf);
#endif
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
   	    XDrawImageString(dpy, Context->win, Context->gclist.bigfont, 
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
		char num[80];
		setTZ(NULL);
		ltp = *localtime(&gtime);
                l = strlen(DateFormat[Context->flags.clock_mode]);
		*s = '\0';
		for (i=0; i<l; i++) {
		   char c = DateFormat[Context->flags.clock_mode][i];
		   if (c != '%') { 
		      num[0] = c;
                      num[1] = '\0'; 
		   }
                   if (c == '%' && i<l-1) {
		      ++i; 
		      c = DateFormat[Context->flags.clock_mode][i];
                      switch(c) {
		        case 'H': sprintf(num, "%02d", ltp.tm_hour); break;
		        case 'M': sprintf(num, "%02d", ltp.tm_min); break;
		        case 'S': sprintf(num, "%02d", ltp.tm_sec); break;
#ifdef NEW_CTIME
			case 'Z': strcpy(num, ltp.tm_zone); break;
#else
			case 'Z': strcpy(num, tzname[ltp.tm_isdst]); break;
#endif
                        case 'a': strcpy(num, Day_name[ltp.tm_wday]); break;
		        case 'd': sprintf(num, "%02d", ltp.tm_mday); break;
		        case 'j': sprintf(num, "%02d", 1+ltp.tm_yday); break;
		        case 'b': strcpy(num, Month_name[ltp.tm_mon]); break;
		        case 'm': sprintf(num, "%02d", 1+ltp.tm_mon); break;
		        case 't': {
			   int w = ltp.tm_year+1900;
			   if (w % 4==0 && (w % 100!=0 || w % 400 == 0))
			     w = 366;
			   else
			     w = 365;
			   sprintf(num, "%d", w);
			   break;
			   }
		        case 'y': sprintf(num, "%02d", ltp.tm_year%100); break;
		        case 'Y': sprintf(num, "%d", ltp.tm_year+1900); break;
		        case 'U': {
	                   struct tm ftm;
	                   time_t ftime;
	                   int w;
		           /*
                            * define weeknumber
                            * week #1 = week with the first thursday
                            */
		           /* set reference date to 1st of january, 12:00:00 */
                           (void) memset(&ftm, 0, sizeof(struct tm));
                           ftm.tm_isdst = -1;
                           ftm.tm_mon = 0;
                           ftm.tm_mday = 1;
                           ftm.tm_year = ltp.tm_year;
                           ftm.tm_hour = 12;
                           ftm.tm_min = 0;
                           ftm.tm_sec = 0;
                           ftime = mktime(&ftm);
                           ftm = *localtime(&ftime);
		           /* get first sunday (start of week) */
                           if (ftm.tm_wday < 5)
                              w = 1 - ftm.tm_wday;
		           else
		              w = 8 - ftm.tm_wday;
		           /* get weeknumber */
		           sprintf(num, "%02d", 
                                ((ltp.tm_yday+1-ltp.tm_wday-w)/7)+1);
                           break; 
			   }
		        case '_': c = ' ';
		        default: num[0] = c; num[1] = '\0'; break;
		      }
		   }
		   strcat(s, num);
		}
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
             (Context->wintype)? Context->gclist.bigfont : Context->gclist.smallfont, 
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
	   Context->win = 0;
	   Context->cmap = 0;
	   Context->mapgeom = MapGeom;
	   Context->clockgeom = ClockGeom;
           if (Context->wintype)
              Context->geom = Context->mapgeom;
           else
 	      Context->geom = Context->clockgeom;
           Context->flags = gflags;
           Context->jump = time_jump;
           Context->progress = (progress_init)? progress_init : 60;
           Context->clock_img_file = RCAlloc(Clock_img_file);
           Context->map_img_file = RCAlloc(Map_img_file);
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
	   if (color_depth>8) {
              Context->gclist = gcl0;
              Context->pixlist = pxl0;
	   }
        }
        fixTextPosition(Context);
        Context->local_day = -1;
        Context->solar_day = -1;
        Context->xim = NULL;
        Context->ximdata = NULL;
	if (color_depth<=8) {
           Context->daypixel = (unsigned char *) salloc(256*sizeof(char));
           Context->nightpixel = (unsigned char *) salloc(256*sizeof(char));
	} else {
           Context->daypixel = NULL;
           Context->nightpixel = NULL;
	}
        Context->pix = 0;
        Context->bits = 0;
	Context->noon = -1;
	Context->pwtab = (short *)salloc((int)(Context->geom.height * sizeof (short *)));
	Context->cwtab = (short *)salloc((int)(Context->geom.height * sizeof (short *)));
	Context->time = 0L;
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
           *pgc = Context->gclist.invert; 
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
    GC gc = Context->gclist.citycolor0;
    Window w;

    if (mode < 0) return;
    if (!mono && !Context->flags.cities && mode <= 2) return;

    if (mode == 0) { gc = Context->gclist.citycolor0; --mode; }
    if (mode == 1)   gc = Context->gclist.citycolor1;
    if (mode == 2)   gc = Context->gclist.citycolor2;
    if (mode == 3)   gc = Context->gclist.markcolor1;
    if (mode == 4)   gc = Context->gclist.markcolor2;
    if (mode == 5)   gc = Context->gclist.suncolor;

    checkMono(Context, &w, &gc);

    ilat = Context->geom.height - (lat + 90) * (Context->geom.height / 180.0);
    ilon = (180.0 + lon) * (Context->geom.width / 360.0);

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

    rad = radius[spot_size];

    if (spot_size != 3)
       XDrawArc(dpy, w, gc, ilon-rad, ilat-rad, 2*rad, 2*rad, 0, 360 * 64);

    if (spot_size == 2)
       XDrawLine(dpy, w, gc, ilon, ilat, ilon, ilat);

    if (spot_size == 3)
       XFillArc(dpy, w, gc, ilon-rad, ilat-rad, 2*rad, 2*rad, 0, 360 * 64);  

    if (spot_size == 4)
       XFillArc(dpy, w, gc, ilon-3, ilat-3, 6, 6, 0, 360 * 64);
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
	  draw_parallel(Context, Context->gclist.linecolor, i*10.0, 3, dotted);
}

void
draw_tropics(Context)
struct Sundata * Context;
{
	static  double val[5] = { -66.55, -23.45, 0.0, 23.45, 66.55 };
	int     i;

        if (mono && Context->flags.parallel)
	  draw_parallel(Context, Context->gclist.linecolor, 0.0, 3, dotted);

        for (i=0; i<5; i++)
	  draw_parallel(Context, Context->gclist.tropiccolor, val[i], 3, 1);
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
        GC gc = Context->gclist.linecolor;
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
	if (Context->flags.sunpos)
  	  draw_sun(Context);
}

void
drawBottomline(Context)
struct Sundata * Context;
{
        Window w = Context->win;
	GC gc = Context->gclist.bigfont;

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
DarkenPixel(Context, x, y, t)
struct Sundata * Context;
int x;
int y;
int t;
{
        register int i;
	unsigned int factor;
	unsigned char u, v, w, r, g, b;

	i = bytes_per_pixel * x + Context->xim->bytes_per_line * y;

	if (color_depth>16) {
#ifdef BIGENDIAN
	   i += bytes_per_pixel - 3;
#endif
           u = Context->ximdata[i];
           v = Context->ximdata[i+1];
           w = Context->ximdata[i+2];
           if (t>=0) {
	      factor = adjust_dark + (t * (255-adjust_dark))/color_scale;
	      u = (u * factor)/255;
	      v = (v * factor)/255;
	      w = (w * factor)/255;
	   }
           Context->xim->data[i] = u;
 	   Context->xim->data[i+1] = v;
           Context->xim->data[i+2] = w;
	} else 
	if (color_depth==16) {
#ifdef BIGENDIAN
           u = Context->ximdata[i+1];
 	   v = Context->ximdata[i];
#else
           u = Context->ximdata[i];
           v = Context->ximdata[i+1];
#endif
           if (t>=0) {
              factor = adjust_dark + (t * (255-adjust_dark))/color_scale;
              r = v>>3;
	      g = ((v&7)<<3) | (u>>5);
	      b = u&31;
	      r = (r * factor)/31;
	      g = (g * factor)/63;
	      b = (b * factor)/31;
	      u = (b&248)>>3 | (g&28)<<3;
	      v = (g&224)>>5 | (r&248);
	   }
#ifdef BIGENDIAN
           Context->xim->data[i+1] = u;
 	   Context->xim->data[i] = v;
#else
	   Context->xim->data[i] = u;
	   Context->xim->data[i+1] = v;
#endif
	} else
	if (color_depth==15) {
#ifdef BIGENDIAN
           u = Context->ximdata[i+1];
 	   v = Context->ximdata[i];
#else
           u = Context->ximdata[i];
 	   v = Context->ximdata[i+1];
#endif
           if (t>=0) {
              factor = adjust_dark + (t * (255-adjust_dark))/color_scale;
              r = v>>2;
	      g = (v&3)<<3 | (u>>5);
	      b = u&31;
	      r = (r * factor)/31;
	      g = (g * factor)/31;
	      b = (b * factor)/31;
	      u = (b&248)>>3 | (g&56)<<2;
	      v = (g&192)>>6 | (r&248)>>1;
	   }
#ifdef BIGENDIAN
           Context->xim->data[i+1] = u;
 	   Context->xim->data[i] = v;
#else
	   Context->xim->data[i] = u;
	   Context->xim->data[i+1] = v;
#endif
	} else {
	   if (t>=0) {
	     if ((7*x+11*y) % color_scale < color_scale-t)
	       Context->xim->data[i] = 
                 Context->nightpixel[(unsigned char)Context->ximdata[i]];
	     else
	       Context->xim->data[i] = Context->ximdata[i];
	   } else
	     Context->xim->data[i] = Context->ximdata[i];
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
	    XDrawLine(dpy, Context->pix, Context->gclist.invert, 0, pline,
		               (orig + npix) - (Context->geom.width + 1), pline);
 	    XDrawLine(dpy, Context->pix, Context->gclist.invert, orig, pline, 
                                              Context->geom.width - 1, pline);
	  } else {
            for (i=0; i<(orig + npix) - Context->geom.width; i++)
	        DarkenPixel(Context, i, pline, -color);
            for (i=orig; i<Context->geom.width; i++)
	        DarkenPixel(Context, i, pline, -color);
	    }
	}
	else {
	  if (invert)
	     XDrawLine(dpy, Context->pix, Context->gclist.invert, orig, pline,
			  orig + (npix - 1), pline);
	  else {
             for (i=orig; i<orig + npix ; i++)
	        DarkenPixel(Context, i, pline, -color);
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
               k = (int) (0.5*(1.0+light)*(color_scale+0.4));
               if (k < 0) k = 0; 
               if (k >= color_scale) k = - 1;
	       DarkenPixel(Context, j, i, k);
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
	       XCopyPlane(dpy, Context->pix, Context->win, Context->gclist.store, 
		    0, 0, Context->geom.width, Context->geom.height, 0, 0, 1);
	   else
               XPutImage(dpy, Context->win, Context->gclist.bigfont, Context->xim, 
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
clearMenu()
{
    if (!mono)
       XSetWindowBackground(dpy, Menu, MenuCaller->pixlist.textbgcolor);
    XClearArea(dpy, Menu,  0, 0, MenuGeom.width, MenuGeom.height, True);
    XSetWindowColormap(dpy, Menu, MenuCaller->cmap);
}

void
initMenu()
{
	char s[2];
	char *ptr[2] = { Label[L_ESCAPE], Label[L_CONTROLS] };
	int i, j, b, d;
	GC gc;

        s[1]='\0';
	d = (5*chwidth)/12;
	for (i=0; i<N_OPTIONS; i++) {
	      b = (Option[2*i+1]==';');
	      for (j=(i+1)*chwidth-b; j<=(i+1)*chwidth+b; j++)
	          XDrawLine(dpy, Menu, MenuCaller->gclist.bigfont, j, 0, j, MapGeom.strip);
	      s[0]=Option[2*i];
	      gc = MenuCaller->gclist.bigfont;
	      if (!MenuCaller->wintype && i>=3 && i<=6) 
                  gc=MenuCaller->gclist.imagefont;
	      XDrawImageString(dpy, Menu, gc, d+i*chwidth, 
                  BigFont->max_bounds.ascent + 3, s, 1);
	}
	for (i=0; i<=1; i++)
	   XDrawImageString(dpy, Menu, MenuCaller->gclist.bigfont, d +(1-i)*N_OPTIONS*chwidth,
                     BigFont->max_bounds.ascent + i*MapGeom.strip + 3, 
		     ptr[i], strlen(ptr[i]));
        XDrawLine(dpy, Menu, MenuCaller->gclist.bigfont, 0, MapGeom.strip, 
                        MENU_WIDTH * MapGeom.strip, MapGeom.strip);
}


void
PopMenu(Context)
struct Sundata * Context;
{
	int    w, h, a, b;
	
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
	   a = Context->geom.y + Context->geom.height + Context->geom.strip + vert_shift;
           b = Context->geom.y - MenuGeom.height - vert_shift - 2*vert_drift;
           if (b < (int) MenuGeom.height ) b = MenuGeom.height;
           if (a > (int) DisplayHeight(dpy,scr) 
                   - 2*MenuGeom.height -vert_drift -20)
              a = b;
	   MenuGeom.y = (placement<=NE)? a : b;              
	}
        setAllHints(NULL, 2);
	clearMenu();
        XMoveWindow(dpy, Menu, MenuGeom.x, MenuGeom.y);
	if (do_selector<=1)
           XMapRaised(dpy, Menu);
	else
           XMapWindow(dpy, Menu);
        XMoveWindow(dpy, Menu, MenuGeom.x, MenuGeom.y);
        XSync(dpy, True);
}

void
clearSelector()
{
    if (!mono)
       XSetWindowBackground(dpy, Selector, SelCaller->pixlist.textbgcolor);
    XClearArea(dpy, Selector,  0,0, SelGeom.width, SelGeom.height, True);
    XSetWindowColormap(dpy, Selector, SelCaller->cmap);
}

void
clearSelectorPartially()
{
    XClearArea(dpy, Selector, 0, MapGeom.strip+1, 
           SelGeom.width-2, SelGeom.height-MapGeom.strip-2, True);
}

void
initSelector()
{
	int i, j, b, d, p, q, h, skip;
	char *s, *sp;
	char *banner[10] = { "home", "share", "  /", "  .", "", "", 
	                     "  K", "  W", "  !", Label[L_ESCAPE]};

	d = chwidth/3;

	if (do_selector==1) {

	  for (i=0; i<=9; i++)
             XDrawImageString(dpy, Selector, SelCaller->gclist.bigfont, d+2*i*chwidth,
                BigFont->max_bounds.ascent + 3, banner[i], strlen(banner[i]));
 
          for (i=1; i<=9; i++) {
	     h = 2*i*chwidth;
             XDrawLine(dpy, Selector, SelCaller->gclist.bigfont, h, 0, h, MapGeom.strip);
	  }

	  /* Drawing small triangular icons */
	  p = MapGeom.strip/4;
	  q = 3*MapGeom.strip/4;
	  h = 9*chwidth;
	  for (i=0; i<=q-p; i++)
	      XDrawLine(dpy,Selector, SelCaller->gclist.bigfont,
                    h-i, p+i, h+i, p+i);
	  h = 11*chwidth;
	  for (i=0; i<= q-p; i++)
	      XDrawLine(dpy,Selector, SelCaller->gclist.bigfont, h-i, q-i, h+i, q-i);

	  h = MapGeom.strip;
          XDrawLine(dpy, Selector, SelCaller->gclist.bigfont, 0, h, SelGeom.width, h);
	}
	
        do_selector = 2;

        XDrawImageString(dpy, Selector, SelCaller->gclist.dirfont,
                d, BigFont->max_bounds.ascent + MapGeom.strip + 3, 
                image_dir, strlen(image_dir));

        h = 2*MapGeom.strip;
        XDrawLine(dpy, Selector, SelCaller->gclist.bigfont, 0, h, SelGeom.width, h);

	dirtable = get_dir_list(image_dir, &num_table_entries);
	if (dirtable)
           qsort(dirtable, num_table_entries, sizeof(char *), dup_strcmp);
	else {
	   char error[] = "Directory inexistent or inaccessible !!!";
           XDrawImageString(dpy, Selector, 
                     SelCaller->gclist.dirfont, d, 3*MapGeom.strip,
		     error, strlen(error));
	   return;
	}

	skip = (4*MapGeom.strip)/5;
	num_lines = (SelGeom.height-2*MapGeom.strip)/skip;
        for (i=0; i<num_table_entries-selector_shift; i++) 
	  if (i<num_lines) {
	  s = dirtable[i+selector_shift];
	  b = (s[strlen(s)-1]=='/');
          if (b==0) {
	    if (strstr(s,".xpm") || strstr(s,".jpg") || strstr(s,".vmf"))
	       b=2;
	  }
	  j = BigFont->max_bounds.ascent + 2 * MapGeom.strip + i*skip + 3;
	  sp = (SelCaller->wintype)? 
	    SelCaller->map_img_file : SelCaller->clock_img_file;
	  if (strstr(sp,s)) {
             XClearArea(dpy, Selector, 2, 
                BigFont->max_bounds.ascent + 2 * MapGeom.strip, 3, 
                SelGeom.height, False);
             XFillRectangle(dpy, Selector, SelCaller->gclist.bigfont,
                                 d/4, j-BigFont->max_bounds.ascent/2, 3, 4);
	  }
          XDrawImageString(dpy, Selector, 
              (b==1)? SelCaller->gclist.dirfont : 
              ((b==2)? SelCaller->gclist.imagefont : 
                       SelCaller->gclist.bigfont),
              d, j, s, strlen(s));
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
	   a = Context->geom.y + Context->geom.height + Context->geom.strip + vert_shift;
           if (do_menu && Context == MenuCaller) 
               a += MenuGeom.height + vert_drift + vert_shift;
           b = Context->geom.y - SelGeom.height - vert_shift - 2*vert_drift;
           if (b < (int) MenuGeom.strip ) b = MenuGeom.height;
           if (a > (int) DisplayHeight(dpy,scr) 
                   - SelGeom.height - vert_drift -20)
              a = b;
	   SelGeom.y = (placement<=NE)? a : b;              
	}

        setAllHints(NULL, 3);
        clearSelector();
        XMoveWindow(dpy, Selector, SelGeom.x, SelGeom.y);
        XMapRaised(dpy, Selector);
        XMoveWindow(dpy, Selector, SelGeom.x, SelGeom.y);
	XSync(dpy, True);
}

void
clearZoom()
{
    if (!mono)
       XSetWindowBackground(dpy, Zoom, ZoomCaller->pixlist.textbgcolor);
    XClearArea(dpy, Zoom,  0,0, ZoomGeom.width, ZoomGeom.height, True);
    XSetWindowColormap(dpy, Zoom, ZoomCaller->cmap);
}

void
initZoom()
{
    char s[]="This will be the zoom command window - still unfinished !  Click to close.";

    XDrawImageString(dpy, Zoom, ZoomCaller->gclist.bigfont, chwidth,
                BigFont->max_bounds.ascent + 3, s, strlen(s));
    XSetWindowBackground(dpy, Zoom, ZoomCaller->pixlist.white);
    XClearArea(dpy, Zoom,  30,30, 200, 100, False);
    XSetWindowBackground(dpy, Zoom, ZoomCaller->pixlist.black);
    XClearArea(dpy, Zoom,  50,50, 40, 20, False);
    if (!mono)
       XSetWindowBackground(dpy, Zoom, ZoomCaller->pixlist.textbgcolor);
    XDrawRectangle(dpy, Zoom, ZoomCaller->gclist.bigfont, 30, 30, 200, 100);
}

void
PopZoom(Context)
struct Sundata * Context;
{
        int a, b, w, h;

	if (do_zoom)
            do_zoom = 0;
	else
	    do_zoom = 1;

        if (!do_zoom) 
	  {
	  XUnmapWindow(dpy, Zoom);
	  if (dirtable) free_dirlist(dirtable);
	  dirtable = NULL;
	  if (Context == ZoomCaller) {
	     ZoomCaller = NULL;
	     return;
	     }
	  do_zoom = 1;
	  }

	ZoomCaller = Context;
	zoom_shift = 0;

	if (!getPlacement(Context->win, &Context->geom.x, &Context->geom.y, &w, &h)) {
	   ZoomGeom.x = Context->geom.x + horiz_shift - horiz_drift;
	   /* + (Context->geom.width - ZoomGeom.width)/2; */
	   a = Context->geom.y + Context->geom.height + Context->geom.strip + vert_shift;
           if (do_menu && Context == MenuCaller) 
               a += MenuGeom.height + vert_drift + vert_shift;
           b = Context->geom.y - ZoomGeom.height - vert_shift - 2*vert_drift;
           if (b < (int) MenuGeom.strip ) b = MenuGeom.height;
           if (a > (int) DisplayHeight(dpy,scr) 
                   - ZoomGeom.height - vert_drift -20)
              a = b;
	   ZoomGeom.y = (placement<=NE)? a : b;              
	}

        setAllHints(NULL, 4);
        clearZoom();
        XMoveWindow(dpy, Zoom, ZoomGeom.x, ZoomGeom.y);
        XMapRaised(dpy, Zoom);
        XMoveWindow(dpy, Zoom, ZoomGeom.x, ZoomGeom.y);
	XSync(dpy, True);
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
	if (num >=3 && num <=5)
	    sprintf(more, " (%s)", Label[L_DEGREE]);
	if (num >=8 && num <=11) {
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
	XDrawImageString(dpy, Menu, MenuCaller->gclist.bigfont, 4, 
              BigFont->max_bounds.ascent + MapGeom.strip + 3, 
              hint, strlen(hint));
}

void
report_failure(path, code)
char *path;
int code;
{
  switch(code) {
    case -1: fprintf(stderr, "%s:\nUnknown image format !!\n", path);
             break;

    case 0:  break;

    case 1:  fprintf(stderr, "Cannot read file %s !!\n", path); 
             break;

    case 2:  fprintf(stderr, "File %s has corrupted data!!\n", path);
             break;

    case 3:  fprintf(stderr, "Cannot decode format specification of %s !!\n", 
             path);
             break;

    case 4:  fprintf(stderr, 
                "Image creation failed (memory alloc. problem?) !!\n");
             break;

    case 5:  fprintf(stderr, "Header of file %s corrupted !!\n", path);
             break;

    case 6:  fprintf(stderr, "Color allocation failed !!\n");
             break;

    case 7:  fprintf(stderr, "Trying instead default  %s\n", path);
             break;

    default:
             fprintf(stderr, "Unknown error in  %s !!\n", path);
             break;
  }
}

#define COMPARE 260
#define QUANTUM 120

void
quantize(Context, image, quantum)
Sundata * Context;
XImage *image;
int quantum;
{
     int i, j, k, l, compare, change;
     unsigned short r[256], g[256], b[256];
     int count[256], done[256];
     char substit[256], value[256];
     int d[COMPARE], v1[COMPARE], v2[COMPARE];
     int size, dist;
     XColor xc;

     if (verbose)
     fprintf(stderr, "Number of distinct colors %d\n"
	             "Quantizing colors and reducing to %d colors.\n", 
                     Context->ncolors, quantum);

     xc.flags = DoRed | DoGreen | DoBlue;

     for(i=0; i<Context->ncolors; i++) {
         count[i] = 0;
	 xc.pixel = Context->daypixel[i];
         XQueryColor(dpy, Context->cmap, &xc);
	 r[i] = xc.red; g[i] = xc.green; b[i] = xc.blue;
     }
     if (Context->cmap != cmap0) XFreeColormap(dpy, Context->cmap);
     
     size = image->bytes_per_line * image->height;
     for (i=0; i<Context->ncolors; i++) {
         substit[i] = (char)i;
	 value[i] = (char)i;
     }

     if (Context->ncolors<=quantum) goto finish;

     for (i=0; i<size; i++) ++count[(unsigned char)image->data[i]];
     
     compare = COMPARE;
     for (i=0; i<compare; i++) d[i] = 2147483647;

     for(i=0; i<Context->ncolors; i++)
        for(j=i+1; j<Context->ncolors; j++) {
            dist = abs((int)(r[i]-r[j]))+abs((int)(g[i]-g[j]))+
                                        +abs((int)(b[i]-b[j]));
	    k=compare-1;
	    while (k>=0 && dist<d[k]) k--;
	    ++k;
	    if (k<compare) {
	      for (l=compare-1; l>k; l--) {
		  d[l] = d[l-1];
                  v1[l] = v1[l-1];
		  v2[l] = v2[l-1];
	      }
	      d[k] = dist;
	      if (count[i]>count[j] || (count[i]==count[j] && i<j)) {
	         v1[k] = i;
	         v2[k] = j;
	      } else {
	         v1[k] = j;
	         v2[k] = i;
	      }
	    }
	}

     l = 0;
     for (i=0; i<compare; i++) {
         j = v1[i];
	 k = v2[i];
         if (substit[k]==(char) k) {
	   substit[k] = j;
	   ++l;
	 }
	 if (l>=Context->ncolors-quantum) break;
     }
     if (verbose)
        fprintf(stderr, 
           "%d substitutions from %d pairs of similar colors\n", l, i);

     change = 1;
     while (change) {
        change = 0;
        l = 0;
        for (i=0; i<Context->ncolors; i++) {
           j = (unsigned char) substit[i];
	   if (substit[i]==(char)i) l++;
           if (substit[j] != (char)j) {
	      substit[i] = substit[j];
	      change = 1;
	   }
        }
     }

  finish:

     private = always_private;

  reattempt:

     if (private) { 
        Context->cmap = 
          XCreateColormap(dpy, RootWindow(dpy, scr), &visual, AllocNone);
	createGCs(Context);
     } else {
        Context->cmap = cmap0;
	Context->gclist = gcl0;
	Context->pixlist = pxl0;
     }

     for (i=0; i<Context->ncolors; i++) done[i] = 0;
     for (i=0; i<256; i++) Context->nightpixel[i] = (unsigned char)i;

     k=0;
     for (i=0; i<Context->ncolors; i++) {
        j = (unsigned char)substit[i];
        if (!done[j]) {
	   xc.red = r[j];
	   xc.green = g[j];
	   xc.blue = b[j];
	   if (!XAllocColor(dpy, Context->cmap, &xc)) {
              color_alloc_failed = 1;
	      value[j] = 0;
	   } else
	      value[j] = (char)xc.pixel;
	   xc.red = (unsigned int) (xc.red * adjust_dark) / 255;
	   xc.green = (unsigned int) (xc.green * adjust_dark) / 255;
	   xc.blue = (unsigned int) (xc.blue * adjust_dark) / 255;
	   if (!XAllocColor(dpy, Context->cmap, &xc)) color_alloc_failed = 1;
	   if (value[j])
	      Context->nightpixel[(unsigned char)value[j]] = (char)xc.pixel;
	   done[j] = 1;
	   k++;
	}
     }

     if (private==0 && color_alloc_failed) {
	color_alloc_failed = 0;
	private = 1;
        goto reattempt;
     }

     if (verbose)
        fprintf(stderr, "%d colors allocated in new colormap\n", 2*k+14);

     for (i=0; i<size; i++)
       image->data[i] = value[(unsigned char)
                              substit[(unsigned char)image->data[i]]];
}

int
createImage(Context)
struct Sundata * Context;
{
   FILE *fd;
   char *file, path[1024]="";
   int code; 

   color_alloc_failed = 0;

   if (!cmap0 && !always_private) {
      Context->cmap = cmap0 = DefaultColormap(dpy, scr);
      createGCs(Context);
      pxl0 = Context->pixlist;
      gcl0 = Context->gclist;
      if (color_depth<=8 && color_alloc_failed) {
	 always_private = 1;
         color_alloc_failed = 0;
      }
   }
   private = always_private;

   if (color_depth>8 || (invert && !private)) {
      Context->cmap = cmap0;
      Context->pixlist = pxl0;
      Context->gclist = gcl0;
   } else
      Context->cmap =
           XCreateColormap(dpy, RootWindow(dpy, scr), &visual, AllocNone);

   file = (Context->wintype)? Context->map_img_file : Context->clock_img_file;

 do_path:

   Context->xim = NULL;
   code = -1;

   if (file) {
      strcpy(path, file);
      if (*file != '/' && *file != '.' ) {
         if ((fd=fopen(file, "r"))) {
	    fclose(fd);
	 } else {
	    if (verbose)
	      fprintf(stderr, "%s not in current directory ...\n"
		      "Trying to load %s from share directory instead\n", 
                      file, file);
            sprintf(path, "%s%s", share_maps_dir, file);
	 }
      }
   }

   if (*path && strcmp(path, Default_vmf)) {
      if ((fd = fopen(path, "r"))) 
	 fclose(fd);
      else {
	 file = Default_vmf;
         fprintf(stderr, "File %s doesn't seem to exist !!\n"
                         "Trying default  %s\n",
                         path, file);
	 goto do_path;
      }
   }

   if (Context->wintype) {
     if (Context->map_img_file && file!=Context->map_img_file) {
       free(Context->map_img_file);
       Context->map_img_file = RCAlloc(file);
     }
   } else {
     if (Context->clock_img_file && file!=Context->clock_img_file) {
       free(Context->clock_img_file);
       Context->clock_img_file = RCAlloc(file);
     }
   }

   if (invert) {
     XGCValues		gcv;
     if (Context->cmap!=cmap0) createGCs(Context);
   retry:
     readVMF(path, Context);
     if (Context->bits) {
       Context->pix = XCreatePixmapFromBitmapData(dpy, RootWindow(dpy, scr),
          Context->bits, Context->geom.width,
          Context->geom.height, 0, 1, 1);
       gcv.background = white;
       gcv.foreground = black;
       gcv.function = GXinvert;
       Context->gclist.invert = XCreateGC(dpy, Context->pix, 
               GCForeground | GCBackground | GCFunction, &gcv);
       free(Context->bits);
       if (color_alloc_failed) report_failure(path, 6);
       return 0;
     } else {
       if (strcmp(path, Default_vmf)) {
	  report_failure(path, 1);
	  strcpy(path, Default_vmf);
	  report_failure(path, 7);
	  goto retry;
       }
       report_failure(path, 1);
       return 1;
     }
   }

   if (strstr(path, ".jpg"))
     code = readJPEG(path, Context);
   else
   if (strstr(path, ".vmf"))
     code = readVMF(path, Context);
   else
   if (strstr(path, ".xpm"))
     code = readXPM(path, Context);

   if (code) {
      report_failure(path, code);
      if (strcmp(path, Default_vmf)) {
         file = Default_vmf;
	 report_failure(file, 7);
	 goto do_path;
      }
   }

   if (color_depth<=8) {
      quantize(Context, Context->xim, QUANTUM);
      if (private && !strcmp(path, Default_vmf)) always_private = 1;
      if (color_alloc_failed) {
	 code = 6;
	 XDestroyImage(Context->xim);
	 Context->xim = 0;
      }
   }

   return code;
}

void 
createWorkImage(Context)
struct Sundata * Context;
{
   int size;

   if (Context->xim) {
     /* Context->flags.alloc_level = (Context->flags.shading>=2)?color_scale:1;
	setDarkPixels(Context, 0, Context->flags.alloc_level); */
     size = Context->xim->bytes_per_line*Context->xim->height;
     if (verbose)
        fprintf(stderr, "Creating work image data of size %d bytes\n", size);
     Context->ximdata = (char *)salloc(size);
     memcpy(Context->ximdata, Context->xim->data, size); 
   }
}

void
resetAuxilWins(Context)
Sundata * Context;
{
      if (do_menu && Context == MenuCaller) {
	 clearMenu();
	 initMenu();
      }
      if (do_selector && Context == SelCaller) { 
         do_selector = 1; 
         clearSelector();
	 initSelector();
      }
      if (do_zoom && Context == ZoomCaller) { 
         do_zoom = 1; 
         clearSelector();
	 initSelector();
      }
}

void
remapAuxilWins(Context)
struct Sundata * Context;
{
      if (do_menu || do_selector || do_zoom) {
	 XFlush(dpy);
	 usleep(TIMESTEP);
      }

      if (do_menu) {
	 do_menu = 0;
   	 PopMenu(Context);
         initMenu();
      }

      if (do_selector) {
	 do_selector = 0;
	 PopSelector(Context);
	 do_selector = 1;
	 initSelector();
      }

      if (do_zoom) {
	 do_zoom = 0;
	 PopZoom(Context);
	 do_zoom = 1;
	 initZoom();
      }
}

void checkLocation();

void 
buildMap(Context, wintype, build)
struct Sundata * Context;
int wintype, build;
{
   Window win;

   if (build) {
      struct Sundata * NewContext;
      NewContext = (struct Sundata *)salloc(sizeof(struct Sundata));
      NewContext->next = NULL;
      if (Context) {
	if (Context->next) {
	    NewContext->next = Context->next;
            Context->next = NewContext;
	} else
            Context->next = NewContext;
      } else
         Seed = NewContext;
      Context = NewContext;
      Context->wintype = wintype;
      if (do_selector<0) {
	 do_selector = 1;
	 SelCaller = Context;
      }
   }
   makeContext(Context, build);
   win = Context->win;
   if (win) {
      XSelectInput(dpy, Context->win, 0);
      XSync(dpy, True);
   } else
      createWindow(Context, wintype);

   if (createImage(Context)) {
     if (Seed->next) {
         shutDown(Context, 0);
	 exit(0);
	 Context = Seed;
         return;
     } else
         shutDown(Context, -1);
   }
   checkGeom(Context, 0);
   if (win) {
      setAllHints(Context, wintype);
      if (Context->wintype)
         XMoveWindow(dpy, Context->win, Context->geom.x,Context->geom.y);
      XResizeWindow(dpy, Context->win, 
          Context->geom.width, Context->geom.height + Context->geom.strip);
      if (!Context->wintype)
         XMoveWindow(dpy, Context->win, Context->geom.x,Context->geom.y);
      checkLocation(Context, CityInit);
      XSync(dpy, True);
      createWorkImage(Context);
      if (private) resetAuxilWins(Context);
      XSync(dpy, True);
      setProtocols(Context, Context->wintype);
   } else {
      createWorkImage(Context);
      checkLocation(Context, CityInit);
      setAllHints(Context, wintype);
      setProtocols(Context, wintype);
      XMapWindow(dpy, Context->win);
      XSync(dpy, True);
      usleep(2*TIMESTEP);
      if (do_title) 
        setClassHints(Context->win, wintype);
   }
   if (build)
      Context->prevgeom = Context->geom;
   if (Context->wintype)
      Context->mapgeom = Context->geom;
   else
      Context->clockgeom = Context->geom;
   if (private)
      XSetWindowColormap(dpy, Context->win, Context->cmap);
   if (do_menu) {
      clearMenu();
      initMenu();
   }
   if (do_selector) {
      do_selector = 1;
      clearSelector();
      initSelector();
   }
   clearStrip(Context);
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

	/* fprintf(stderr, "Test: <%c> %d %u\n", key, key, key); */

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
	   clearSelectorPartially();
	   return;
	}

        if (win==Zoom)
	   return;

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
	   case '=':
	     do_sync = 1 - do_sync;
	     break;
	   case 'ª':
	   case '*':
             i = Context->geom.width/2;
	     if (i>0 && Context->geom.height!=i) {
                Context->geom.height = i;
	        if (Context->wintype)
	              MapGeom.height = i;
	        else
	              ClockGeom.height = i;
                adjustGeom(Context, 0);
                shutDown(Context, 0);
	        buildMap(Context, Context->wintype, 0);
	        remapAuxilWins(Context);
	        Context->flags.resized = 0;
	     }
	     break;
	   case ' ':
	   case '!':
	     if (Context==Seed && do_dock) return;
             Context->wintype = 1 - Context->wintype;
             if (Context->wintype)
                Context->geom = MapGeom;
             else
                Context->geom = ClockGeom;
	     adjustGeom(Context, 1);
             shutDown(Context, 0);
             buildMap(Context, Context->wintype, 0);
	     remapAuxilWins(Context);
	     return;
	   case '­':
	   case '-':
	     Context->jump = 0;
	     Context->flags.force_proj = 1;
	     Context->flags.last_hint = -1;
	     break;
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
	     if (!Context->wintype) break;
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
	     if (!Context->wintype) break;
	     if (Context->flags.map_mode != DISTANCES) 
	       Context->flags.dms = gflags.dms;
	     else
	       Context->flags.dms = 1 - Context->flags.dms;
	     Context->flags.map_mode = DISTANCES;
	     break;
	   case 'e': 
	     if (!Context->wintype) break;
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
	     if (!Context->wintype) {
	        clearStrip(Context);
	        if (!Context->wintype)
                   Context->flags.clock_mode = 
                     (Context->flags.clock_mode+1) % num_formats;
	        updateMap(Context);
                break;
	     }
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
		for (i = 0; i < Context->geom.height; i++)
		    Context->cwtab[i] = Context->geom.width;
                size = Context->xim->width*Context->xim->height*(Context->xim->bitmap_pad/8);
                memcpy(Context->xim->data, Context->ximdata, size); 
	     }
	     Context->flags.force_proj = 1;
	     exposeMap(Context);
	     break;
	   case 'o':
             Context->flags.sunpos = 1 - Context->flags.sunpos;
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
             /* if (do_menu) PopMenu(Context); */
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
	     if (!do_zoom)
	        PopZoom(Context);
	     else
                RaiseAndFocus(Zoom);
	     break;
           default:
	     if (!Context->wintype) {
	       Context->flags.clock_mode = 
                   (1+Context->flags.clock_mode) % num_formats;
	       updateMap(Context);
	     }
	     break ;
	}

        if (old_mode == EXTENSION && Context->flags.map_mode != old_mode) 
             clearStrip(Context);

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

void refreshPopups(Context, w, h)
struct Sundata * Context;
unsigned int w, h;
{
	int i;

        if (w!=Context->geom.width || h!=Context->geom.height) {
	   XFlush(dpy);
	   usleep(3*TIMESTEP);
           if (do_menu) for (i=0; i<=1; i++) PopMenu(Context);
	   XFlush(dpy);
	   usleep(TIMESTEP);
           if (do_selector) for (i=0; i<=1; i++) PopSelector(Context);
	   XFlush(dpy);
	   usleep(2*TIMESTEP);
        }
        if (do_selector) {
           RaiseAndFocus(Selector);
	   if (private) 
              XSetWindowColormap(dpy, Selector, SelCaller->cmap);
        }
}

void 
processSelAction(Context, a, b)
struct Sundata * Context;
int a;
int b;
{
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
	  if (a==6) {	
              processKey(SelCaller->win, 'k');
	      return;
	  }
	  if (a==7) {	
	      do_selector = -1;
              processKey(SelCaller->win, 'w');
	      return;
	  }
	  if (a==8) {	
              processKey(SelCaller->win, '!');
	      return;
	  }
	  if (a>=9) {
	     XUnmapWindow(dpy, Selector);
	     do_selector = 0;
	     return;
	  }
	  clearSelectorPartially();
	  return;
	}
        if (b <= 2*MapGeom.strip) {
	  selector_shift = 0;
	  clearSelectorPartially();
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
              clearSelectorPartially();
	      return;
	   } else {
	      char *f, *path;
	      unsigned int w, h;
	      path = (Context->wintype)? 
                    Context->map_img_file : Context->clock_img_file;
	      f = (char *)
                salloc((strlen(image_dir)+strlen(s)+2)*sizeof(char));
	      sprintf(f, "%s%s", image_dir, s);
	      if (!path || strcmp(f, path)) {
 		 if (Context->wintype) {
                    free(Context->map_img_file);
		    Context->map_img_file = NULL;
		 } else {
                    free(Context->clock_img_file);
		    Context->clock_img_file = NULL;
		 }
		 adjustGeom(Context, 0);
	         shutDown(Context, 0);
		 if (Context->wintype) 
                    Context->map_img_file = f;
		 else
                    Context->clock_img_file = f;
		 w = Context->geom.width;
		 h = Context->geom.height;
	         buildMap(Context, Context->wintype, 0);
                 /* refreshPopups(Context, w, h); */
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
	   processKey(MenuCaller->win, key);
	   return;
	}

        if (win == Selector) {
	   processSelAction(SelCaller, x, y);
	   return;
	}

        if (win == Zoom) {
	   PopZoom(ZoomCaller);
	   return;
	}

	Context = getContext(win);
	if (!Context) return;
 
        /* Click on bottom strip of window */
        if (y >= Context->geom.height) {
	   if (button==1) {
	      PopMenu(Context);
              return;
	   }
           if (button==2) {
	        processKey(win, 'l');
		return;
	   }
           if (button==3) {
	        /* Open new window */
	        processKey(win, 'w');
	        return;
	   }
	}

        /* Click on the map */
	if (button==2) {
	   PopSelector(Context);
	   return;
        }
	if (button==3) {
	   /* Open zoom selector */
	   PopZoom(Context);
	   return;
        } 
	
        /* Click with button 1 on the map */

	/* It's a clock, just execute predefined command */
	if (!Context->wintype) {
	   processKey(win, 'x');
	   return;
	}

        /* Otherwise, user wants to get info on a city or a location */
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
	      if (verbose)
	         fprintf(stderr, "Resizing Selector to %d %d\n", w, h);
	      XSelectInput(dpy, Selector, 0);
	      XSync(dpy, True);
	      XResizeWindow(dpy, Selector, w, h);
	      XSync(dpy, True);
	      usleep(TIMESTEP);
              setAllHints(NULL,3);
              setProtocols(NULL,3);
	      do_selector = 1;
	      initSelector();
	      return;
	   }

	   if (win == Menu) return;

           Context = getContext(win);
	   if(!Context) return;

           if (Context==Seed && !Context->wintype && do_dock) return;

	   if (getPlacement(win, &x, &y, &w, &h)) return;
           Context->prevgeom = Context->geom;
           h -= Context->geom.strip;
           if (w==Context->geom.width && h==Context->geom.height) return;
	   if (w<Context->geom.w_mini) w = Context->geom.w_mini;
	   if (h<Context->geom.h_mini) h = Context->geom.h_mini;
	   Context->geom.width = w;
	   Context->geom.height = h;
	   if (Context->wintype) {
	      MapGeom.width = w;
	      MapGeom.height = h;
	   } else {
	      ClockGeom.width = w;
	      ClockGeom.height = h;
	   }
           adjustGeom(Context, 0);
           shutDown(Context, 0);
	   buildMap(Context, Context->wintype, 0);
	   Context->flags.resized = 1;
	   remapAuxilWins(Context);
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

	if (Context->count==0) {
		updateImage(Context);
		showImage(Context);
                if (mono) pulseMarks(Context);
	}
}

void
doExpose(w)
register Window	w;
{
        struct Sundata * Context = (struct Sundata *)NULL;

        if (w == Menu)
	   initMenu(Context);

        if (w == Selector)
           initSelector(Context);

        if (w == Zoom)
           initZoom(Context);

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
	Sundata * 		Context;
	int			key;

	for (;;) {
		if (XCheckIfEvent(dpy, &ev, evpred, (char *)0))

			switch(ev.type) {
		
			case VisibilityNotify:
			        doExpose(ev.xexpose.window);
				break;

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
					if (ev.xexpose.window == Zoom)
					   PopSelector(ZoomCaller);
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
			        Sundata *Which = getContext(ev.xexpose.window);
			        usleep(TIMESTEP);
				for (Context=Seed; Context; 
                                                   Context=Context->next)
				if (do_sync || Context == Which ||
                                    (do_dock && Context == Seed))
				   doTimeout(Context);
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
		return;
            }

	if (Context->mark1.city == &Context->pos1) {
		Context->flags.map_mode = SOLARTIME;
		Context->mark1.city = NULL;
		setTZ(NULL);
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
	int    i, rem_menu;

	ProgName = *argv;

	/* Set default values */
        initValues();

	/* Read the configuation file */

        checkRCfile(argc, argv);
	if (readrc()) exit(1);

	if ((p = strrchr(ProgName, '/'))) ProgName = ++p;

	leave = 1;
	(void) parseArgs(argc, argv);
	leave = 0;

	dpy = XOpenDisplay(Display_name);
	if (dpy == (Display *)NULL) {
		fprintf(stderr, "%s: can't open display `%s'\n", ProgName,
			Display_name);
		exit(1);
	}

	scr = DefaultScreen(dpy);
        color_depth = DefaultDepth(dpy, scr);
        visual = *DefaultVisual(dpy, scr);
	black = BlackPixel(dpy, scr);
	white = WhitePixel(dpy, scr);

        if (color_depth == 16 && visual.green_mask==992) color_depth = 15;
        if (color_depth > 16)
            color_pad = 32;
        else if (color_depth > 8)
            color_pad = 16;
        else
            color_pad = 8;

        if (verbose) {
           fprintf(stderr, "%s: version %s, %s\nDepth %d    Bits/RGB %d\n"
                    "Red mask %ld    Green mask %ld    Blue mask %ld\n", 
                ProgName, VERSION, COPYRIGHT,
                color_depth, visual.bits_per_rgb, 
                visual.red_mask, visual.green_mask, visual.blue_mask);
	}

        wm_protocols = XInternAtom(dpy, "WM_PROTOCOLS", False);
        wm_delete_window = XInternAtom(dpy, "WM_DELETE_WINDOW", False);

        /* Correct some option parameters */
	if (placement<0) placement = NW;
	if (invert && (gflags.shading>=2)) gflags.shading = 1;
	if (color_depth>8 || mono) private = always_private = 0;
	adjust_dark = (unsigned int) ((1.0-darkness) * 255.25);
	rem_menu = do_menu;
	do_menu = 0;
	parseFormats(ListFormats);

	getFonts();


        buildMap(NULL, win_type, 1);

	for (i=2; i<=4; i++) {
	   createWindow(NULL, i);
	   setProtocols(NULL, i);
	   setAllHints(NULL, i);
	}

	if (rem_menu)
	   PopMenu(Seed);
	eventLoop();
	exit(0);
}
