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
    program.  Please leave the original attribution information intact  so
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
       3.32  03/19/01  Implementation of the zoom widget
       3.34  03/28/01  Vast improvements in the zoom widget
*/

#include <unistd.h>
#include <sys/types.h>
#include <sys/timeb.h>
#include <sys/stat.h>
#include <string.h>

#define DEFVAR
#include "sunclock.h"
#include "langdef.h"

/* 
 *  external routines
 */

extern long     time();
#ifdef NEW_CTIME
extern char *   timezone();
#endif

extern double   jtime();
extern double   gmst();
extern void     sunpos();
extern long     jdate();

extern char *   salloc();
extern int      readVMF();
extern int      readJPEG();
extern int      readXPM();

extern void free_dirlist();

extern char *tildepath();       /* Returns path to ~/<file> */

extern int getPlacement();
extern void checkGeom();
extern void adjustGeom();
extern void setSizeHints();
extern void setClassHints();
extern void shutDown();
extern void setProtocols();
extern void createWindow();
extern void destroyGCs();

extern void setupMenu();
extern void PopMenu();
extern void showMenuHint();

extern void setupSelector();
extern void PopSelector();
extern void processSelectorAction();

extern void checkZoomSettings();
extern void setZoomDimension();
extern void setZoomAspect();
extern void setupZoom();
extern void PopZoom();
extern void processZoomAction();
extern void activateZoom();

extern void setupOption();
extern void PopOption();
extern void activateOption();
extern void processOptionAction();

extern void resetAuxilWins();
extern void remapAuxilWins();
extern void RaiseAndFocus();

void doTimeout();

char share_i18n[] = SHAREDIR"/i18n/Sunclock.**";
char app_default[] = SHAREDIR"/Sunclockrc";
char Default_vmf[] = SHAREDIR"/earthmaps/vmf/standard.vmf";

char *share_maps_dir = SHAREDIR"/earthmaps/";
char *Clock_img_file;
char *Map_img_file;

char image_dir[1024];

char *SmallFont_name;
char *BigFont_name;

char **dirtable = NULL;
char * freq_filter = "";

char language[4] = "";

char *  rc_file = NULL;
char *  ProgName;
char *  ExternAction = NULL;

char    StdFormats[] = STDFORMATS;
char *  ListFormats = StdFormats;
char ** DateFormat;

Pixmap zoompix = 0;

struct Sundata *Seed, *MenuCaller, *SelCaller, *ZoomCaller, *OptionCaller;

char    TextBgColor[COLORLENGTH], TextFgColor[COLORLENGTH], 
        MapBgColor[COLORLENGTH], MapFgColor[COLORLENGTH], 
        ZoomBgColor[COLORLENGTH], ZoomFgColor[COLORLENGTH], 
        DirColor[COLORLENGTH], ImageColor[COLORLENGTH],
        ChangeColor[COLORLENGTH], ChoiceColor[COLORLENGTH],
        CityColor0[COLORLENGTH], CityColor1[COLORLENGTH], 
        CityColor2[COLORLENGTH],
        MarkColor1[COLORLENGTH], MarkColor2[COLORLENGTH], 
        LineColor[COLORLENGTH], MeridianColor[COLORLENGTH],
        ParallelColor[COLORLENGTH], TropicColor[COLORLENGTH], 
        SunColor[COLORLENGTH], MoonColor[COLORLENGTH];

char *          Display_name = "";
char *          CityInit = NULL;

Display *       dpy;
Visual          visual;
Colormap        cmap0, tmp_cmap;

Flags           gflags;
ZoomSettings    gzoom;

int             scr;
int             bigendian;
int             color_depth;
int             color_pad;
int             bytes_per_pixel;
int             total_colors;

int             runtime = 0;
int             button_pressed = 0;
int             control_key = 0;

int             zoom_mode = 0;
int             zoom_active = 1;

int             option_caret;
int             old_option_caret = -1;
int             old_option_length;

KeySym          menu_newhint;
char            option_newhint;
char            zoom_newhint;

char            menu_lasthint;
char            option_lasthint;
char            zoom_lasthint;

char            old_option_string_char;
char *          option_string = 0;

Pixel           black, white;

Atom            wm_delete_window, wm_protocols;

XFontStruct     *SmallFont, *BigFont;

Window          Menu = 0, Selector = 0, Zoom = 0, Option = 0;

struct Geometry MapGeom    = { 0, 30,  30, 792, 396, 320, 160 };
struct Geometry ClockGeom  = { 0, 30,  30, 128,  64,  48,  24 };
struct Geometry MenuGeom   = { 0, 30, 430, 792,  40, 700,  40 };
struct Geometry SelGeom    = { 0, 30,  40, 600, 180, 450,  80 };
struct Geometry ZoomGeom   = { 0, 30,  40, 500, 320, 360, 250 };
struct Geometry OptionGeom = { 0, 30,  40, 640,  80, 640,  80 };

int             radius[5] = {0, 2, 2, 3, 5};
int             map_strip, clock_strip;

int             win_type = 0;
int             placement = -1;
int             color_alloc_failed = 0;
int             num_formats;
int             leave;
int             verbose = 0;
int             chwidth;
int             num_lines;
int             num_table_entries;

int             horiz_shift = 0;
int             vert_shift = 12;
int             label_shift = 0;
int             selector_shift = 0;
int             zoom_shift = 0;

int             horiz_drift = 0;
int             vert_drift =  0;

int             do_menu = 0;
int             do_selector = 0;
int             do_zoom = 0;
int             do_option = 0;

int             do_hint = 0;
int             do_dock = 0;
int             do_sync = 0;
int             do_zoomsync = 0;
int             do_title = 1;

int             time_jump = 0;

long            last_time = 0;
long            progress_value[6] = { 60, 3600, 86400, 604800, 2592000, 0 };

double          darkness = 0.5;
double          atm_refraction = ATM_REFRACTION;
double          atm_diffusion = ATM_DIFFUSION;
double          *daywave, *cosval, *sinval;    /* pointers to trigo values */

/* Records to hold extra marks 1 and 2 */

City position, *cities = NULL;

void
usage()
{
     fprintf(stderr, "%s: version %s, %s\nUsage:\n"
     "%s [-help] [-listmenu] [-version] [-verbose] [-title] [-notitle]\n"
     SP"[-display name] [-rcfile file] [-sharedir directory]\n"
     SP"[-clock] [-clockgeom <geom>] [-clockimage file]\n"
     SP"[-map] [-mapgeom <geom>] [-mapimage file] [-mapmode * <L,C,S,D,E>]\n"
     SP"[-menu] [-nomenu] [-horizshift h (map<->menu)] [-vertshift v]\n"
     SP"[-selector] [-noselector] [-selgeom <geom>] [-synchro] [-nosynchro]\n" 
     SP"[-zoom] [-nozoom] [-zoomgeom <geom>] [-zoomsync] [-nozoomsync]\n"
"**       [-language name] [-bigfont name] [-smallfont name]\n"
     SP"[-dateformat string1|string2|...] [-command string]\n"
     SP"[-fullcolors] [-colorinvert] [-monochrome] [-aspect mode]\n"
     SP"[-placement (random, fixed, center, NW, NE, SW, SE)] [-dock]\n"
     SP"[-decimal] [-dms] [-city name] [-position latitude longitude]\n"
     SP"[-jump number[s,m,h,d,M,Y]] [-progress number[s,m,h,d,M,Y]]\n"
     SP"[-nonight] [-night] [-terminator] [-twilight] [-luminosity]\n"
     SP"[-shading mode=0,1,2,3,4] [-diffusion value] [-refraction value]\n"
     SP"[-darkness value<=1.0] [-colorscale number<=256]\n"
     SP"[-coastlines] [-contour] [-landfill] [-fillmode number=0,1,2]\n"
     SP"[-mag value] [-magx value] [-magy value] [-dx value ] [-dy value]\n"
     SP"[-sun] [-nosun] [-cities] [-nocities] [-meridians] [-nomeridians]\n"
     SP"[-parallels] [-noparallels] [-tropics] [-notropics]\n"
     SP"[-spotsize size(0,1,2,3,4)] [-dottedlines] [-plainlines]\n"
     SP"[-textbgcolor color] [-textfgcolor color]\n"
     SP"[-mapbgcolor color] [-mapfgcolor color]\n"
     SP"[-zoombgcolor color] [-zoomfgcolor color]\n"
     SP"[-changecolor color] [-choicecolor color]\n"
     SP"[-dircolor color] [-imagecolor color]\n"
     SP"[-linecolor color] [-suncolor color] [-mooncolor color]\n"
     SP"[-meridiancolor color] [-parallelcolor] [-tropiccolor color]\n"
     SP"[-citycolor0 color] [-citycolor1 color] [-citycolor2 color]\n"
     SP"[-markcolor1 color] [-markcolor2 color]\n\n%s\n\n",
        ProgName, VERSION, COPYRIGHT, ProgName, Configurability);
}

void
ListMenu()
{
        int i;

        fprintf(stderr, "%s\n", ShortHelp);
        for (i=0; i<N_HELP; i++)
        fprintf(stderr, "%s %c : %s\n", Label[L_KEY], CommandKey[i], Help[i]);
        fprintf(stderr, "\n");
}

void
initValues()
{
        gflags.mono = 0;
        gflags.fillmode = 2;
        gflags.dotted = 0;
        gflags.spotsize = 3;
        gflags.colorscale = 16;

        gflags.update = -1;
        gflags.firsttime = 1;
        gflags.progress = 0;
        gflags.bottom = 0;
        gflags.map_mode = LEGALTIME;
        gflags.clock_mode = 0;
        gflags.hours_shown = 0;
        gflags.dms = 0;
        gflags.shading = 2;
        gflags.cities = 1;
        gflags.sunpos = 1;
        gflags.meridian = 0;
        gflags.parallel = 0;
        gflags.tropics = 0;

        gzoom.mode = 0;
        gzoom.fx = 1.0;
        gzoom.fy = 1.0;
        gzoom.fdx = 0.5;
        gzoom.fdy = 0.5;

        strcpy(image_dir, share_maps_dir);
        Clock_img_file = Default_vmf;
        Map_img_file = Default_vmf;

        strcpy(TextBgColor, "Grey92");
        strcpy(TextFgColor, "Black");
        strcpy(MapBgColor, "White");
        strcpy(MapFgColor, "Black");
        strcpy(ZoomBgColor, "White");
        strcpy(ZoomFgColor, "Black");
        strcpy(DirColor, "Blue");
        strcpy(ChangeColor, "Brown");
        strcpy(ChoiceColor, "SkyBlue2");
        strcpy(ImageColor, "Magenta");
        strcpy(CityColor0, "Orange");
        strcpy(CityColor1, "Red");
        strcpy(CityColor2, "Red3");
        strcpy(MarkColor1, "Pink1");
        strcpy(MarkColor2, "Pink2");
        strcpy(LineColor, "White");
        strcpy(MeridianColor, "White");
        strcpy(ParallelColor, "White");
        strcpy(TropicColor, "White");
        strcpy(SunColor, "Yellow");
        strcpy(MoonColor, "Khaki");

        position.lat = 100;
        position.tz = NULL;

        BigFont_name = (char *)salloc((strlen(BIGFONT)+1)*sizeof(char));
	strcpy(BigFont_name, BIGFONT);
        SmallFont_name = (char *)salloc((strlen(SMALLFONT)+1)*sizeof(char));
	strcpy(SmallFont_name, SMALLFONT);	
}

void
needMore(argc, argv)
register int                    argc;
register char **                argv;
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
register char *                 s;
register struct Geometry *      g;
{
        register int            mask;

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

int setWindowAspect(Context)
struct Sundata * Context;
{
   unsigned int b, i, j;
   int change;
   struct Geometry *       Geom;
   double f;

   if (Context->newzoom.mode <= 1)
      f = 1.0;
   else
      f = sin(M_PI*Context->newzoom.fdy);

   change = 0;
   i = Context->geom.width;

   j = (unsigned int) (0.5 + Context->newzoom.fx * 
          (double)Context->geom.width / ( 2.0 * Context->newzoom.fy * f) );
   b = DisplayHeight(dpy,scr) - Context->hstrip - vert_drift - 5;
   if (j<=0) j=1;
   if (j>b) {
      i = (Context->geom.width * b) / j;
      j = b;
   }

   Geom = (Context->wintype)? &MapGeom : &ClockGeom;

   if (j<Geom->h_mini) {
      i = (i * Geom->h_mini)/j;
      j = Geom->h_mini;
   }
  
   if (Context->geom.width != i) {
      change = 1;
      Context->geom.width = Geom->width = i;
   }
   if (Context->geom.height != j) {
      change = 1;
      Context->geom.height = Geom->height = j;
   }

   if (change && verbose)
      fprintf(stderr, "Resizing window to width = %d , height = %d\n", i, j); 

   return change;
}

void
checkRCfile(argc, argv)
register int                    argc;
register char **                argv;
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
register int                    argc;
register char **                argv;
{
        int     opt;

        while (--argc > 0) {
                ++argv;
                if (runtime) goto configurable;
                if (strcasecmp(*argv, "-display") == 0) {
                        needMore(argc, argv);
                        Display_name = RCAlloc(*++argv); 
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
                else if (strcasecmp(*argv, "-selgeom") == 0) {
                        needMore(argc, argv);
                        getGeom(*++argv, &SelGeom);
                        --argc;
                }
                else if (strcasecmp(*argv, "-zoomgeom") == 0) {
                        needMore(argc, argv);
                        getGeom(*++argv, &ZoomGeom);
                        --argc;
                }
                else if (strcasecmp(*argv, "-mapgeom") == 0) {
                        needMore(argc, argv);
                        getGeom(*++argv, &MapGeom);
                        --argc;
                }
                else if (strcasecmp(*argv, "-mag") == 0) {
                        needMore(argc, argv);
                        gzoom.fx = atof(*++argv);
                        if (gzoom.fx < 1) gzoom.fx = 1.0;
                        if (gzoom.fx > 100.0) gzoom.fx = 100.0;
                        gzoom.fy = gzoom.fx;
                        --argc;
                }
                else if (strcasecmp(*argv, "-magx") == 0) {
                        needMore(argc, argv);
                        gzoom.fx = atof(*++argv);
                        if (gzoom.fx < 1) gzoom.fx = 1.0;
                        --argc;
                }
                else if (strcasecmp(*argv, "-magy") == 0) {
                        needMore(argc, argv);
                        gzoom.fy = atof(*++argv);
                        if (gzoom.fy < 1) gzoom.fy = 1.0;
                        --argc;
                }
                else if (strcasecmp(*argv, "-dx") == 0) {
                        needMore(argc, argv);
                        gzoom.fdx = atof(*++argv)/360.0-0.5;
                        checkZoomSettings(&gzoom);
                        --argc;
                }
                else if (strcasecmp(*argv, "-dy") == 0) {
                        needMore(argc, argv);
                        gzoom.fdy = 0.5-atof(*++argv)/180.0;
                        checkZoomSettings(&gzoom);
                        --argc;
                }
                else 
                  configurable:
                     if (strcasecmp(*argv, "-language") == 0) {
                        needMore(argc, argv);
                        strncpy(language, *++argv, 2);
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
                else if (strcasecmp(*argv, "-mapmode") == 0) {
                        needMore(argc, argv);
                        if (!strcasecmp(*++argv, "c")) 
                           gflags.map_mode = COORDINATES;
                        if (!strcasecmp(*argv, "d")) 
                           gflags.map_mode = DISTANCES;
                        if (!strcasecmp(*argv, "e")) 
                           gflags.map_mode = EXTENSION;
                        if (!strcasecmp(*argv, "l")) {
                           CityInit = NULL;
                           gflags.map_mode = LEGALTIME;
                        }
                        if (!strcasecmp(*argv, "s")) 
                           gflags.map_mode = SOLARTIME;
                        --argc;
                }
                else if (strcasecmp(*argv, "-spotsize") == 0) {
                        needMore(argc, argv);
                        gflags.spotsize = atoi(*++argv);
                        if (gflags.spotsize<0) gflags.spotsize = 0;
                        if (gflags.spotsize>4) gflags.spotsize = 4;
                        --argc;
                }
                else if (strcasecmp(*argv, "-fillmode") == 0) {
                        needMore(argc, argv);
                        gflags.fillmode = atoi(*++argv);
                        if (gflags.fillmode<0) gflags.fillmode = 0;
                        if (gflags.fillmode>3) gflags.fillmode = 3;
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
                        gflags.colorscale = atoi(*++argv);
                        if (gflags.colorscale<=0) gflags.colorscale = 1;
                        if (gflags.colorscale>256) gflags.colorscale = 256;
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
                else if (strcasecmp(*argv, "-mapbgcolor") == 0) {
                        needMore(argc, argv);
                        strncpy(MapBgColor, *++argv, COLORLENGTH);
                        --argc;
                }
                else if (strcasecmp(*argv, "-mapfgcolor") == 0) {
                        needMore(argc, argv);
                        strncpy(MapFgColor, *++argv, COLORLENGTH);
                        --argc;
                }
                else if (strcasecmp(*argv, "-zoombgcolor") == 0) {
                        needMore(argc, argv);
                        strncpy(ZoomBgColor, *++argv, COLORLENGTH);
                        --argc;
                }
                else if (strcasecmp(*argv, "-zoomfgcolor") == 0) {
                        needMore(argc, argv);
                        strncpy(ZoomFgColor, *++argv, COLORLENGTH);
                        --argc;
                }
                else if (strcasecmp(*argv, "-changecolor") == 0) {
                        needMore(argc, argv);
                        strncpy(ChangeColor, *++argv, COLORLENGTH);
                        --argc;
                }
                else if (strcasecmp(*argv, "-choicecolor") == 0) {
                        needMore(argc, argv);
                        strncpy(ChoiceColor, *++argv, COLORLENGTH);
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
                else if (strcasecmp(*argv, "-meridiancolor") == 0) {
                        needMore(argc, argv);
                        strncpy(MeridianColor, *++argv, COLORLENGTH);
                        --argc;
                }
                else if (strcasecmp(*argv, "-parallelcolor") == 0) {
                        needMore(argc, argv);
                        strncpy(ParallelColor, *++argv, COLORLENGTH);
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
                else if (strcasecmp(*argv, "-mooncolor") == 0) {
                        needMore(argc, argv);
                        strncpy(MoonColor, *++argv, COLORLENGTH);
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
                        position.lat = 100.0;
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
                        ExternAction = RCAlloc(*++argv);
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
                        if (opt) {
                           progress_value[5] = abs(value); 
                           if (value) 
                              gflags.progress = 0;
                           else
                              gflags.progress = 5;
                        } else 
                           time_jump = value;
                        --argc;
                }
                else if (strcasecmp(*argv, "-aspect") == 0) {
                        needMore(argc, argv);
                        gzoom.mode = atoi(*++argv);
                        if (gzoom.mode<0) gzoom.mode = 0;
                        if (gzoom.mode>2) gzoom.mode = 2;
                        --argc;
                }
                else if (strcasecmp(*argv, "-fullcolors") == 0) {
                        gflags.spotsize = 3;
                        gflags.mono = 0;
                        gflags.fillmode = 2;
                }
                else if (strcasecmp(*argv, "-invertcolors") == 0) {
                        gflags.spotsize = 3;
                        gflags.mono = 1;
                        gflags.fillmode = 1;
                }
                else if (strcasecmp(*argv, "-monochrome") == 0) {
                        gflags.spotsize = 4;
                        gflags.mono = 2;
                        gflags.fillmode = 1;
                }
                else if (strcasecmp(*argv, "-verbose") == 0)
                        verbose = 1;
                else if (strcasecmp(*argv, "-coastlines") == 0)
                        gflags.fillmode = 0;
                else if (strcasecmp(*argv, "-contour") == 0)
                        gflags.fillmode = 1;
                else if (strcasecmp(*argv, "-landfill") == 0)
                        gflags.fillmode = 2;
                else if (strcasecmp(*argv, "-dottedlines") == 0)
                        gflags.dotted = 0;
                else if (strcasecmp(*argv, "-plainlines") == 0)
                        gflags.dotted = 1;
                else if (strcasecmp(*argv, "-decimal") == 0)
                        gflags.dms = 0;
                else if (strcasecmp(*argv, "-dms") == 0)
                        gflags.dms = gflags.dms = 1;
                else if (strcasecmp(*argv, "-parallels") == 0)
                        gflags.parallel = 1;
                else if (strcasecmp(*argv, "-cities") == 0)
                        gflags.cities = 1;
                else if (strcasecmp(*argv, "-nonight") == 0)
                        gflags.shading = 0;
                else if (strcasecmp(*argv, "-night") == 0)
                        gflags.shading = 1;
                else if (strcasecmp(*argv, "-terminator") == 0)
                        gflags.shading = 2;
                else if (strcasecmp(*argv, "-twilight") == 0)
                        gflags.shading = 3;
                else if (strcasecmp(*argv, "-luminosity") == 0)
                        gflags.shading = 4;
                else if (strcasecmp(*argv, "-sun") == 0)
                        gflags.sunpos = 1;
                else if (strcasecmp(*argv, "-nomeridians") == 0)
                        gflags.meridian = 0;
                else if (strcasecmp(*argv, "-notropics") == 0)
                        gflags.tropics = 0;
                else if (strcasecmp(*argv, "-noparallels") == 0)
                        gflags.parallel = 0;
                else if (strcasecmp(*argv, "-nocities") == 0)
                        gflags.cities = 0;
                else if (strcasecmp(*argv, "-nosun") == 0)
                        gflags.sunpos = 0;
                else if (strcasecmp(*argv, "-meridians") == 0)
                        gflags.meridian = 1;
                else if (strcasecmp(*argv, "-tropics") == 0)
                        gflags.tropics = 1;
                else if (runtime) {
                   fprintf(stderr, 
                      "Option %s : unknown or not available at runtime !!\n",
                      *argv);
                   runtime = -1;
                   return(0);
                } else if (strcasecmp(*argv, "-dock") == 0)
                        do_dock = 1;
                else if (strcasecmp(*argv, "-zoomsync") == 0)
                        do_zoomsync = 1;
                else if (strcasecmp(*argv, "-nozoomsync") == 0)
                        do_zoomsync = 0;
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
                else if (strcasecmp(*argv, "-selector") == 0)
                        do_selector = 1;
                else if (strcasecmp(*argv, "-noselector") == 0)
                        do_selector = 0;
                else if (strcasecmp(*argv, "-zoom") == 0)
                        do_zoom = 1;
                else if (strcasecmp(*argv, "-nozoom") == 0)
                        do_zoom = 0;
                else if (strcasecmp(*argv, "-listmenu") == 0) {
                        ListMenu();
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
 * Read language i18n file
 */

void 
readLanguage()
{
    char *fname;        /* Path to .sunclockrc file */
    FILE *rc;           /* File pointer for rc file */
    char buf[128];      /* Buffer to hold input lines */
    int i, j, k, l, m, n, p, tot;

    fname = share_i18n;

    i = strlen(share_i18n);
       
    if (!*language && getenv("LANG"))
       strncpy(language, getenv("LANG"), 2);
    if (!(language[0] && language[1]))
       strcpy (language,"en");

    for (j=0; j<=1; j++) share_i18n[i+j-2] = tolower(language[j]);

    j = k = l = m = n = 0;
    tot = 2;
    if ((rc = fopen(fname, "r")) != NULL) {
      while (fgets(buf, 128, rc)) {
        if (buf[0] != '#') {
                p = strlen(buf) - 1;
                if (p>=0 && buf[p]=='\n') buf[p] = '\0';
                if (j<7) { strcpy(Day_name[j], buf); j++; } else
                if (k<12) { strcpy(Month_name[k], buf); k++; } else
                if (l<L_END) { 
		   if (leave) Label[l] = NULL;
		   Label[l] = (char *)realloc(Label[l], (p+1)*sizeof(char));
                   strcpy(Label[l], buf); 
                   l++; 
                } else 
                if (m<N_HELP) { 
		   if (leave) Help[m] = NULL;
		   Help[m] = (char *)realloc(Help[m], (p+1)*sizeof(char));
                   strcpy(Help[m], buf); 
                   m++; 
                } else {
		   if (n==0) {
		      if (leave) {
			 Configurability = NULL;
			 ShortHelp = NULL;
		      } 
                      Configurability = (char *) 
                         realloc(Configurability, (p+1)*sizeof(char));
		      strcpy(Configurability, buf);
		   } else {
		      tot += strlen(buf);
                      ShortHelp = (char *) realloc(ShortHelp,tot*sizeof(char));
		      if (n==1) *ShortHelp = '\0';
                      strcat(ShortHelp, buf); 
                      strcat(ShortHelp, "\n");
		   }
                   n++;
                }
        }
      }
      fclose(rc);
    } else
        fprintf(stderr, 
             "Unable to open language in %s\n", share_i18n);
}


/*
 * readRC() - Read the user's ~/.sunclockrc file and app-defaults
 */

int 
readRC()
{
    /*
     * Local Variables
     */

    char *fname;        /* Path to .sunclockrc file */
    FILE *rc;           /* File pointer for rc file */
    char buf[128];      /* Buffer to hold input lines */
    char option[3][128]; /* Pointers to options */
    char *args[3];      /* Pointers to options */
    char *city, *lat, *lon, *tz; /* Information about a place */
    City *crec;         /* Pointer to new city record */
    int  first_step=1;  /* Are we parsing options in rc file ? */
    int  i, j, shift;   /* indices */

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
           if (*buf=='-') shift = 1; else shift = 0;
           i = sscanf(buf+shift, "%s %s %s\n", 
                 option[0]+1, option[1], option[2]);
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

        if ((lat = strtok(NULL, "       \n")) == NULL) {
            fprintf(stderr, "Error in %s for city %s\n", fname, city);
            continue;
        }

        if ((lon = strtok(NULL, "       \n")) == NULL) {
            fprintf(stderr, "Error in %s for city %s\n", fname, city);
            continue;
        }

        if ((tz = strtok(NULL, "        \n")) == NULL) {
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
    if (rc) fclose(rc);

    readLanguage();

    return(0);
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
   if (win==Option)
     return OptionCaller;

   Context = Seed;
   while (Context && Context->win != win) Context = Context->next;
   return Context;
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

        map_strip = BigFont->max_bounds.ascent + 
                    BigFont->max_bounds.descent + 8;

        chwidth = map_strip + 5;

        clock_strip = SmallFont->max_bounds.ascent +
                      SmallFont->max_bounds.descent + 4;

        MenuGeom.width = MENU_WIDTH * map_strip - 6;
        MenuGeom.height = 2 * map_strip;

        SelGeom.width = SEL_WIDTH * map_strip;
        SelGeom.height = (11 + 4*SEL_HEIGHT)*map_strip/5;

	OptionGeom.height = OptionGeom.h_mini = (15 * map_strip)/4;
}

unsigned long 
getColor(name, other, cmap)
char *           name;
unsigned long    other;
Colormap         cmap;
{

        XColor                  c;
        XColor                  e;
        register Status         s;

        s = XAllocNamedColor(dpy, cmap, name, &c, &e);
        ++total_colors;

        if (s != (Status)1) {
                fprintf(stderr, "%s: warning: can't allocate color `%s'\n",
                        ProgName, name);
                color_alloc_failed = 1;
                return(other);
        } else
                return(c.pixel);
                
}

void
createGData(Context, private)
struct Sundata * Context;
int private;
{
        Sundata *               OtherContext;
        Pixel                   *ptr;
        int                     i, j;

        /* Try to use already defined GCs and Pixels in default colormap
           if already defined */
        if (!private && !runtime)
        for (OtherContext = Seed; OtherContext; 
             OtherContext = OtherContext->next) {
           if (OtherContext != Context &&
               OtherContext->flags.mono == gflags.mono &&
               OtherContext->gdata->cmap == cmap0) {
             Context->gdata = OtherContext->gdata;
             ++Context->gdata->links;
             return;
           }
        } 

    newcmap:

        /* Otherwise, define new adhoc graphical data */
        Context->gdata = (GraphicData *)salloc(sizeof(GraphicData));
        Context->gdata->links = 0;
        if (color_depth>8 || Context->flags.mono==2 ||
           (Context->flags.mono==1 && !private))
           Context->gdata->cmap = cmap0;
        else
           Context->gdata->cmap =
                XCreateColormap(dpy, RootWindow(dpy, scr), &visual, AllocNone);

        color_alloc_failed = 0;

        if (Context->flags.mono==2) {
           Context->gdata->pixlist.white = white;
           Context->gdata->pixlist.black = black;
           Context->gdata->pixlist.textbgcolor = white;
           Context->gdata->pixlist.textfgcolor = black;
           Context->gdata->pixlist.mapbgcolor = white;
           Context->gdata->pixlist.mapfgcolor = black;
           Context->gdata->pixlist.zoombgcolor = white;
           Context->gdata->pixlist.zoomfgcolor = black;
           Context->gdata->pixlist.dircolor = black;
           Context->gdata->pixlist.imagecolor = black;
           Context->gdata->pixlist.choicecolor = black;
           Context->gdata->pixlist.changecolor = black;
        } else {
           total_colors = 0;
           Context->gdata->pixlist.black = 
              getColor("Black", black, Context->gdata->cmap);
           Context->gdata->pixlist.white = 
              getColor("White", white, Context->gdata->cmap);
           Context->gdata->pixlist.textbgcolor = getColor(TextBgColor, 
              Context->gdata->pixlist.white, Context->gdata->cmap);
           Context->gdata->pixlist.textfgcolor = getColor(TextFgColor, 
              Context->gdata->pixlist.black, Context->gdata->cmap);
           Context->gdata->pixlist.mapbgcolor = getColor(MapBgColor, 
              Context->gdata->pixlist.white, Context->gdata->cmap);
           Context->gdata->pixlist.mapfgcolor = getColor(MapFgColor,
              Context->gdata->pixlist.black, Context->gdata->cmap);
           Context->gdata->pixlist.zoombgcolor = getColor(ZoomBgColor,
              Context->gdata->pixlist.white, Context->gdata->cmap);
           Context->gdata->pixlist.zoomfgcolor = getColor(ZoomFgColor, 
              Context->gdata->pixlist.black, Context->gdata->cmap);
           Context->gdata->pixlist.dircolor = getColor(DirColor, 
              Context->gdata->pixlist.black, Context->gdata->cmap);
           Context->gdata->pixlist.imagecolor = getColor(ImageColor, 
              Context->gdata->pixlist.black, Context->gdata->cmap);
           Context->gdata->pixlist.choicecolor = getColor(ChoiceColor,
              Context->gdata->pixlist.black, Context->gdata->cmap);
           Context->gdata->pixlist.changecolor = getColor(ChangeColor,
              Context->gdata->pixlist.black, Context->gdata->cmap);
           Context->gdata->pixlist.citycolor0 = getColor(CityColor0,
              Context->gdata->pixlist.black, Context->gdata->cmap);
           Context->gdata->pixlist.citycolor1 = getColor(CityColor1,
              Context->gdata->pixlist.black, Context->gdata->cmap);
           Context->gdata->pixlist.citycolor2 = getColor(CityColor2,
              Context->gdata->pixlist.black, Context->gdata->cmap);
           Context->gdata->pixlist.markcolor1 = getColor(MarkColor1,
              Context->gdata->pixlist.black, Context->gdata->cmap);
           Context->gdata->pixlist.markcolor2 = getColor(MarkColor2,
              Context->gdata->pixlist.black, Context->gdata->cmap);
           Context->gdata->pixlist.linecolor = getColor(LineColor,
              Context->gdata->pixlist.black, Context->gdata->cmap);
           Context->gdata->pixlist.meridiancolor = getColor(MeridianColor,
              Context->gdata->pixlist.white, Context->gdata->cmap);
           Context->gdata->pixlist.parallelcolor = getColor(ParallelColor,
              Context->gdata->pixlist.white, Context->gdata->cmap);
           Context->gdata->pixlist.tropiccolor = getColor(TropicColor,
              Context->gdata->pixlist.black, Context->gdata->cmap);
           Context->gdata->pixlist.suncolor = getColor(SunColor,
              Context->gdata->pixlist.white, Context->gdata->cmap);
           Context->gdata->pixlist.mooncolor = getColor(MoonColor,
              Context->gdata->pixlist.white, Context->gdata->cmap);

           if (color_depth<=8 && color_alloc_failed &&
               Context->gdata->cmap==cmap0) {
              private = 1;
              fprintf(stderr, 
                 "Color allocation failed with default colormap.\n"
                 "Trying instead private colormap...\n");
              goto newcmap;
           }

           if (color_alloc_failed) 
              fprintf(stderr, "Color allocation failed !!!\n");
           else {
              if (verbose)
                fprintf(stderr, 
                   "Basic colors successfully allocated in %s colormap.\n",
                   (Context->gdata->cmap==cmap0)? "default":"private");
           }

           Context->gdata->usedcolors = total_colors;
           ptr = (Pixel *)&Context->gdata->pixlist;
           for (i=0; i<total_colors; i++) {
              for (j=0; j<i; j++) if (ptr[i]==ptr[j]) { 
                 --Context->gdata->usedcolors; 
                 break;
              }
           }
        }

        Context->gdata->gclist.mapinvert = 0;

}

void
createGCs(Context)
struct Sundata * Context;
{
        XGCValues               gcv;
        Window                  root;

        if (Context->gdata->links==0) {
           if (verbose)
              fprintf(stderr, "Creating new GCs, mode = %d\n", gflags.mono);
        } else {
           if (verbose)
              fprintf(stderr, "Using previous GC settings (%d link)\n", 
                     Context->gdata->links);
           return;
        }

        root = RootWindow(dpy, scr);

        if (Context->flags.mono==2) {
           gcv.background = black;
           gcv.foreground = white;
        } else {
           gcv.background = Context->gdata->pixlist.choicecolor;
           gcv.foreground = Context->gdata->pixlist.zoomfgcolor;
        }
        Context->gdata->gclist.zoomfg = XCreateGC(dpy, root, GCForeground | GCBackground, &gcv);

        if (Context->flags.mono==2) {
           gcv.background = white;
           gcv.foreground = black;
        } else {
           gcv.background = Context->gdata->pixlist.zoombgcolor;
           gcv.foreground = Context->gdata->pixlist.zoomfgcolor;
        }
        Context->gdata->gclist.zoombg = XCreateGC(dpy, root, GCForeground | GCBackground, &gcv);

        if (Context->flags.mono) {
           gcv.background = Context->gdata->pixlist.mapbgcolor;
           gcv.foreground = Context->gdata->pixlist.mapfgcolor;
           Context->gdata->gclist.mapstore = XCreateGC(dpy, root, 
                             GCForeground | GCBackground, &gcv);
        }

        if (Context->flags.mono<2) {
           gcv.background = Context->gdata->pixlist.textbgcolor;
           gcv.foreground = Context->gdata->pixlist.textfgcolor;
        }

        gcv.font = SmallFont->fid;
        Context->gdata->gclist.smallfont = XCreateGC(dpy, root, GCForeground | GCBackground | GCFont, &gcv);
        gcv.font = BigFont->fid;
        Context->gdata->gclist.bigfont = XCreateGC(dpy, root, GCForeground | GCBackground | GCFont, &gcv);

        if (gflags.mono==2)
           Context->gdata->gclist.optionfont = Context->gdata->gclist.bigfont;
        else {
           gcv.background = Context->gdata->pixlist.white;         
           Context->gdata->gclist.optionfont = 
             XCreateGC(dpy, root, GCForeground | GCBackground | GCFont, &gcv);
           gcv.background = Context->gdata->pixlist.textbgcolor;           
        }
        
        if (Context->flags.mono==2) {
           Context->gdata->gclist.dirfont = Context->gdata->gclist.bigfont;
           Context->gdata->gclist.imagefont = Context->gdata->gclist.bigfont;
           Context->gdata->gclist.choice = Context->gdata->gclist.bigfont;
           Context->gdata->gclist.change = Context->gdata->gclist.bigfont;
           return;
        }

        gcv.foreground = Context->gdata->pixlist.dircolor;
        gcv.font = BigFont->fid;
        Context->gdata->gclist.dirfont = XCreateGC(dpy, root, GCForeground | GCBackground | GCFont, &gcv);

        gcv.foreground = Context->gdata->pixlist.imagecolor;
        gcv.font = BigFont->fid;
        Context->gdata->gclist.imagefont = XCreateGC(dpy, root, GCForeground | GCBackground | GCFont, &gcv);

        gcv.foreground = Context->gdata->pixlist.choicecolor;
        Context->gdata->gclist.choice = XCreateGC(dpy, root, GCForeground, &gcv);

        gcv.foreground = Context->gdata->pixlist.changecolor;
        Context->gdata->gclist.change = XCreateGC(dpy, root, GCForeground, &gcv);

        gcv.foreground = Context->gdata->pixlist.citycolor0;
        Context->gdata->gclist.citycolor0 = XCreateGC(dpy, root, GCForeground, &gcv);

        gcv.foreground = Context->gdata->pixlist.citycolor1;
        Context->gdata->gclist.citycolor1 = XCreateGC(dpy, root, GCForeground, &gcv);

        gcv.foreground = Context->gdata->pixlist.citycolor2;
        Context->gdata->gclist.citycolor2 = XCreateGC(dpy, root, GCForeground, &gcv);

        gcv.foreground = Context->gdata->pixlist.markcolor1;
        Context->gdata->gclist.markcolor1 = XCreateGC(dpy, root, GCForeground, &gcv);

        gcv.foreground = Context->gdata->pixlist.markcolor2;
        Context->gdata->gclist.markcolor2 = XCreateGC(dpy, root, GCForeground, &gcv);

        gcv.foreground = Context->gdata->pixlist.linecolor;
        Context->gdata->gclist.linecolor = XCreateGC(dpy, root, GCForeground, &gcv);

        gcv.foreground = Context->gdata->pixlist.meridiancolor;
        Context->gdata->gclist.meridiancolor = XCreateGC(dpy, root, GCForeground, &gcv);

        gcv.foreground = Context->gdata->pixlist.parallelcolor;
        Context->gdata->gclist.parallelcolor = XCreateGC(dpy, root, GCForeground, &gcv);

        gcv.foreground = Context->gdata->pixlist.tropiccolor;
        Context->gdata->gclist.tropiccolor = XCreateGC(dpy, root, GCForeground, &gcv);

        gcv.foreground = Context->gdata->pixlist.suncolor;
        gcv.font = BigFont->fid;
        Context->gdata->gclist.suncolor = XCreateGC(dpy, root, GCForeground, &gcv);

        gcv.foreground = Context->gdata->pixlist.mooncolor;
        Context->gdata->gclist.mooncolor = XCreateGC(dpy, root, GCForeground, &gcv);

}

void
clearStrip(Context)
struct Sundata * Context;
{
        XSetWindowBackground(dpy, Context->win, 
                 Context->gdata->pixlist.textbgcolor);
        XClearArea(dpy, Context->win, 0, Context->geom.height+1, 
                 Context->geom.width, 
                 (Context->wintype)? map_strip-1 : clock_strip-1, False);
        if (Context->flags.bottom) --Context->flags.bottom;
}

/*
 *  Set the timezone of selected location.
 *  This is definitely the most tricky point in the whole sunclock stuff
 *  because of the incompatible Unix implementations !
 */

void
setTZ(cptr)
City    *cptr;
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
        struct tm               ctp, stp;
        time_t                  stime;
        double                  jt, gt;
        double                  sunra, sunrv;
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
double  lat;
{
        double                  duration;
        double                  sundec, junk;
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
        time_t                  gtime, stime, sr, ss, dl;

        menu_lasthint = ' ';

        if (!Context->mark1.city) return;

        time(&Context->time);
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
        int i, x;
        char s[128];

        if (Context->flags.hours_shown) return;
        XClearArea(dpy, Context->win, 0, Context->geom.height+1, 
                 Context->geom.width, map_strip-1, False);
        for (i=0; i<24; i++) {
            sprintf(s, "%d", i);
            x = ((i*Context->zoom.width)/24 + 2*Context->zoom.width
                - chwidth*strlen(s)/8 
                + (int)(Context->sunlon*Context->zoom.width/360.0))%
                  Context->zoom.width + 1 - Context->zoom.dx;
            if (x>=0 && x<Context->geom.width)
            XDrawImageString(dpy, Context->win, Context->gdata->gclist.bigfont, 
               x, BigFont->max_bounds.ascent + Context->geom.height + 4, 
               s, strlen(s));
        }
        Context->flags.hours_shown = 1;
}

void
writeStrip(Context)
struct Sundata * Context;
{
        register struct tm      ltp;
        register struct tm      gtp;
        register struct tm      stp;
        time_t                  gtime;
        time_t                  stime;
        int                     i, l;
        char            s[128];
        char            slat[20], slon[20], slatp[20], slonp[20];
        double          dist;
#ifdef NEW_CTIME
        struct timeb            tp;

        if (ftime(&tp) == -1) {
                fprintf(stderr, "%s: ftime failed: ", ProgName);
                perror("");
                exit(1);
        }
#endif

        time(&Context->time);
        gtime = Context->time + Context->jump;

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
             (Context->wintype)? Context->gdata->gclist.bigfont : Context->gdata->gclist.smallfont, 
             Context->textx, Context->texty, s+label_shift, l-label_shift);
}

void 
initShading(Context) 
struct Sundata * Context;
{
      int i;

      if (Context->flags.shading != 2 && Context->flags.shading != 3) {
         if (Context->tr2) {
            free(Context->tr2);
            Context->tr2 = NULL;
         }
      }
      if (Context->flags.shading == 0 || Context->flags.shading == 4) {
         if (Context->tr1) {
            free(Context->tr1);
            Context->tr1 = NULL;
         }
         return;
      }

      if (!Context->tr1)  
         Context->tr1 = (short *) 
                     salloc(Context->geom.width*sizeof(short));
      for (i=0; i< (int)Context->geom.width; i++) Context->tr1[i] = 0;

      if (Context->flags.shading <2) 
         Context->shadefactor = 1.0;
      else {
         if (!Context->tr2)  
            Context->tr2 = (short *) 
                     salloc(Context->geom.width*sizeof(short));
         for (i=0; i< (int)Context->geom.width; i++) 
            Context->tr2[i] = -1; /* (short)(Context->geom.height-1); */
         if (Context->flags.shading == 2)
            Context->shadefactor = 180.0/(M_PI*(SUN_APPRADIUS+atm_refraction));
         else
            Context->shadefactor = 180.0/(M_PI*(SUN_APPRADIUS+atm_diffusion));
      }

      Context->south = -1;
}       

void
makeContext(Context, build)
struct Sundata * Context;
int build;
{
        if (build) {
           Context->win = 0;
           if (Context->wintype)
              Context->geom = MapGeom;
           else
              Context->geom = ClockGeom;
           Context->zoom = gzoom;
           Context->flags = gflags;
           Context->jump = time_jump;
           Context->clock_img_file = RCAlloc(Clock_img_file);
           Context->map_img_file = RCAlloc(Map_img_file);
           Context->mark1.city = NULL;
           Context->mark1.status = 0;
           Context->pos1.tz = NULL;
           Context->mark2.city = NULL;
           Context->mark2.status = 0;
           Context->pos2.tz = NULL;
           Context->tr1 = Context->tr2 = NULL;
           if (position.lat<=90.0) {
              Context->pos1 = position;
              Context->mark1.city = &Context->pos1;
           }
        }

        Context->textx = 4;
        if (Context->wintype)
           Context->texty = Context->geom.height +
                            BigFont->max_bounds.ascent + 4;
        else
           Context->texty = Context->geom.height +
                            SmallFont->max_bounds.ascent + 3;

        Context->newzoom = Context->zoom;
        setZoomDimension(Context);
        Context->zoom = Context->newzoom;

        Context->hstrip = (Context->wintype)? map_strip : clock_strip;
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
        Context->flags.update = -1;
        Context->time = 0L;
        Context->projtime = -1L;
        Context->wave = (double *) salloc( 
              (2*Context->geom.height+Context->geom.width)*sizeof(double));

        initShading(Context);
}

/*
 * placeSpot() - Put a spot (city or mark) on the map.
 */

void
placeSpot(Context, mode, lon, lat)
struct Sundata * Context;
int    mode;
double lon, lat;                /* Latitude and longtitude of the city */
{
    /*
     * Local Variables
     */

    int ilon, ilat;             /* Screen coordinates of the city */
    int rad;
    GC gc;
    Window w;

    if (mode < 0) return;
    if (Context->flags.mono<2 && !Context->flags.cities && mode <= 2) return;

    if (Context->flags.mono==2) {
       w = Context->pix;
       gc = Context->gdata->gclist.mapinvert;
    } else {
       w = Context->win;
       gc = Context->gdata->gclist.citycolor0;
       if (mode == 0) { gc = Context->gdata->gclist.citycolor0; --mode; }
       if (mode == 1)   gc = Context->gdata->gclist.citycolor1;
       if (mode == 2)   gc = Context->gdata->gclist.citycolor2;
       if (mode == 3)   gc = Context->gdata->gclist.markcolor1;
       if (mode == 4)   gc = Context->gdata->gclist.markcolor2;
       if (mode == 5)   gc = Context->gdata->gclist.suncolor;
    }

    ilon = (int) ((180.0 + lon) * ((double)Context->zoom.width / 360.0))
                  - Context->zoom.dx ;
    ilat = (int) ((double)Context->zoom.height 
                  - (lat + 90) * ((double)Context->zoom.height / 180.0))
                  - Context->zoom.dy ;

    if (ilon<0 || ilon>Context->geom.width) return;
    if (ilat<0 || ilat>Context->geom.height) return;

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

    rad = radius[Context->flags.spotsize];

    if (Context->flags.spotsize != 3)
       XDrawArc(dpy, w, gc, ilon-rad, ilat-rad, 2*rad, 2*rad, 0, 360 * 64);

    if (Context->flags.spotsize == 2)
       XDrawLine(dpy, w, gc, ilon, ilat, ilon, ilat);

    if (Context->flags.spotsize == 3)
       XFillArc(dpy, w, gc, ilon-rad, ilat-rad, 2*rad, 2*rad, 0, 360 * 64);  

    if (Context->flags.spotsize == 4)
       XFillArc(dpy, w, gc, ilon-3, ilat-3, 6, 6, 0, 360 * 64);
}

void
drawCities(Context)
struct Sundata * Context;
{
City *c;
        if (!Context->wintype) return; 
        for (c = cities; c; c = c->next)
            placeSpot(Context, c->mode, c->lon, c->lat);
}

void
drawMarks(Context)
struct Sundata * Context;
{
        if (Context->flags.mono==2 || !Context->wintype) return; 

        /* code for color mode */
        if (Context->mark1.city == &Context->pos1)
          placeSpot(Context, 3, Context->mark1.city->lon, Context->mark1.city->lat);
        if (Context->mark2.city == &Context->pos2)
          placeSpot(Context, 4, Context->mark2.city->lon, Context->mark2.city->lat);
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
        Window w;
        int ilat, i0, i1, i;

        if (!Context->wintype) return; 

        ilat = (int) ((double) Context->zoom.height
                  - (lat + 90) * ((double)Context->zoom.height / 180.0))
                  - Context->zoom.dy ;

        if (ilat<0 || ilat>=Context->geom.height) return;

        if (Context->flags.mono==2) {
           w = Context->pix;
           gc = Context->gdata->gclist.mapinvert;
        } else
           w = Context->win;   

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
          draw_parallel(Context, Context->gdata->gclist.parallelcolor, 
              i*10.0, 3, Context->flags.dotted);
}

void
draw_tropics(Context)
struct Sundata * Context;
{
        static  double val[5] = { -66.55, -23.45, 0.0, 23.45, 66.55 };
        int     i;

        if (Context->flags.mono==2 && Context->flags.parallel)
          draw_parallel(Context, Context->gdata->gclist.parallelcolor, 
              0.0, 3, Context->flags.dotted);

        for (i=0; i<5; i++)
          draw_parallel(Context, Context->gdata->gclist.tropiccolor, 
              val[i], 3, 1);
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
        Window w;
        GC gc;
        int ilon, i0, i1, i, j, jp;

        if (!Context->wintype) return; 
        
        ilon = (int) ((180.0 + lon) * ((double) Context->zoom.width / 360.0))
                   - Context->zoom.dx;
        if (ilon<0 || ilon>=Context->geom.width) return;

        if (Context->flags.mono==2) {
           w = Context->pix;
           gc = Context->gdata->gclist.mapinvert;
        } else {
           w = Context->win;   
           gc = Context->gdata->gclist.meridiancolor;
        }

        i0 = Context->geom.height/2;
        i1 = 1+i0/step;
        for (i=-i1; i<i1; i+=1) {
           j = i0+step*i-thickness;
           jp = i0+step*i+thickness;
           if (j<0) j = 0;
           if (jp>(int)Context->geom.height) jp = (int)Context->geom.height;
           XDrawLine(dpy, w, gc, ilon, j, ilon, jp);
        }
}

void
draw_meridians(Context)
struct Sundata * Context;
{
        int     i;

        for (i=-11; i<=11; i++)
          draw_meridian(Context, i*15.0, 3, Context->flags.dotted);
}

/*
 * draw_sun() - Draw Sun at position where it stands at zenith
 */

void
draw_sun(Context)
struct Sundata * Context;
{
        placeSpot(Context, 5, Context->sunlon, Context->sundec);
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
        XDrawLine(dpy, Context->win, Context->gdata->gclist.bigfont, 
                   0, Context->geom.height, 
                   Context->geom.width-1, Context->geom.height);
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
        drawBottomline(Context);
}

void
drawAll(Context)
struct Sundata * Context;
{
        if (Context->flags.mono) {
           if (!Context->pix) return;
        } else {
           if (!Context->xim) return;
        }
        drawLines(Context);
        drawCities(Context);
        drawMarks(Context);
}

void
showImage(Context)
struct Sundata * Context;
{

        if (button_pressed) return;

        if (Context->flags.update>=2) {
           if (Context->flags.mono)
               XCopyPlane(dpy, Context->pix, Context->win, 
                    Context->gdata->gclist.mapstore, 
                    0, 0, Context->geom.width, Context->geom.height, 0, 0, 1);
           else
               XPutImage(dpy, Context->win, Context->gdata->gclist.bigfont, 
                    Context->xim, 0, 0, 0, 0,
                    Context->geom.width, Context->geom.height);
        }
        if (Context->flags.update) {
           Context->flags.update = 0;
           if (Context->flags.mono<2) {
               drawAll(Context);
               drawSun(Context);
           }
        }
}

void
pulseMarks(Context)
struct Sundata * Context;
{
int     done = 0;
        
        if (!Context->wintype) return; 
        if (Context->flags.mono) {
           if (!Context->pix) return;
        } else {
           if (!Context->xim) return;
        }
        if (Context->mark1.city && Context->mark1.status<0) {
           if (Context->mark1.pulse) {
             placeSpot(Context, 0, Context->mark1.save_lon, 
                   Context->mark1.save_lat);
             done = 1;
           }
           Context->mark1.save_lat = Context->mark1.city->lat;
           Context->mark1.save_lon = Context->mark1.city->lon;
           if (Context->mark1.city == &Context->pos1) {
              done = 1;
              placeSpot(Context, 0, Context->mark1.save_lon, Context->mark1.save_lat);
              Context->mark1.pulse = 1;
           } else
              Context->mark1.pulse = 0;
           Context->mark1.status = 1;
        }
        else
        if (Context->mark1.status>0) {
           if (Context->mark1.city|| Context->mark1.pulse) {
              placeSpot(Context, 0, Context->mark1.save_lon, Context->mark1.save_lat);
              Context->mark1.pulse = 1-Context->mark1.pulse;
              done = 1;
           }
           if (Context->mark1.city == NULL) Context->mark1.status = 0;
        }

        if (Context->mark2.city && Context->mark2.status<0) {
           if (Context->mark2.pulse) {
             placeSpot(Context, 0, Context->mark2.save_lon, Context->mark2.save_lat);
             done = 1;
           }
           Context->mark2.save_lat = Context->mark2.city->lat;
           Context->mark2.save_lon = Context->mark2.city->lon;
           if (Context->mark2.city == &Context->pos2) {
              placeSpot(Context, 0, Context->mark2.save_lon, Context->mark2.save_lat);
              done = 1;
              Context->mark2.pulse = 1;
           } else
              Context->mark2.pulse = 0;
           Context->mark2.status = 1;
        }
        else
        if (Context->mark2.status>0) {
           if (Context->mark2.city || Context->mark2.pulse) {
              placeSpot(Context, 0, Context->mark2.save_lon, Context->mark2.save_lat);
              Context->mark2.pulse = 1 - Context->mark2.pulse;
              done = 1;
           }
           if (Context->mark2.city == NULL) Context->mark2.status = 0;
        }
        if (done) {
           Context->flags.update = 2;
           showImage(Context);
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

        if (Context->flags.mono) {
           XDrawPoint(dpy, Context->pix, Context->gdata->gclist.mapinvert, x, y);
           return;
        }
        i = bytes_per_pixel * x + Context->xim->bytes_per_line * y;

        if (color_depth>16) {
           if (bigendian)
              i += bytes_per_pixel - 3;
           u = Context->ximdata[i];
           v = Context->ximdata[i+1];
           w = Context->ximdata[i+2];
           if (t>=0) {
              factor = Context->flags.darkness + (t * (255-Context->flags.darkness))/Context->flags.colorscale;
              u = (u * factor)/255;
              v = (v * factor)/255;
              w = (w * factor)/255;
           }
           Context->xim->data[i] = u;
           Context->xim->data[i+1] = v;
           Context->xim->data[i+2] = w;
        } else 
        if (color_depth==16) {
	   if (bigendian) {
              u = Context->ximdata[i+1];
              v = Context->ximdata[i];
           } else {
              u = Context->ximdata[i];
              v = Context->ximdata[i+1];
           }
           if (t>=0) {
              factor = Context->flags.darkness + (t * (255-Context->flags.darkness))/Context->flags.colorscale;
              r = v>>3;
              g = ((v&7)<<3) | (u>>5);
              b = u&31;
              r = (r * factor)/31;
              g = (g * factor)/63;
              b = (b * factor)/31;
              u = (b&248)>>3 | (g&28)<<3;
              v = (g&224)>>5 | (r&248);
           }
           if (bigendian) {
              Context->xim->data[i+1] = u;
              Context->xim->data[i] = v;
           } else {
              Context->xim->data[i] = u;
              Context->xim->data[i+1] = v;
	   }
        } else
        if (color_depth==15) {
	   if (bigendian) {
              u = Context->ximdata[i+1];
              v = Context->ximdata[i];
	   } else {
              u = Context->ximdata[i];
              v = Context->ximdata[i+1];
	   }
           if (t>=0) {
              factor = Context->flags.darkness + (t * (255-Context->flags.darkness))/Context->flags.colorscale;
              r = v>>2;
              g = (v&3)<<3 | (u>>5);
              b = u&31;
              r = (r * factor)/31;
              g = (g * factor)/31;
              b = (b * factor)/31;
              u = (b&248)>>3 | (g&56)<<2;
              v = (g&192)>>6 | (r&248)>>1;
           }
           if (bigendian) {
              Context->xim->data[i+1] = u;
              Context->xim->data[i] = v;
	   } else {
              Context->xim->data[i] = u;
              Context->xim->data[i+1] = v;
	   }
        } else {
           if (t>=0) {
             if ((277*x+359*y) % Context->flags.colorscale < Context->flags.colorscale-t)
               Context->xim->data[i] = 
                 Context->nightpixel[(unsigned char)Context->ximdata[i]];
             else
               Context->xim->data[i] = Context->ximdata[i];
           } else
             Context->xim->data[i] = Context->ximdata[i];
        }
}

int
howDark(Context, i, j)
struct Sundata * Context;
int i, j;
{
      double light;
      int k;

      light = daywave[i] * cosval[j] + sinval[j];

      if (Context->flags.shading == 1) {
         if (light >= 0) k = -1; else k = 0;
      } else {
         if (Context->flags.shading<=3 || light<0) 
             light *= Context->shadefactor;
         k = (int) (0.5*(1.0+light)*(Context->flags.colorscale+0.4));
         if (k < 0) k = 0;
         if (k >= Context->flags.colorscale) k = - 1;
      }
      return k;
}

/* To be used in case of mono mode only, to clear night area */

void
clearNightArea(Context)
struct Sundata * Context;
{
      int i, j;

      if (Context->flags.mono==0) return;

      for (i = 0; i < (int)Context->geom.width; i++) {
        if (Context->south==0)
          for (j = Context->tr1[i]; j< (int)Context->geom.height; j++)
             DarkenPixel(Context, i, j, -1);
        else
          for (j = 0; j <Context->tr1[i]; j++)
             DarkenPixel(Context, i, j, -1);
      }
}

/*  moveNightArea  --  Update illuminated portion of the globe.  */

void
moveNightArea(Context)
struct Sundata * Context;
{
      int i, j, k, l, jmin, jmax, j0;
      int midcolor, south, north;
      short tr1, tr2;
      double shift, shiftp, quot, cd, sd;
      double f1, f2, f3; 

      Context->flags.hours_shown = 0;
      if (!Context->flags.shading) return;

      f1 = M_PI/((double) Context->zoom.height);
      f2 = ((double) Context->zoom.height)/M_PI;
      f3 = 1E-8 * f2;

      shift = f1 * (double)Context->zoom.dy;
      shiftp = 0.5*(Context->zoom.height+1) - (double) Context->zoom.dy;
      quot = dtr(Context->sundec);
      cd = cos(quot);
      sd = sin(quot);
      if (quot>0) south = 0; else south = -1;
      north = -1-south;

      daywave = Context->wave;
      cosval = daywave + Context->geom.width;
      sinval = cosval + Context->geom.height;

      quot = 2.0*M_PI/(double)Context->zoom.width;
      for (i = 0; i < (int)Context->geom.width; i++)
         daywave[i] = cos(quot *(((double)i)-Context->fnoon));

      for (j = 0; j < (int)Context->geom.height; j++) {
         quot = shift + f1 * (double)j;
         cosval[j] = sin(quot)*cd;
         sinval[j] = cos(quot)*sd;
      }

      /* Shading = 1 uses tr1 as j-integer value of transition day/night */
      /* Context->south means color value near South pole */
      /* which is updated as south, with north = -1-south = opposite color */

      if (Context->flags.shading == 1) {
         for (i = 0; i < (int)Context->geom.width; i++) {
            if (fabs(sd)>f3)
               tr1 = (int) (shiftp+f2*atan(daywave[i]*cd/sd));
            else
               tr1 = 0;
            if (tr1<0) tr1 = 0;     
            if (tr1>(int)Context->geom.height) tr1 = (int)Context->geom.height;
            if (south==Context->south) {
               for (j = tr1; j<(int)Context->tr1[i]; j++)
                     DarkenPixel(Context, i, j, south);
               for (j = (int)Context->tr1[i]; j<tr1; j++)
                     DarkenPixel(Context, i, j, north);
            } else {
               if (tr1 <= (int)Context->tr1[i]) {
                  for (j = 0; j<tr1; j++)
                     DarkenPixel(Context, i, j, north);
                  for (j = (int)Context->tr1[i]; 
                           j<(int)Context->geom.height; j++)
                     DarkenPixel(Context, i, j, south);
               } else {
                  for (j = 0; j<(int)Context->tr1[i]; j++)
                     DarkenPixel(Context, i, j, north);
                  for (j = tr1; j<(int)Context->geom.height; j++)
                     DarkenPixel(Context, i, j, south);
               }
            }
            Context->tr1[i] = tr1;
         }
         Context->south = south;
         return;
      }

      /* Shading = 4 is quite straightforward... compute everything! */

      if (Context->flags.shading == 4) {
         for (i = 0; i < (int)Context->geom.width; i++)
            for (j = 0; j< (int)Context->geom.height; j++) {
               DarkenPixel(Context, i, j, howDark(Context, i, j));
            }
         return;
      }

      /* Shading = 2,3 uses both tr1 and tr2 and is very tricky... */
      /* If both tr1,tr2 >=0 then normal transition 
            day -> shadow -> night  (or night -> shadow -> day)
         Otherwise we have an "exceptional transition"
            shadow near North pole -> (day or night) -> shadow near South pole
         Day or night is encoded midcolor, determined as follow:
              if tr1<0 then midcolor = Context->south
              if tr2<0 then midcolor = -1-Context->south (opposite color)
         Renormalize integres by  tr1=-2-tr1 if <0 tr2=-2-tr2 if <0.
         Then tr1>tr2 are the limits for the interval where color=midcolor */

      for (i = 0; i < (int)Context->geom.width; i++) {
         if (fabs(sd)>f3)
            j0 = (int) (shiftp+f2*atan(daywave[i]*cd/sd));
         else
            j0 = 0;
         if (j0<0) j0 = 0;
         if (j0>(int)Context->geom.height) j0 = (int)Context->geom.height;
         
         tr1 = 0;
         tr2 = (short)(Context->geom.height-1);
         midcolor = -2;
         if (Context->tr1[i] < 0) {
            Context->tr1[i] = -Context->tr1[i]-2;
            midcolor = Context->south;
         }
         if (Context->tr2[i] < 0 && midcolor==-2) {
            Context->tr2[i] = -Context->tr2[i]-2;
            midcolor = -1-Context->south;
         }

         for (j=j0; j<(int)Context->geom.height; j++) {
            k = howDark(Context, i, j);
            if (k!=south) 
               DarkenPixel(Context, i, j, k);
            else {
               tr2 = (short)(j-1);
               jmax = (int)Context->geom.height-1;
               if (j<jmax && howDark(Context, i, jmax) != south) {
                  for (l=jmax; l>tr2 ; l--) {
                     k = howDark(Context, i, l);
                     if (k!=south) 
                        DarkenPixel(Context, i, l, k);
                     else {
                        jmax = l;
                        break;
                     }
                  }
                  tr1 = (short)(-jmax-1);
               }
               if (Context->tr1[i]<=Context->tr2[i]) {
                  if (Context->south == south) {
                     if ((int)Context->tr2[i]<jmax) jmax=(int)Context->tr2[i];
                  } else {
                     if ((int)Context->tr1[i]>j) j=(int)Context->tr1[i];
                  }
               } else {
                  if (midcolor == south) {
                     for (l=j; l<=jmax && l<= (int)Context->tr2[i]; l++)
                         DarkenPixel(Context, i, l, south);
                     for (l=jmax; l>=j && l>= (int)Context->tr1[i]; l--)
                         DarkenPixel(Context, i, l, south);
                     break;
                  }
               }
               for (l=j; l<=jmax; l++) DarkenPixel(Context, i, l, south);
               break;
            }
         }

         for (j=j0-1; j>=0; j--) {
            k = howDark(Context, i, j);
            if (k!=north) 
               DarkenPixel(Context, i, j, k);
            else {
               tr1 = (short) j+1;
               jmin = 0;
               if (j>0 && howDark(Context, i, 0) != north) {
                  for (l=0; l<tr1; l++) {
                     k = howDark(Context, i, l);
                     if (k!=north) 
                        DarkenPixel(Context, i, l, k);
                     else {
                        jmin = l;
                        break;
                     }
                  }
                  tr2 = (short)(-jmin-1);
               }
               if (Context->tr1[i]<=Context->tr2[i]) {
                  if (Context->south == south) {
                     if ((int)Context->tr1[i]>jmin) jmin=(int)Context->tr1[i];
                  } else {
                     if ((int)Context->tr2[i]<j) j=(int)Context->tr2[i];
                  }
               } else {
                  if (midcolor == north) {
                     for (l=jmin; l<=j && l<= (int)Context->tr2[i]; l++)
                         DarkenPixel(Context, i, l, north);
                     for (l=j; l>=jmin && l>= (int)Context->tr1[i]; l--)
                         DarkenPixel(Context, i, l, north);
                     break;
                  }
               }
               for (l=jmin; l<=j; l++) DarkenPixel(Context, i, l, north);
               break;
            }
         }

         Context->tr1[i] = tr1;
         Context->tr2[i] = tr2;
      }

      Context->south = south;
}

void
drawShadedArea (Context)
Sundata * Context;
{
     int size;

     if (Context->flags.mono || (Context->flags.colorscale == 1)) {
        if (Context->flags.shading) {
           initShading(Context);
           moveNightArea(Context);
        } else {
           clearNightArea(Context);
           if (Context->tr1) {
              free(Context->tr1);
              Context->tr1 = NULL;
           }
        }
     } else {
        size = Context->xim->bytes_per_line*Context->xim->height;
        memcpy(Context->xim->data, Context->ximdata, size); 
        initShading(Context);
     }
}

void checkLocation(Context, name)
struct Sundata * Context;
char *  name;
{
City *c = NULL;

        if (CityInit)
        for (c = cities; c; c = c->next) {
            if (!strcasecmp(c->name, CityInit)) {
                Context->mark1.city = c;
                if (Context->flags.mono==2) Context->mark1.status = -1;
                c->mode = 1;
            } else
                c->mode = 0;
        }

        if (Context->mark1.city == &Context->pos1) {
                Context->flags.map_mode = SOLARTIME;
                Context->mark1.city = NULL;
                setTZ(NULL);
                Context->mark1.city = &Context->pos1;
                if (Context->flags.mono==2) pulseMarks(Context);
                Context->pos1.name = Label[L_POINT];
        }       
}

/* --- */
/*  UPDIMAGE  --  Update current displayed image.  */

void
updateImage(Context)
struct Sundata * Context;
{
        int                     noon;
        double                  fnoon;

        /* If this is a full repaint of the window, force complete
           recalculation. */

        if (button_pressed) return;

        time(&Context->time);
        
        if (Context->flags.mono==2 && !Context->flags.firsttime) drawSun(Context);

        (void) sunParams(Context->time + Context->jump, 
              &Context->sunlon, &Context->sundec, NULL);
        fnoon = Context->sunlon * (Context->zoom.width / 360.0) 
                         - (double) Context->zoom.dx;
        noon = (int) fnoon;
        Context->sunlon -= 180.0;

        if (Context->flags.mono==2) {
          drawSun(Context);
          if (Context->flags.firsttime) {
            drawAll(Context);
            Context->flags.firsttime = 0;
          }
        }

        /* Projecting the illumination curve  for the current seasonal
           instant is costly.  If we're running in real time, only  do
           it every PROJINT seconds.
           If the subsolar point has moved at least one pixel, also 
           update the illuminated area on the screen.   */

        if (Context->projtime < 0 || 
            (Context->time - Context->projtime) > PROJINT ||
            Context->noon != noon || Context->flags.update<0) {
                Context->flags.update = 2;
                Context->projtime = Context->time;
                Context->noon = noon;
                Context->fnoon = fnoon;
                moveNightArea(Context);
        }
}

void setPosition1(Context, x, y)
Sundata *Context;
int x, y;
{
    Context->pos1.name = Label[L_POINT];
    Context->pos1.lat = 90.0-((double)(y+Context->zoom.dy)/
                              (double)Context->zoom.height)*180.0 ;
    Context->pos1.lon = ((double)(x+Context->zoom.dx)/
                         (double)Context->zoom.width)*360.0-180.0 ;
    Context->mark1.city = &Context->pos1;
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

        cx = (int) ((180.0 + cptr->lon) * (Context->zoom.width / 360.0))
                   - Context->zoom.dx;
        cy = (int) (Context->zoom.height 
                   - (cptr->lat + 90.0) * (Context->zoom.height / 180.0))
                   - Context->zoom.dy;

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
        }
        Context->flags.update = 1;
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
        } else
          setPosition1(Context, x, y);
        Context->flags.update = 2;
        break;

      case SOLARTIME:
        if (Context->mark1.city) 
           Context->mark1.city->mode = 0;
        if (cptr) {
          Context->mark1.city= cptr;
          Context->mark1.city->mode = 1;
        } else
          setPosition1(Context, x, y);
        Context->flags.update = 2;
        break;

      default:
        break;
    }

    setDayParams(Context);

    if (Context->flags.mono==2) {
      if (Context->mark1.city) Context->mark1.status = -1;
      if (Context->mark2.city) Context->mark2.status = -1;
    }
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

void
quantize(Context)
Sundata * Context;
{
     int i, j, k, l, compare, quantum, change;
     unsigned short r[256], g[256], b[256];
     int count[256], done[256];
     char substit[256], value[256];
     int d[COMPARE], v1[COMPARE], v2[COMPARE];
     int size, dist;
     XColor xc;

     xc.flags = DoRed | DoGreen | DoBlue;

     if (verbose)
        fprintf(stderr, "Number of distinct colors in the map: %d colors\n",
                Context->ncolors);

     for(i=0; i<Context->ncolors; i++) {
         count[i] = 0;
         xc.pixel = Context->daypixel[i];
         XQueryColor(dpy, tmp_cmap, &xc);
         r[i] = xc.red; g[i] = xc.green; b[i] = xc.blue;
     }

     if (tmp_cmap != cmap0)
        XFreeColormap(dpy, tmp_cmap);
     
     size = Context->xim->bytes_per_line * Context->xim->height;
     for (i=0; i<Context->ncolors; i++) {
         substit[i] = (char)i;
         value[i] = (char)i;
     }

     createGData(Context, 0);
     quantum = (256-Context->gdata->usedcolors)/2;     

     if (Context->ncolors<=quantum) goto finish;

     if (verbose)
        fprintf(stderr, "That's too much, quantizing to %d colors...\n", 
                 quantum);

     for (i=0; i<size; i++) ++count[(unsigned char)Context->xim->data[i]];
     
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

     if (verbose) {
        if (Context->gdata->cmap==cmap0)
           fprintf(stderr, "Allocating colors in default colormap...\n");
        else         
           fprintf(stderr, "Allocating colors in private colormap...\n");
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
           if (!XAllocColor(dpy, Context->gdata->cmap, &xc)) {
              color_alloc_failed = 1;
              value[j] = 0;
           } else
              value[j] = (char)xc.pixel;
           xc.red = (unsigned int) (xc.red * Context->flags.darkness) / 255;
           xc.green = (unsigned int) (xc.green * Context->flags.darkness) / 255;
           xc.blue = (unsigned int) (xc.blue * Context->flags.darkness) / 255;
           if (!XAllocColor(dpy, Context->gdata->cmap, &xc)) 
              color_alloc_failed = 1;
           if (value[j])
              Context->nightpixel[(unsigned char)value[j]] = (char)xc.pixel;
           done[j] = 1;
           k++;
        }
     }

     if (Context->gdata->cmap==cmap0 && color_alloc_failed) {
        if (verbose) fprintf(stderr, "Failed !!\n");
        if (Context->gdata->links==0) free(Context->gdata);
        createGData(Context, 1);
        goto finish;
     }

     if (verbose)
        fprintf(stderr, "%d colors allocated in colormap\n", 
            2*k+Context->gdata->usedcolors);

     for (i=0; i<size; i++)
       Context->xim->data[i] = value[(unsigned char)
                              substit[(unsigned char)Context->xim->data[i]]];
}

int
createImage(Context)
struct Sundata * Context;
{
   FILE *fd;
   char *file, path[1024]="";
   int code; 

   if (color_depth<=8 && !Context->flags.mono)
     tmp_cmap = XCreateColormap(dpy, RootWindow(dpy, scr), &visual, AllocNone);
   else
     tmp_cmap = cmap0;

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

   if (gflags.mono) {
   retry:
     readVMF(path, Context);
     if (Context->bits) {
       Context->pix = XCreatePixmapFromBitmapData(dpy, RootWindow(dpy, scr),
          Context->bits, Context->geom.width,
          Context->geom.height, 0, 1, 1);
       createGData(Context, 0);
       if (!Context->gdata->gclist.mapinvert) {
          XGCValues gcv;
          gcv.function = GXinvert;
          Context->gdata->gclist.mapinvert = 
             XCreateGC(dpy, Context->pix, GCFunction, &gcv);
       }
       if (color_alloc_failed) report_failure(path, 6);
       free(Context->bits);
       createGCs(Context);
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
      quantize(Context);
      if (color_alloc_failed) {
         code = 6;
         XDestroyImage(Context->xim);
         Context->xim = 0;
      }
   } else
      createGData(Context, 0);

   createGCs(Context);

   return code;
}

void 
createWorkImage(Context)
struct Sundata * Context;
{
   int size;

   if (Context->xim) {
     size = Context->xim->bytes_per_line*Context->xim->height;
     if (verbose)
        fprintf(stderr, "Creating work image data of size "
             "%d x %d x %d bpp = %d bytes\n", 
             Context->xim->width, Context->xim->height, 
             Context->xim->bytes_per_line/Context->xim->width,size);
     Context->ximdata = (char *)salloc(size);
     memcpy(Context->ximdata, Context->xim->data, size); 
   }
}

void 
buildMap(Context, wintype, build)
struct Sundata * Context;
int wintype, build;
{
   Window win;
   int presentw, presenth, resize;

   if (build < 2) 
      resize = 0;
   else {
      resize = 1;
      build = 0;
   }

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
      if (do_menu<0) {
         do_menu = 1;
         MenuCaller = Context;
      }
      if (do_selector<0) {
         do_selector = 1;
         SelCaller = Context;
      }
      if (do_zoom<0) {
         do_zoom = 1;
         ZoomCaller = Context;
      }
      if (do_option<0) {
         do_option = 1;
         OptionCaller = Context;
      }
   }

   makeContext(Context, build);

   win = Context->win;
   if (win)
      XSelectInput(dpy, Context->win, 0);
   else
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
      setSizeHints(Context, wintype);
      getPlacement(Context->win, &Context->geom.x, &Context->geom.y, 
                   &presentw, &presenth);
      presenth -= Context->hstrip;
      if (resize ||
          Context->geom.width!=presentw || Context->geom.height!=presenth) {
         XResizeWindow(dpy, Context->win, 
            Context->geom.width, Context->geom.height + Context->hstrip);
         XFlush(dpy);
         usleep(2*TIMESTEP);
         remapAuxilWins(Context);
      } else
         resetAuxilWins(Context);
      createWorkImage(Context);
      setProtocols(Context, Context->wintype);
      setClassHints(Context->win, wintype);
   } else {
      createWorkImage(Context);
      setSizeHints(Context, wintype);
      setClassHints(Context->win, wintype);
      XMapWindow(dpy, Context->win);
      XFlush(dpy);
      usleep(3*TIMESTEP);
      remapAuxilWins(Context);
      setProtocols(Context, wintype);
      Context->prevgeom.width = 0;
   }
   checkLocation(Context, CityInit);
   Context->flags.update = -1;
   clearStrip(Context);   
   doTimeout(Context);
   if (Context->gdata->cmap!=cmap0)
      XSetWindowColormap(dpy, Context->win, Context->gdata->cmap);
   runtime = 0;
}

/*
 *  Process key events in eventLoop
 */

void
processKey(win, keysym)
Window  win;
KeySym  keysym;
{
        double v;
        int i, j, old_mode;
        KeySym key;
        struct Sundata * Context = NULL;

        Context = getContext(win);
        if (!Context) return;
        if (Context->flags.mono) {
           if (!Context->pix) return;
        } else {
           if (!Context->xim) return;
        }

        key = keysym;
        Context->flags.update = 1;
        if (key>=XK_A && key<=XK_Z) key += 32;
        old_mode = Context->flags.map_mode;

        /* fprintf(stderr, "Test: <%c> %d %u\n", key, key, key); */

        if (win==Selector) {
           switch(key) {
             case XK_Escape:
                if (do_selector) 
                  PopSelector(Context);
                return;
             case XK_Page_Up:
                if (selector_shift == 0) return;
                selector_shift -= num_lines/2;
                if (selector_shift <0) selector_shift = 0;
                break;
             case XK_Page_Down:
                if (num_table_entries-selector_shift<num_lines) return;
                selector_shift += num_lines/2;
                break;
             case XK_Up:
                if (selector_shift == 0) return;
                selector_shift -= 1;
                break;
             case XK_Down:
                if (num_table_entries-selector_shift<num_lines) return;
                selector_shift += 1;
                break;
             case XK_Home:
                if (selector_shift == 0) return;
                selector_shift = 0;
                break;
             case XK_End:
                if (num_table_entries-selector_shift<num_lines) return;
                selector_shift = num_table_entries - num_lines+2;
                break;
             case XK_Left:
             case XK_Right:
                return;
             default :
                goto general;
           }
           setupSelector(1);
           return;
        }

        if (win==Zoom) {
           switch(key) {
             case XK_Escape:
                if (do_zoom) 
                  PopZoom(Context);
                return;
             default:
                goto general;
           }
        }

        if (win==Option) {
           i = strlen(option_string);
           switch(keysym) {
             case XK_Escape:
               if (do_option) 
                  PopOption(Context);
               return;
             case XK_Return:
                  activateOption();
               return;
             case XK_Left:
               if (option_caret>0) --option_caret;
               break;
             case XK_Right:
               if (option_caret<i) ++option_caret;
               break;
             case XK_Home:
               option_caret = 0;
               break;
             case XK_End:
               option_caret = strlen(option_string);
               break;
             case XK_BackSpace:
             case XK_Delete:
               if (option_caret>0) {
                  --option_caret;
                  for (j=option_caret; j<i;j++)
                     option_string[j] = option_string[j+1];
               }
               break;
             default:
               if (control_key) {
                  if (keysym==XK_space) {
                     keysym = 31;
                     goto specialspace;
                  }
                  if (keysym==XK_a) option_caret = 0;
                  if (keysym==XK_b && option_caret>0) --option_caret;
                  if (keysym==XK_e) option_caret = i;
                  if (keysym==XK_f && option_caret<i) ++option_caret;
                  if (keysym==XK_d) {
                     for (j=option_caret; j<i;j++)
                        option_string[j] = option_string[j+1];
                  }
                  if (keysym==XK_k) {
                     old_option_caret = option_caret;
                     old_option_length = i;
                     old_option_string_char = option_string[option_caret];
                     option_string[option_caret] = '\0';
                  }
                  if (keysym==XK_y && option_caret==old_option_caret) {
                     option_string[old_option_caret] = old_option_string_char;
                     option_string[old_option_length] = '\0';
                     old_option_caret = -1;
                  }
                  break;
               }
           specialspace:
               if (keysym<31) break;
               if (keysym>255) break;
               if (i<90) {
                  for (j=i; j>option_caret; j--)
                     option_string[j] = option_string[j-1];
                  option_string[option_caret] = (char) keysym;
                  option_string[i+1] = '\0';
                  ++option_caret;
               }
               break;
           }
           setupOption(0);
           return;
        }

     general:
        switch(key) {
           case XK_Tab:
             Context->flags.dms = 1 -Context->flags.dms;
             break;
           case XK_Escape:
             if (do_menu) PopMenu(Context);
             break;
           case XK_less:
             if (Context->prevgeom.width && 
                 (Context->prevgeom.width != Context->geom.width ||
                  Context->prevgeom.height != Context->geom.height)) {
                Context->geom = Context->prevgeom;
                Context->prevgeom.width = 0;
                adjustGeom(Context, 0);
                shutDown(Context, 0);
                buildMap(Context, Context->wintype, 0);
             }
             break;
           case XK_Home:
             label_shift = 0;
             break;
           case XK_End:
             label_shift = 50;
             clearStrip(Context);
             break;
           case XK_Page_Up: 
             if (label_shift>0)
               --label_shift;
             break;
           case XK_Page_Down: 
             if (label_shift<50)
               ++label_shift;
             break;
           case XK_equal:
             do_sync = 1 - do_sync;
             break;
           case XK_Left:
             v = 0.5/Context->newzoom.fx;
             Context->newzoom.fdx -= v;
             if (Context->newzoom.fdx<v) Context->newzoom.fdx = v;
             zoom_mode |= 14;
             activateZoom(Context, zoom_active);
             break;
           case XK_Right:
             v = 0.5/Context->newzoom.fx;
             Context->newzoom.fdx += v;
             if (Context->newzoom.fdx>1.0-v) Context->newzoom.fdx = 1.0-v;
             zoom_mode |= 14;
             activateZoom(Context, zoom_active);
             break;
           case XK_Up:
             v = 0.5/Context->newzoom.fy;
             Context->newzoom.fdy -= v;
             if (Context->newzoom.fdy<v) Context->newzoom.fdy = v;
             zoom_mode |= 14;
             activateZoom(Context, zoom_active);
             break;
           case XK_Down:
             v = 0.5/Context->newzoom.fy;
             Context->newzoom.fdy += v;
             if (Context->newzoom.fdy>1.0-v) Context->newzoom.fdy = 1.0-v;
             zoom_mode |= 14;
             activateZoom(Context, zoom_active);
             break;
           case XK_greater:
             if (do_dock && Context==Seed) break;
             Context->prevgeom = Context->geom;
             Context->geom.width = DisplayWidth(dpy, scr) - 10;
           case XK_KP_Divide:
           case XK_colon:
           case XK_slash:
             key = XK_slash;
             if (do_dock && Context==Seed) break;
             i = Context->zoom.mode;
             if (key!=XK_greater) {
                Context->prevgeom = Context->geom;
                if (i==0)
                   Context->zoom.mode = 1;
                Context->newzoom.mode = Context->zoom.mode;
             }
             if (!do_zoom)
                Context->newzoom = Context->zoom;
             if (setWindowAspect(Context, &Context->zoom)) {
                if (key == XK_greater) {
                   adjustGeom(Context, 0);
                   Context->geom.x = 5;
                   XMoveWindow(dpy, Context->win, 
                      Context->geom.x, Context->geom.y);
                }
                shutDown(Context, 0);
                buildMap(Context, Context->wintype, 0);
                MapGeom = Context->geom;
             }
             Context->zoom.mode = i;
             Context->newzoom.mode = i;
             break;
           case XK_quotedbl:
             if (do_zoom) zoom_active = 1 - zoom_active;
             zoom_mode = 30;
             activateZoom(Context, zoom_active);
             break;
           case XK_KP_Multiply:
           case XK_asterisk:
             if (!memcmp(&Context->newzoom, 
                         &Context->zoom, sizeof(ZoomSettings))) break;
             activateZoom(Context, 1);
             break;
           case XK_period:
             if (Context->mark1.city) {
                Context->newzoom.fdx = 0.5+Context->mark1.city->lon/360.0;
                Context->newzoom.fdy = 0.5-Context->mark1.city->lat/180.0;
                zoom_mode |= 14;
                zoom_lasthint = ' ';
                activateZoom(Context, zoom_active);
             }
             break;
           case XK_at:
                activateOption();
		return;
           case XK_space:
           case XK_exclam:
             menu_newhint = XK_exclam;
             if (Context==Seed && do_dock) return;
             Context->wintype = 1 - Context->wintype;
             if (Context->wintype)
                Context->geom = MapGeom;
             else
                Context->geom = ClockGeom;
             adjustGeom(Context, 1);
             XResizeWindow(dpy, Context->win, 1, 1);
             XMoveWindow(dpy, Context->win,Context->geom.x,Context->geom.y);
             XFlush(dpy);
             shutDown(Context, 0);
             buildMap(Context, Context->wintype, 0);
             return;
           case XK_1:
           case XK_KP_1:
             if (memcmp(&Context->newzoom, 
                        &gzoom, sizeof(ZoomSettings))) {
                Context->newzoom = gzoom;
                zoom_mode |= 15;
                activateZoom(Context, zoom_active);
             }
             break;
           case XK_numbersign:
             if (memcmp(&Context->newzoom, 
                        &Context->zoom, sizeof(ZoomSettings))) {
                Context->newzoom = Context->zoom;
                zoom_mode |= 15;
                activateZoom(Context, zoom_active);
             }
             break;
           case XK_plus:
           case XK_KP_Add:
             Context->newzoom.fx *= ZFACT;
             Context->newzoom.fy *= ZFACT;
             setZoomDimension(Context);
             zoom_mode |= 14;
             activateZoom(Context, zoom_active);
             break;
           case XK_minus:
           case XK_KP_Subtract:
             Context->newzoom.fx /= ZFACT;
             Context->newzoom.fy /= ZFACT;
             setZoomDimension(Context);  
             zoom_mode |= 14;
             activateZoom(Context, zoom_active);
             break;
           case XK_ampersand:
             Context->newzoom.mode = (Context->newzoom.mode+1) %3;
             setZoomDimension(Context);             
             zoom_mode |= 13;
             activateZoom(Context, zoom_active);
             break;
           case XK_a: 
             Context->jump += progress_value[Context->flags.progress];
             Context->flags.update = -1;
             menu_lasthint = ' ';
             break;
           case XK_b: 
             Context->jump -= progress_value[Context->flags.progress];
             Context->flags.update = -1;
             menu_lasthint = ' ';
             break;
           case XK_c: 
             if (!Context->wintype) break;
             if (Context->flags.map_mode != COORDINATES) 
               Context->flags.dms = gflags.dms;
             else
               Context->flags.dms = 1 - Context->flags.dms;
             Context->flags.map_mode = COORDINATES;
             if (Context->mark1.city == &Context->pos1) 
                 Context->mark1.city = NULL;
             if (Context->mark1.city)
               setDayParams(Context);
             if (Context->mark2.city) Context->mark2.city->mode = 0;
             Context->mark2.city = NULL;
             Context->flags.update = 2;
             break;
           case XK_d: 
             if (!Context->wintype) break;
             if (Context->flags.map_mode != DISTANCES) 
               Context->flags.dms = gflags.dms;
             else
               Context->flags.dms = 1 - Context->flags.dms;
             Context->flags.map_mode = DISTANCES;
             break;
           case XK_e: 
             if (!Context->wintype) break;
             Context->flags.map_mode = EXTENSION;
             old_mode = EXTENSION;
             Context->flags.hours_shown = 0;
             show_hours(Context);
             break;
           case XK_f:
             if (!do_selector)
                PopSelector(Context);
             else {
                XMapRaised(dpy, Selector);
                if (SelCaller != Context) {
                   PopSelector(Context);
                   PopSelector(Context);
                }
             }
             break;
           case XK_g: 
             if (!do_menu)
               PopMenu(Context);
             else {
               menu_lasthint = ' ';
               if (keysym==XK_g) {
                  Context->flags.progress = (Context->flags.progress+1) % 6;
                  if (!progress_value[Context->flags.progress])
                     Context->flags.progress = 0;
               }
               if (keysym==XK_G) {
                  Context->flags.progress = (Context->flags.progress+5) % 6;
                  if (!progress_value[Context->flags.progress])
                     Context->flags.progress = 4;
               }
             }
             break;
           case XK_h:
             if (!do_menu) {
                menu_newhint = XK_space;
                PopMenu(Context);
		return;
             } else {
                XMapRaised(dpy, Menu);
                if (MenuCaller != Context) {
                   PopMenu(Context);
                   PopMenu(Context);
                }
             }
             break;
           case XK_i: 
             if (Context->wintype && do_menu) PopMenu(Context);
             XIconifyWindow(dpy, Context->win, scr);
             break;
           case XK_j:
             Context->jump = 0;
             Context->flags.update = -1;
             menu_lasthint = ' ';
             break;
           case XK_k:
             if (do_menu) PopMenu(Context);
             if (do_selector) PopSelector(Context);
             if (Context==Seed && Seed->next==NULL)
                shutDown(Context, -1);
             else
                shutDown(Context, 1);
             return;
           case XK_l:
             if (!Context->wintype) {
                clearStrip(Context);
                if (!Context->wintype)
                   Context->flags.clock_mode = 
                     (Context->flags.clock_mode+1) % num_formats;
                Context->flags.update = 1;
                break;
             }
             Context->flags.map_mode = LEGALTIME;
             if (Context->mark1.city) Context->mark1.city->mode = 0;
             if (Context->mark2.city) Context->mark2.city->mode = 0;
             Context->mark1.city = NULL;
             Context->mark2.city = NULL;
             Context->flags.update = 2;
             break;
           case XK_m:
             if (!Context->wintype) break;
             Context->flags.meridian = 1 - Context->flags.meridian;
             if (Context->flags.mono==2) draw_meridians(Context);
             Context->flags.update = 2;
             break;
           case XK_n:
             if (Context->flags.mono || (Context->flags.colorscale == 1))
                Context->flags.shading = 1 - Context->flags.shading;
             else {
                if (keysym==XK_n) 
                   Context->flags.shading = (Context->flags.shading + 1) % 5;
                if (keysym==XK_N) 
                   Context->flags.shading = (Context->flags.shading + 4) % 5;
             }
             drawShadedArea(Context);
             Context->flags.update = -1;
             break;
           case XK_o:
             if (!do_option)
                PopOption(Context);
             else {
                XMapRaised(dpy, Option);
                if (OptionCaller != Context) {
                   PopOption(Context);
                   PopOption(Context);
                }
             }
             break;
           case XK_p:
             if (!Context->wintype) break;
             Context->flags.parallel = 1 - Context->flags.parallel;
             if (Context->flags.mono==2) draw_parallels(Context);
             Context->flags.update = 2;
             break;
           case XK_q: 
             shutDown(Context, -1);
             break;
           case XK_s: 
             if (Context->flags.map_mode != SOLARTIME) 
               Context->flags.dms = gflags.dms;
             else
               Context->flags.dms = 1 - Context->flags.dms;
             Context->flags.map_mode = SOLARTIME;
             if (Context->mark2.city) Context->mark2.city->mode = 0;
             Context->mark2.city = NULL;
             if (Context->mark1.city)
               setDayParams(Context);
             Context->flags.update = 2;
             break;
           case XK_t:
             if (!Context->wintype) break;
             Context->flags.tropics = 1 - Context->flags.tropics;
             if (Context->flags.mono==2) draw_tropics(Context);
             Context->flags.update = 2;
             break;
           case XK_u:
             if (!Context->wintype) break;
             if (Context->flags.mono==2 && Context->flags.cities) 
                drawCities(Context);
             Context->flags.cities = 1 - Context->flags.cities;
             if (Context->flags.mono==2 && Context->flags.cities) 
                drawCities(Context);
             Context->flags.update = 2;
             break;
           case XK_w: 
             if (Context->time<=last_time+2) return;
             if (do_menu) do_menu = -1;
             if (do_selector) do_selector = -1;
             if (do_zoom) do_zoom = -1;
             if (do_option) do_option = -1;
             buildMap(Context, 1, 1);
             last_time = Context->time;
             break;
           case XK_r:
             clearStrip(Context);
             Context->flags.update = -1;             
             break;
           case XK_x:
             if (ExternAction) system(ExternAction);
             break;
           case XK_y:
             Context->flags.sunpos = 1 - Context->flags.sunpos;
             if (Context->flags.mono==2) draw_sun(Context);
             Context->flags.update = 2;
             break;
           case XK_z:
             if (!do_zoom)
                PopZoom(Context);
             else {
                XMapRaised(dpy, Zoom);
                if (ZoomCaller != Context) {
                   PopZoom(Context);
                   PopZoom(Context);
                }
             }
             break;
           default:
             if (!Context->wintype) {
               Context->flags.clock_mode = 
                   (1+Context->flags.clock_mode) % num_formats;
               Context->flags.update = 1;
             }
             break ;
        }

        if (old_mode == EXTENSION && Context->flags.map_mode != old_mode) 
           clearStrip(Context);

        if (do_menu) {
	   menu_newhint = keysym;
           showMenuHint();
	}
}

/*
 *  Process mouse events from eventLoop
 */

void
processMouseEvent(win, x, y, button, evtype)
Window  win;
int     x, y;
int     button;
int     evtype;
{
char             key;
int              click_pos;
struct Sundata * Context = (struct Sundata *) NULL;

        Context = getContext(win);
        if (!Context) return;
        if (Context->flags.mono) {
           if (!Context->pix) return;
        } else {
           if (!Context->xim) return;
        }

        if (evtype!=MotionNotify) RaiseAndFocus(win);
        if (evtype==ButtonPress && win!=Zoom) return;

        if (win == Menu) {
           if (y>map_strip) return;
           click_pos = x/chwidth;
           if (evtype == MotionNotify) {
	     menu_newhint = (click_pos >= N_MENU)? '\033':MenuKey[2*click_pos];
             showMenuHint();
             return;
           }
           if (do_menu && click_pos >= N_MENU) {
              PopMenu(MenuCaller);
              return;
           }
           key = tolower(MenuKey[2*click_pos]);
           processKey(win, (KeySym)key);
           return;
        }

        if (win == Selector) {
           processSelectorAction(SelCaller, x, y);
           return;
        }

        if (win == Zoom) {
           processZoomAction(ZoomCaller, x, y, button, evtype);
           return;
        }

        if (win == Option) {
           processOptionAction(OptionCaller, x, y, button, evtype);
           return;
        }

        /* Click on bottom strip of window */
        if (y >= Context->geom.height) {
           if (button==1) {
              processKey(win, XK_h);
              return;
           }
           if (button==2) {
              processKey(win, XK_l);
              return;
           }
           if (button==3) {
              /* Open new window */
              processKey(win, XK_w);
              return;
           }
        }

        /* Click on the map */
        if (button==2) {
           processKey(win, XK_f);
           return;
        }
        if (button==3) {
          if (do_zoom && win==ZoomCaller->win) {
              Context->newzoom.fdx = ((double)(x+Context->zoom.dx))
                         /((double)Context->zoom.width);
              Context->newzoom.fdy = ((double)(y+Context->zoom.dy))
                         /((double)Context->zoom.height);
              setZoomAspect(Context, 3);
              setZoomDimension(Context);
              zoom_mode = 14;
              zoom_lasthint = ' ';
              activateZoom(Context, zoom_active);
           } else
           /* Open zoom selector */
             processKey(win, XK_z);
           return;
        } 
        
        /* Click with button 1 on the map */

        /* It's a clock, just execute predefined command */
        if (!Context->wintype) {
           processKey(win, XK_x);
           return;
        }

        /* Otherwise, user wants to get info on a city or a location */
        Context->flags.update = 1;

        /* Erase bottom strip to clean-up spots overlapping bottom strip */
        if (Context->flags.bottom) clearStrip(Context);

        /* Set the timezone, marks, etc, on a button press */
        processPoint(Context, x, y);

        /* if spot overlaps bottom strip, set flag */
        if (y >= Context->geom.height - radius[Context->flags.spotsize]) {
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
           int x, y, w, h, num=0;
           struct Sundata * Context = NULL;
           struct Geometry *Geom=NULL;

           if (win == Menu) return;

           if (win == Selector) {
              Geom = &SelGeom;
              num = 3;
           }
           if (win == Zoom) {
              Geom = &ZoomGeom;
              num = 4;
           }
           if (win == Option) {
              Geom = &OptionGeom;
              num = 5;
           }
           if (num) {
              if (getPlacement(win, &x, &y, &w, &h)) return;
              if (w==Geom->width && h==Geom->height) return;
              if (w<Geom->w_mini) w = Geom->w_mini;
              if (h<Geom->h_mini) h = Geom->h_mini;
              Geom->width = w;
              Geom->height = h;
              if (verbose)
                 fprintf(stderr, "Resizing %s to %d %d\n", 
                   (num==3)? "selector" : ((num==4)? "zoom" : 
                             "option window"), w, h);
              XSelectInput(dpy, win, 0);
              XFlush(dpy);
              XResizeWindow(dpy, win, w, h);
              XFlush(dpy);
              usleep(2*TIMESTEP);
              setSizeHints(NULL, num);
              setProtocols(NULL, num);
              if (num==3)
                 setupSelector(0);
              if (num==4) {
                 if (zoompix) {
                    XFreePixmap(dpy, zoompix);
                    zoompix = 0;
                 }
                 setupZoom(-1);
              }
              if (num==5) {
                 setupOption(-1);
              }
              return;
           }

           Context = getContext(win);
           if(!Context) return;

           if (Context==Seed && !Context->wintype && do_dock) return;

           if (getPlacement(win, &x, &y, &w, &h)) return;
           h -= Context->hstrip;
           if (w==Context->geom.width && h==Context->geom.height) return;
           Context->prevgeom = Context->geom;
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
           (void)setZoomAspect(Context, 3);
           buildMap(Context, Context->wintype, 2);
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
                return;         /* ensure events processed first */

        if (Context->flags.update)
           Context->count = 0;
        else
           Context->count = (Context->count+1) % TIMECOUNT;

        if (Context->count==0) {
           updateImage(Context);
           showImage(Context);
           writeStrip(Context);
           if (Context->flags.mono==2) pulseMarks(Context);
        }
}

void
doExpose(w)
register Window w;
{
        struct Sundata * Context = (struct Sundata *)NULL;

        if (w == Menu) {
           do_menu = 1;
           setupMenu();
           return;
        }

        if (w == Selector) {
           do_selector = 1;
           setupSelector(0);
           return;
        }

        if (w == Zoom) {
           zoom_lasthint = ' ';
           setupZoom(-1);
           return;
        }

        if (w == Option) {
           option_lasthint = ' ';
           setupOption(-1);
           return;
        }

        Context = getContext(w);
        if (!Context) return;

        Context->flags.update = 2;
        showImage(Context);
        writeStrip(Context);
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
register Display *              d;
register XEvent *               e;
register char *                 a;
{
        return (True);
}

void
eventLoop()
{
        XEvent                  ev;
        Sundata *               Context;
        char                    buffer[1];
        KeySym                  keysym;

        for (;;) {
                if (XCheckIfEvent(dpy, &ev, evpred, (char *)0))

                        switch(ev.type) {

                        case FocusChangeMask:
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
                                           PopZoom(ZoomCaller);
                                        else
                                        if (ev.xexpose.window == Option)
                                           PopZoom(OptionCaller);
                                        else
                                           shutDown(
                                         getContext(ev.xexpose.window), 1);
                                }
                                break;

                        case KeyPress:
                        case KeyRelease:
                                XLookupString((XKeyEvent *) &ev, buffer, 
                                              1, &keysym, NULL);
                                if (keysym==XK_Control_L || 
                                    keysym==XK_Control_R) {
                                   if (ev.type == KeyPress) control_key = 1;
                                   if (ev.type == KeyRelease) control_key = 0;
                                } else
                                if (ev.type == KeyPress)
                                   processKey(ev.xexpose.window, keysym);
                                break;

                        case ButtonPress:
                        case ButtonRelease:
                        case MotionNotify:
                                if (ev.type==ButtonPress)
                                   button_pressed = ev.xbutton.button;
                                if (ev.type==ButtonRelease)
                                   button_pressed = 0;
                                processMouseEvent(ev.xexpose.window,
                                             ev.xbutton.x, ev.xbutton.y,
                                             ev.xbutton.button, ev.type);
                                break;

                        case ResizeRequest: 
                                processResize(ev.xexpose.window);
                                break;

                        } else {
                                Sundata *Which = getContext(ev.xexpose.window);
                                usleep(TIMESTEP);
                                if (Which==NULL) Which=Seed;
                                for (Context=Seed; Context; 
                                                   Context=Context->next)
                                if (do_sync || Context == Which ||
                                    (do_dock && Context == Seed))
                                   doTimeout(Context);
                        }
        }
}

int
main(argc, argv)
int             argc;
char **         argv;
{
        char * p;
        int    i, rem_selector, rem_zoom, rem_menu, rem_option;

        /* Open Display */

        dpy = XOpenDisplay(Display_name);
        if (dpy == (Display *)NULL) {
                fprintf(stderr, "%s: can't open display `%s'\n", ProgName,
                        Display_name);
                exit(1);
        }

        scr = DefaultScreen(dpy);
        color_depth = DefaultDepth(dpy, scr);
	bigendian = (ImageByteOrder(dpy) == MSBFirst);
        visual = *DefaultVisual(dpy, scr);
        cmap0 = DefaultColormap(dpy, scr);

        black = BlackPixel(dpy, scr);
        white = WhitePixel(dpy, scr);

        if (color_depth == 16 && visual.green_mask==992) color_depth = 15;
        if (color_depth > 16)
            color_pad = 32;
        else if (color_depth > 8)
            color_pad = 16;
        else
            color_pad = 8;

        wm_protocols = XInternAtom(dpy, "WM_PROTOCOLS", False);
        wm_delete_window = XInternAtom(dpy, "WM_DELETE_WINDOW", False);

        ProgName = *argv;

        /* Set default values */
        initValues();

        /* Read the configuation file */

        leave = 1;
        checkRCfile(argc, argv);
        if (readRC()) exit(1);

        if ((p = strrchr(ProgName, '/'))) ProgName = ++p;

        (void) parseArgs(argc, argv);
        leave = 0;

        if (verbose) {
           fprintf(stderr, 
              "%s: version %s, %s\nDepth %d    Bits/RGB %d   Bigendian : %s\n"
              "Red mask %ld    Green mask %ld    Blue mask %ld\n", 
                ProgName, VERSION, COPYRIGHT,
                color_depth, visual.bits_per_rgb, (bigendian)? "yes":"no",
                visual.red_mask, visual.green_mask, visual.blue_mask);
        }

        /* Correct some option parameters */
        if (placement<0) placement = NW;
        if (gflags.mono && (gflags.shading>=2)) gflags.shading = 1;
        gflags.darkness = (unsigned int) ((1.0-darkness) * 255.25);
        rem_menu = do_menu; do_menu = 0;
        rem_selector = do_selector; do_selector = 0;
        rem_zoom = do_zoom; do_zoom = 0;
        rem_option = do_option; do_option = 0;

        getFonts();
        parseFormats(ListFormats);
        buildMap(NULL, win_type, 1);

        for (i=2; i<=5; i++) {
           createWindow(NULL, i);
           setProtocols(NULL, i);
           setSizeHints(NULL, i);
        }

        if (rem_menu)
           PopMenu(Seed);
        if (rem_selector)
           PopSelector(Seed);
        if (rem_zoom)
           PopZoom(Seed);
        if (rem_option)
           PopOption(Seed);

        eventLoop();
        exit(0);
}
