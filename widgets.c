#include <unistd.h>
#include <sys/types.h>
#include <sys/timeb.h>
#include <sys/stat.h>
#include <string.h>

#include "sunclock.h"
#include "langdef.h"

extern char *   salloc();

extern char             **get_dir_list();
extern int              dup_strcmp();
extern void             free_dirlist();

extern void             processKey();
extern int              readVMF();
extern void             buildMap();
extern int              parseArgs();
extern void             readLanguage();
extern void             getFonts();

extern void             createGData();
extern void             createGCs();

extern Display *	dpy;
extern int		scr;
extern Colormap		cmap0;

extern int              color_depth;
extern int              runtime;

extern XFontStruct      *BigFont, *SmallFont;
extern Atom		wm_delete_window;
extern Window		Menu, Selector, Zoom, Option;
extern Pixmap           zoompix;
extern Pixel		black, white;

extern Flags            gflags;
extern struct Sundata   *Seed;
extern struct Sundata   *MenuCaller, *SelCaller, *ZoomCaller, *OptionCaller;
extern struct Geometry	ClockGeom, MapGeom;
extern struct Geometry  MenuGeom, SelGeom, ZoomGeom, OptionGeom;

extern char *	        ProgName;
extern char *           ExternAction;
extern char *           CityInit;
extern char *           share_maps_dir;
extern char **          dirtable;

extern char *           SmallFont_name;
extern char *           BigFont_name;

extern char             image_dir[1024];
extern char             language[4];
extern char             Default_vmf[];
extern char *           Help[N_HELP];
extern char *           Label[L_END];

extern int		do_menu, do_selector, do_zoom, do_option;
extern int              do_dock, do_title, do_zoomsync;

extern int              chwidth;
extern int              map_strip, clock_strip;
extern int              placement;
extern int              verbose;
extern int		button_pressed;
extern int		leave;

extern int              horiz_shift, vert_shift;
extern int              horiz_drift, vert_drift;
extern int              label_shift, selector_shift, zoom_shift;

extern int              num_lines, num_table_entries;

extern int              zoom_mode, zoom_active;
extern int              option_caret, old_option_length, old_option_caret;

extern char	        menu_lasthint;
extern char	        option_lasthint;
extern char             zoom_lasthint;

extern KeySym	        menu_newhint;
extern char	        option_newhint;
extern char             zoom_newhint;

extern char             old_option_string_char;
extern char             *option_string;

extern long             last_time;
extern long             progress_value[6];

extern double           darkness;
extern unsigned int     adjust_dark;

int                     areaw, areah;

void shutDown();
void destroyGCs();

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
              - Context->hstrip - vert_drift - 5;
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
              dx = (MapGeom.width - ClockGeom.width)/2; else
           if (placement >= NW) {
	      if (placement & 1) 
                 dx = 0; else 
                 dx = MapGeom.width - ClockGeom.width;
           }

	   diff= MapGeom.height - ClockGeom.height + map_strip - clock_strip;

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

/*
 * Set up stuff the window manager will want to know.  Must be done
 * before mapping window, but after creating it.
 */

void
setSizeHints(Context, num)
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
	    xsh.height = Geom->height + Context->hstrip;
	    if (do_dock && Context==Seed) {
              xsh.max_width = xsh.min_width = xsh.width;
              xsh.max_height = xsh.min_height = xsh.height;
	      xsh.flags |= PMaxSize | PMinSize;
	    } else {
              xsh.min_width = Geom->w_mini;
              xsh.min_height = Geom->h_mini;
	    }
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
	    if (num==5) {
	      win = Option;
	      Geom = &OptionGeom;
	      xsh.flags |= PMaxSize;
              xsh.max_width = 2000;
	      xsh.min_width = Geom->w_mini;
              xsh.max_height = xsh.min_height = Geom->h_mini;
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
        char 			name[80];
	char *instance[6] =     
           { "clock", "map", "menu", "selector", "zoom", "option" };
        XClassHint xch;

        xch.res_name = ProgName;
        xch.res_class = "Sunclock";
        XSetClassHint(dpy, win, &xch);

        sprintf(name, "%s %s", ProgName, VERSION);
        XSetIconName(dpy, win, name);
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

	mask = FocusChangeMask | VisibilityChangeMask | ExposureMask | 
               ButtonPressMask | ButtonReleaseMask | KeyPressMask;

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
		mask |= PointerMotionMask | ResizeRedirectMask;
		break;

	   case 5:
	        win = Option;
		mask |= PointerMotionMask | ResizeRedirectMask | KeyReleaseMask;
		break;
	}

	if (!win) return;

       	XSelectInput(dpy, win, mask);
	XSetWMProtocols(dpy, win, &wm_delete_window, 1);
	if (num>=2) setClassHints(win, num);
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
	int		        mask;
	int                     strip;

	xswa.background_pixel = white;
	xswa.border_pixel = black;
	xswa.backing_store = WhenMapped;
	strip = 0;

	mask = CWBackPixel | CWBorderPixel | CWBackingStore;

        switch (num) {

	   case 0:
	        win = &Context->win;
		Geom = &Context->geom;
		strip = clock_strip;
		break;

	   case 1:
	        win = &Context->win;
		Geom = &Context->geom;
		strip = map_strip;
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

	   case 5:
	        if (!Option) win = &Option;
		Geom = &OptionGeom;
		break;
	}

	if (num<=1) {
	   if (Geom->mask & XNegative)
	      Geom->x = DisplayWidth(dpy, scr) - 
                   Geom->width + Geom->x;
	   if (Geom->mask & YNegative)
	      Geom->y = DisplayHeight(dpy, scr) - 
                   Geom->height - strip + Geom->y;
	}

        if (win) {
	   *win = XCreateWindow(dpy, root,
		      Geom->x, Geom->y,
		      Geom->width, 
                      Geom->height + strip, 0,
		      CopyFromParent, CopyFromParent, 
		      CopyFromParent, mask, &xswa);
	}
}

int
getNumCmd(key)
char key;
{
     int i;
     for (i=0; i<N_HELP; i++) 
	 if (key==CommandKey[i]) return i;
     return -1;	
}

void
showMenuHint()
{
        char key;
	char hint[128], more[128], prog_str[60];
	int i,j,k,l;

	if (!do_menu) return;
	if (!MenuCaller) return;

        if (menu_newhint == XK_Shift_L || menu_newhint == XK_Shift_R) return;
	key = toupper((char) menu_newhint);

	if (key == menu_lasthint) return;

        menu_lasthint = key;
	if (key=='\033')
           sprintf(hint, " %s", Label[L_ESCMENU]); 
	else
	if (key==' ')
           sprintf(hint, " %s", Label[L_CONTROLS]); 
	else {
	   i = getNumCmd(key);
	   if (i>=0) 
              sprintf(hint, " %s", Help[i]); 
	   else
	      sprintf(hint, " %s", Label[L_UNKNOWN]);
        }
        *more = '\0';
	if (index("CDS", key))
	   sprintf(more, " (%s)", Label[L_DEGREE]);
	if (index("ABGJ", key)) {
           switch(MenuCaller->flags.progress) {
              case 0: sprintf(prog_str, "1 %s", Label[L_MIN]); break;
	      case 1: sprintf(prog_str, "1 %s", Label[L_HOUR]); break;
	      case 2: sprintf(prog_str, "1 %s", Label[L_DAY]); break;
	      case 3: sprintf(prog_str, "7 %s", Label[L_DAYS]); break;
	      case 4: sprintf(prog_str, "30 %s", Label[L_DAYS]); break;
      	      case 5: sprintf(prog_str, "%ld %s", 
                          progress_value[5], Label[L_SEC]); break;
           }
           sprintf(more, " ( %s %s%s   %s %.3f %s )", 
		  Label[L_PROGRESS], 
                  (key=='A')? "+":((key=='B')? "-":""), prog_str, 
                  Label[L_TIMEJUMP], MenuCaller->jump/86400.0,
		  Label[L_DAYS]);
	}
	if (key == 'H') {
	    sscanf(RELEASE, "%d %d %d", &i, &j, &k);
	    sprintf(more, " (%s %s, %d %s %d, %s)", 
                      ProgName, VERSION, i, Month_name[j-1], k, COPYRIGHT);
	}
	if (key == 'X') {
	    sprintf(more, " : %s", (ExternAction)? ExternAction : "(null)");
	}
        if (*more) strncat(hint, more, 120 - strlen(hint));
        l = strlen(hint);
	if (l<120)
	    for (i=l; i<120; i++) hint[i] = ' ';
	hint[120] = '\0';

	XDrawImageString(dpy, Menu, MenuCaller->gdata->gclist.bigfont, 4, 
              BigFont->max_bounds.ascent + map_strip + 4, 
              hint, strlen(hint));
}

void
setupMenu()
{
	char s[2];
	char which[] = "CDEMPSTU";
	int i, j, b, d;
	GC gc;

	if (!do_menu) return;

        XSetWindowColormap(dpy, Menu, MenuCaller->gdata->cmap);
        XSetWindowBackground(dpy, Menu, MenuCaller->gdata->pixlist.textbgcolor);
        XClearArea(dpy, Menu,  0, 0, MenuGeom.width, MenuGeom.height, False);

        s[1]='\0';
	d = (5*chwidth)/12;
	for (i=0; i<N_MENU; i++) {
	      b = (MenuKey[2*i+1]==';');
	      for (j=(i+1)*chwidth-b; j<=(i+1)*chwidth+b; j++)
	          XDrawLine(dpy, Menu, MenuCaller->gdata->gclist.bigfont, 
                      j, 0, j, map_strip);
	      s[0]=MenuKey[2*i];
	      if (!MenuCaller->wintype && index(which,s[0])) 
                 gc = MenuCaller->gdata->gclist.imagefont;
	      else
                 gc = MenuCaller->gdata->gclist.bigfont;
	      XDrawImageString(dpy, Menu, gc, d+i*chwidth, 
                  BigFont->max_bounds.ascent + 4, s, 1);

	}

	XDrawImageString(dpy, Menu, MenuCaller->gdata->gclist.bigfont, 
                     d +N_MENU*chwidth,
                     BigFont->max_bounds.ascent + 4, 
		     Label[L_ESCAPE], strlen(Label[L_ESCAPE]));
        XDrawLine(dpy, Menu, MenuCaller->gdata->gclist.bigfont, 0, map_strip, 
                     MENU_WIDTH * map_strip, map_strip);
        menu_lasthint = '\0';
	showMenuHint();
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
	  MenuCaller = NULL;
	  return;
	  }

        MenuCaller = Context;
	if (!getPlacement(Context->win, &Context->geom.x, 
                                        &Context->geom.y, &w, &h)) {
	   MenuGeom.x = Context->geom.x + horiz_shift - horiz_drift;
	   /* To center: + (Context->geom.width - MenuGeom.width)/2; */
	   a = Context->geom.y + Context->geom.height + 
               Context->hstrip + vert_shift;
           b = Context->geom.y - MenuGeom.height - vert_shift - 2*vert_drift;
	   if (do_title) b = b-18;
           if (b < (int) MenuGeom.height ) b = MenuGeom.height;
           if (a > (int) DisplayHeight(dpy,scr) 
                   - 2*MenuGeom.height -vert_drift -20)
              a = b;
	   MenuGeom.y = (placement<=NE)? a : b;              
	}
        setSizeHints(NULL, 2);
        XMoveWindow(dpy, Menu, MenuGeom.x, MenuGeom.y);
        XMapWindow(dpy, Menu);
        XMoveWindow(dpy, Menu, MenuGeom.x, MenuGeom.y);
	menu_lasthint = '\0';
	setupMenu();
}

void
clearSelectorPartially()
{
    XSetWindowBackground(dpy, Selector, SelCaller->gdata->pixlist.textbgcolor);
    XClearArea(dpy, Selector, 0, map_strip+1, 
           SelGeom.width-2, SelGeom.height-map_strip-2, False);
}

void
setupSelector(mode)
int mode;
{
	int i, j, b, d, p, q, h, skip;
	char *s, *sp;
	char *banner[10] = { "home", "share", "  /", "  .", "", "", 
	                     "  W", "  K", "  !", Label[L_ESCAPE]};

        if (!do_selector) return;

        XSetWindowColormap(dpy, Selector, SelCaller->gdata->cmap);
        XSetWindowBackground(dpy, Selector, SelCaller->gdata->pixlist.textbgcolor);

	d = chwidth/3;

	if (mode <= 0) {

          XClearArea(dpy, Selector,  0, 0, 
             SelGeom.width, map_strip, False);

	  for (i=0; i<=9; i++)
             XDrawImageString(dpy, Selector, SelCaller->gdata->gclist.bigfont, 
                d+2*i*chwidth, BigFont->max_bounds.ascent + 4, 
                banner[i], strlen(banner[i]));
 
          for (i=1; i<=9; i++) {
	     h = 2*i*chwidth;
             XDrawLine(dpy, Selector, SelCaller->gdata->gclist.bigfont, h, 0, h, 
             map_strip);
	  }

	  /* Drawing small triangular icons */
	  p = map_strip/4;
	  q = 3*map_strip/4;
	  h = 9*chwidth;
	  for (i=0; i<=q-p; i++)
	      XDrawLine(dpy,Selector, SelCaller->gdata->gclist.bigfont,
                    h-i, p+i, h+i, p+i);
	  h = 11*chwidth;
	  for (i=0; i<= q-p; i++)
	      XDrawLine(dpy,Selector,
                 SelCaller->gdata->gclist.bigfont, h-i, q-i, h+i, q-i);

	  h = map_strip;
          XDrawLine(dpy, Selector, SelCaller->gdata->gclist.bigfont, 
              0, h, SelGeom.width, h);
	}
	
        if (mode <= 1) {
            XClearArea(dpy, Selector,  0, map_strip+1, 
                SelGeom.width, SelGeom.height, False);

            XDrawImageString(dpy, Selector, SelCaller->gdata->gclist.dirfont,
                d, BigFont->max_bounds.ascent + map_strip + 4, 
                image_dir, strlen(image_dir));

            h = 2*map_strip;
            XDrawLine(dpy, Selector, SelCaller->gdata->gclist.bigfont, 
                0, h, SelGeom.width, h);
	}

	if (!dirtable) 
	   dirtable = get_dir_list(image_dir, &num_table_entries);
	if (dirtable)
           qsort(dirtable, num_table_entries, sizeof(char *), dup_strcmp);
	else {
	   char error[] = "Directory inexistent or inaccessible !!!";
           XDrawImageString(dpy, Selector, 
                     SelCaller->gdata->gclist.dirfont, d, 3*map_strip,
		     error, strlen(error));
	   return;
	}

	skip = (3*map_strip)/4;
	num_lines = (SelGeom.height-2*map_strip)/skip;
        for (i=0; i<num_table_entries-selector_shift; i++) 
	  if (i<num_lines) {
	  s = dirtable[i+selector_shift];
	  b = (s[strlen(s)-1]=='/');
          if (b==0) {
	    if (strstr(s,".xpm") || strstr(s,".jpg") || strstr(s,".vmf"))
	       b=2;
	  }
	  j = BigFont->max_bounds.ascent + 2 * map_strip + i*skip + 3;
	  sp = (SelCaller->wintype)? 
	    SelCaller->map_img_file : SelCaller->clock_img_file;
	  if (strstr(sp,s)) {
	     if (mode<=3)
                XClearArea(dpy, Selector, 2, 
                   BigFont->max_bounds.ascent + 2 * map_strip, 3, 
                   SelGeom.height, False);
	     if (mode==3)
                XDrawRectangle(dpy, Selector, SelCaller->gdata->gclist.change,
                           d/4, j-BigFont->max_bounds.ascent/2, 3, 4);
	     else
                XFillRectangle(dpy, Selector, SelCaller->gdata->gclist.choice,
                           d/4, j-BigFont->max_bounds.ascent/2, 3, 4);
	  }
	  if (mode<=1)
          XDrawImageString(dpy, Selector, 
              (b==1)? SelCaller->gdata->gclist.dirfont : 
              ((b==2)? SelCaller->gdata->gclist.imagefont : 
                       SelCaller->gdata->gclist.bigfont),
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
	  SelCaller = NULL;
	  return;
	  }

	SelCaller = Context;
	selector_shift = 0;

	if (!getPlacement(Context->win, &Context->geom.x, 
                                        &Context->geom.y, &w, &h)) {
	   SelGeom.x = Context->geom.x + horiz_shift - horiz_drift;
	   /* + (Context->geom.width - SelGeom.width)/2; */
	   a = Context->geom.y + Context->geom.height + 
               Context->hstrip + vert_shift;
           if (do_menu && Context == MenuCaller) 
               a += MenuGeom.height + vert_drift + vert_shift;
           b = Context->geom.y - SelGeom.height - vert_shift - 2*vert_drift;
	   if (do_title) b = b-18;
           if (b < map_strip ) b = MenuGeom.height;
           if (a > (int) DisplayHeight(dpy,scr) 
                   - SelGeom.height - vert_drift -20)
              a = b;
	   SelGeom.y = (placement<=NE)? a : b;              
	}

        setSizeHints(NULL, 3);
        XMoveWindow(dpy, Selector, SelGeom.x, SelGeom.y);
        XMapRaised(dpy, Selector);
        XMoveWindow(dpy, Selector, SelGeom.x, SelGeom.y);
	XFlush(dpy);
	setupSelector(0);
}

void 
processSelectorAction(Context, a, b)
struct Sundata * Context;
int a;
int b;
{
        if (b <= map_strip) {
	  a = a/(2*chwidth);
	  if (a==0 && getenv("HOME"))
	     sprintf(image_dir, "%s/", getenv("HOME")); 
	  if (a==1)
	     strcpy(image_dir, share_maps_dir);
	  if (a==2)
	     strcpy(image_dir, "/"); 
	  if (a==3 && getcwd(NULL,1024))
	     sprintf(image_dir, "%s/", getcwd(NULL,1024));
	  if (a<=3) {
	     selector_shift = 0;
	     if (dirtable) {
	        free(dirtable);
	        dirtable = NULL;
	     }
	  }
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
	      do_selector = -1;
              processKey(Context->win, XK_w);
	      return;
	  }
	  if (a==7) {	
              processKey(Context->win, XK_k);
	      return;
	  }
	  if (a==8) {	
              processKey(Context->win, XK_exclam);
	      return;
	  }
	  if (a>=9) {
	     XUnmapWindow(dpy, Selector);
	     do_selector = 0;
	     return;
	  }
	  setupSelector(1);
	  return;
	}
        if (b <= 2*map_strip) {
	  selector_shift = 0;
	  setupSelector(1);
	  return;
	}
	b = (b-2*map_strip-4)/(3*map_strip/4) + selector_shift;
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
              setupSelector(1);
	      return;
	   } else {
	      char *f, *path;
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
		 if (Context->wintype) 
                    Context->map_img_file = f;
		 else
                    Context->clock_img_file = f;
		 setupSelector(3);
		 adjustGeom(Context, 0);
	         shutDown(Context, 0);
	         buildMap(Context, Context->wintype, 0);
	      } else free(f);
	   }
	}
}

void 
checkZoomSettings(zoom)
ZoomSettings *zoom;
{
    if (zoom->fx<1.0) zoom->fx = 1.0;
    if (zoom->fx>100.0) zoom->fx = 100.0;

    if (zoom->fy<1.0) zoom->fy = 1.0;
    if (zoom->fy>100.0) zoom->fy = 100.0;

    if (zoom->fdx<0.0) zoom->fdx = 0.0;
    if (zoom->fdx>1.0) zoom->fdx = 1.0;

    if (zoom->fdy<0.01) zoom->fdy = 0.01;
    if (zoom->fdy>0.99) zoom->fdy = 0.99;
}

int setZoomAspect(Context, mode)
struct Sundata * Context;
int mode;
{
   double a, b, f, p;
   int change;

   checkZoomSettings(&Context->newzoom);
   
   if (!Context->newzoom.mode) return 0;

   a = Context->newzoom.fx;
   b = Context->newzoom.fy;
   change = 0;

   if (Context->newzoom.mode == 1)
      f = 1.0;
   else
      f = (double)Context->geom.width / 
          ((double)Context->geom.height * 2.0 * sin(M_PI*Context->newzoom.fdy));

   if (mode == 3) {
      p = sqrt( Context->newzoom.fx * Context->newzoom.fy / f);
      Context->newzoom.fx = p;
      Context->newzoom.fy = p * f;
      f = 0;
      mode = 1;
   }

   if (mode == 1) {
      if (f == 0) 
          mode = 2;
      else
          Context->newzoom.fx = Context->newzoom.fy / f;
      if (Context->newzoom.fx < 1.0) {
	  Context->newzoom.fy = Context->newzoom.fy / Context->newzoom.fx;
	  Context->newzoom.fx = 1.0;
      }
      if (Context->newzoom.fx > 100.0) {
	  Context->newzoom.fy = 100.0 * Context->newzoom.fy / Context->newzoom.fx;
	  Context->newzoom.fx = 100.0;
      }
   }

   if (mode == 2) {
      if (f!=0) 
          Context->newzoom.fy = Context->newzoom.fx * f;
      if (Context->newzoom.fy < 1.0) {
	  Context->newzoom.fx = Context->newzoom.fx / Context->newzoom.fy;
	  Context->newzoom.fy = 1.0;
      }
      if (Context->newzoom.fy > 100.0) {
	  Context->newzoom.fx = 100.0 * Context->newzoom.fx / Context->newzoom.fy;
	  Context->newzoom.fy = 100.0;
      }
   }

   if (fabs(a-Context->newzoom.fx) > 1E-4) {
      zoom_mode |= 10;
      change = 1;
   } else
      Context->newzoom.fx = a;

   if (fabs(b-Context->newzoom.fy) > 1E-4) {
      zoom_mode |= 12;
      change = 1;
   } else
      Context->newzoom.fy = b;
    
   if (verbose && change)
      fprintf(stderr, "Adjusting zoom area aspect (%.2f , %.2f) --> "
            "(%.2f , %.2f)\n", a, b, Context->newzoom.fx , Context->newzoom.fy);

   return change;
}

void
setZoomDimension(Context)
struct Sundata * Context;
{
    double rx, ry;

    setZoomAspect(Context, 3);

    Context->newzoom.width = (int) 
       ((double) Context->geom.width * Context->newzoom.fx + 0.25);
    Context->newzoom.height = (int)
       ((double) Context->geom.height * Context->newzoom.fy + 0.25);

    rx = 0.5/Context->newzoom.fx;
    ry = 0.5/Context->newzoom.fy;
    Context->newzoom.dx = (int) 
       ((double) Context->newzoom.width * (Context->newzoom.fdx-rx) + 0.25);
    Context->newzoom.dy = (int)
       ((double) Context->newzoom.height * (Context->newzoom.fdy-ry) + 0.25);

    if (Context->newzoom.dx < 0) Context->newzoom.dx = 0;
    if (Context->newzoom.dy < 0) Context->newzoom.dy = 0;
    if (Context->newzoom.dx+Context->geom.width>Context->newzoom.width)
        Context->newzoom.dx = Context->newzoom.width - Context->geom.width;
    if (Context->newzoom.dy+Context->geom.height>Context->newzoom.height)
        Context->newzoom.dy = Context->newzoom.height - Context->geom.height;

    if (verbose && !button_pressed)
       fprintf(stderr, "Zoom (%.2f, %.2f)  centering at (%.2f, %.2f)\n",
	  Context->newzoom.fx, Context->newzoom.fy, 
          Context->newzoom.fdx, Context->newzoom.fdy);
}

void
showZoomHint()
{
	char hint[120];
	int v;

	if (!do_zoom || zoom_lasthint==zoom_newhint) return;

	zoom_lasthint = zoom_newhint;
	v = ZoomGeom.height - map_strip;

	if (zoom_newhint=='\033')
           strcpy(hint, Label[L_ESCMENU]);
	else
	if (zoom_newhint==' ')
           sprintf(hint, 
                "magx = %.3f, magy = %.3f,  lon = %.3f°, lat = %.3f°", 
                ZoomCaller->newzoom.fx, ZoomCaller->newzoom.fy,
	 	360.0 * ZoomCaller->newzoom.fdx - 180.0, 
                90.0 - ZoomCaller->newzoom.fdy*180.0);
        else
	   strcpy(hint, Help[getNumCmd(zoom_newhint)]);

	XClearArea(dpy, Zoom, 0, v+1, ZoomGeom.width, map_strip-1, False);
	XDrawImageString(dpy, Zoom, ZoomCaller->gdata->gclist.bigfont, 4, 
              v + BigFont->max_bounds.ascent + 3,
              hint, strlen(hint));
}

void
setupZoom(mode)
int mode;
{
    int b, i, j, k, l;
    int zoomx, zoomy, zoomw, zoomh;
    char *num[] = { "1", "2", "5", "10", "20", "50", "100"};
    char *synchro = Label[L_SYNCHRO];
    char s[80];

    if (!do_zoom) return;

    XSetWindowColormap(dpy, Zoom, ZoomCaller->gdata->cmap);
    XSetWindowBackground(dpy, Zoom, ZoomCaller->gdata->pixlist.textbgcolor);

    areaw = ZoomGeom.width - 74;
    areah = ZoomGeom.height - 2*map_strip - 65;

    if (mode == -1) {

       XClearArea(dpy, Zoom,  0,0, ZoomGeom.width, ZoomGeom.height, False);
       XDrawImageString(dpy, Zoom, ZoomCaller->gdata->gclist.bigfont, 
          160-XTextWidth(BigFont, synchro, strlen(synchro))+areah, 
          24+ BigFont->max_bounds.ascent, synchro, strlen(synchro));

       for (i=0; i<=N_ZOOM; i++) {
          if (i<N_ZOOM) {
	     s[0] = ZoomKey[2*i];
	     s[1] = '\0';
	     b = (ZoomKey[2*i+1]==';');
	     for (j=(i+1)*chwidth-b; j<=(i+1)*chwidth+b; j++)
                 XDrawLine(dpy, Zoom, ZoomCaller->gdata->gclist.bigfont, 
                              j, areah+64, j, areah+64+map_strip);
	  } else
	     strcpy(s, Label[L_ESCAPE]);
          XDrawImageString(dpy, Zoom, ZoomCaller->gdata->gclist.bigfont, 
	       i*chwidth + (5*chwidth)/12, 
               BigFont->max_bounds.ascent + areah+69, s, strlen(s));
       }

       for (i=0; i<=6; i++) {
          j = 63 + (int) ( (areah-6)*log(atof(num[i]))/log(100.0));
          k = j - chwidth*strlen(num[i])/10;
          XDrawImageString(dpy, Zoom, ZoomCaller->gdata->gclist.bigfont, k,
                BigFont->max_bounds.ascent + 3, num[i], strlen(num[i]));
          k = j + BigFont->max_bounds.ascent/2 - 10;
          XDrawImageString(dpy, Zoom, ZoomCaller->gdata->gclist.bigfont,
                24-chwidth*strlen(num[i])/4, k , num[i], strlen(num[i]));
          XDrawLine(dpy, Zoom, ZoomCaller->gdata->gclist.bigfont, j, 20, j, 24);
          XDrawLine(dpy, Zoom, ZoomCaller->gdata->gclist.bigfont, 30, j-10, 34, j-10);
       }

       for (i=0; i<=1; i++)
          XDrawLine(dpy, Zoom, ZoomCaller->gdata->gclist.bigfont, 
             0, areah+64+i*map_strip, ZoomGeom.width, areah+64+i*map_strip);
       XDrawLine(dpy, Zoom, ZoomCaller->gdata->gclist.bigfont, 60, 22, areah+60, 22);
       XDrawLine(dpy, Zoom, ZoomCaller->gdata->gclist.bigfont, 32, 50, 32, areah+50);
    }

    XSetWindowBackground(dpy, Zoom, ZoomCaller->gdata->pixlist.white);
    if ((mode & 1) && !ZoomCaller->newzoom.mode)
       XClearArea(dpy, Zoom,  41, 31, 9, 9, False);
    if (mode & 2)
       XClearArea(dpy, Zoom,  61, 31, areah, 9, False);
    if (mode & 4)
       XClearArea(dpy, Zoom,  41, 51, 9, areah, False);

    if (!zoompix) {
       int oldmono, oldfill;
       oldmono = ZoomCaller->flags.mono;
       oldfill = ZoomCaller->flags.fillmode;
       ZoomCaller->flags.mono = 2;
       ZoomCaller->flags.fillmode = 1;
       i = ZoomCaller->geom.width;
       j = ZoomCaller->geom.height;
       ZoomCaller->newzoom = ZoomCaller->zoom;
       ZoomCaller->zoom.width = ZoomCaller->geom.width = areaw;
       ZoomCaller->zoom.height = ZoomCaller->geom.height = areah;
       ZoomCaller->zoom.dx = ZoomCaller->zoom.dy = 0;
       readVMF(Default_vmf, ZoomCaller);
       if (ZoomCaller->bits) {
          zoompix = XCreatePixmapFromBitmapData(dpy, RootWindow(dpy, scr),
                      ZoomCaller->bits, ZoomCaller->geom.width,
                      ZoomCaller->geom.height, 0, 1, 1);
          free(ZoomCaller->bits);
       }
       ZoomCaller->zoom = ZoomCaller->newzoom;
       ZoomCaller->geom.width = i;
       ZoomCaller->geom.height = j;
       ZoomCaller->flags.mono = oldmono;
       ZoomCaller->flags.fillmode = oldfill;
    }

    XSetWindowBackground(dpy, Zoom, ZoomCaller->gdata->pixlist.choicecolor);

    if (mode & 2) {
       i = (int) ( (double)(areah-6)*log(ZoomCaller->zoom.fx)/log(100.00));
       XClearArea(dpy, Zoom,  61+i, 31, 6, 9, False);
       if (ZoomCaller->newzoom.fx != ZoomCaller->zoom.fx) {
         i = (int) ((double)(areah-6)*log(ZoomCaller->newzoom.fx)/log(100.00));
         XDrawRectangle(dpy, Zoom, ZoomCaller->gdata->gclist.change, 61+i, 31, 5, 8);
       }
    }
    if (mode & 4) {
       j = (int) ( (double)(areah-6)*log(ZoomCaller->zoom.fy)/log(100.0));
       XClearArea(dpy, Zoom,  41, 51+j, 9, 6, False);
       if (ZoomCaller->newzoom.fy != ZoomCaller->zoom.fy) {
         j = (int) ((double)(areah-6)*log(ZoomCaller->newzoom.fy)/log(100.0));
         XDrawRectangle(dpy, Zoom, ZoomCaller->gdata->gclist.change, 41, 51+j, 8, 5);
       }
    }

    if (mode & 8) {
       zoomw = (areaw*ZoomCaller->geom.width)/ZoomCaller->zoom.width;
       zoomh = (areah*ZoomCaller->geom.height)/ZoomCaller->zoom.height;
       zoomx = (areaw*ZoomCaller->zoom.dx)/ZoomCaller->zoom.width;
       zoomy = (areah*ZoomCaller->zoom.dy)/ZoomCaller->zoom.height;
       i = areaw-zoomx-zoomw-1;
       j = areah-zoomy-zoomh-1;

       if (zoomy)
          XCopyPlane(dpy, zoompix, Zoom, ZoomCaller->gdata->gclist.zoombg, 
                0, 0, areaw-1, zoomy, 61, 51, 1);
       if (j>0)
          XCopyPlane(dpy, zoompix, Zoom, ZoomCaller->gdata->gclist.zoombg, 
                0, zoomy+zoomh+1, areaw, j, 61, 52+zoomy+zoomh, 1);
       if (zoomx)
          XCopyPlane(dpy, zoompix, Zoom, ZoomCaller->gdata->gclist.zoombg, 
                0, 0, zoomx, areah-1, 61, 51, 1);
       if (i>0)
          XCopyPlane(dpy, zoompix, Zoom, ZoomCaller->gdata->gclist.zoombg, 
                zoomx+zoomw+1, 0, i, areah, 62+zoomx+zoomw, 51, 1);

       XCopyPlane(dpy, zoompix, Zoom, ZoomCaller->gdata->gclist.zoomfg, 
                zoomx, zoomy, zoomw+1, zoomh+1, 61+zoomx, 51+zoomy, 1);

       if (ZoomCaller->newzoom.fx!=ZoomCaller->zoom.fx || 
           ZoomCaller->newzoom.fy!=ZoomCaller->zoom.fy || 
           ZoomCaller->newzoom.fdx!=ZoomCaller->zoom.fdx || 
           ZoomCaller->newzoom.fdy!=ZoomCaller->zoom.fdy) {
          zoomw = (areaw*ZoomCaller->geom.width)/ZoomCaller->newzoom.width;
          zoomh = (areah*ZoomCaller->geom.height)/ZoomCaller->newzoom.height;
          zoomx = (areaw*ZoomCaller->newzoom.dx)/ZoomCaller->newzoom.width;
          zoomy = (areah*ZoomCaller->newzoom.dy)/ZoomCaller->newzoom.height;
	  if (ZoomCaller->flags.mono)
	  for (j=0; j<=zoomh; j++)
	    for (i=0; i<=zoomw; i++) {
               k = zoomx+i; 
               l = zoomy+j; 
	       XDrawPoint(dpy, Zoom, ((i+j)%2)? 
                   ZoomCaller->gdata->gclist.zoomfg : ZoomCaller->gdata->gclist.zoombg, 
                   61+k, 51+l);
	       if (i==0 && j>0 && j<zoomh) i = zoomw-1;
	    }
	  else
          XDrawRectangle(dpy, Zoom, ZoomCaller->gdata->gclist.change,
               61+zoomx, 51+zoomy, zoomw, zoomh);
       }

       i = 60 + (int) (areaw * ZoomCaller->newzoom.fdx);
       j = 50 + (int) (areah * ZoomCaller->newzoom.fdy);

       if (i<60+areaw && j<50+areah)
          XDrawRectangle(dpy, Zoom, ZoomCaller->gdata->gclist.change, i, j, 1, 1);

       XDrawRectangle(dpy, Zoom, ZoomCaller->gdata->gclist.bigfont, 60, 50, 
          areaw+1,areah+1);
    }

    XSetWindowBackground(dpy, Zoom, ZoomCaller->gdata->pixlist.textbgcolor);

    if (mode & 1) {
       XClearArea(dpy, Zoom,  33, 23, 17, 17, False);
       s[0] = '0'+ZoomCaller->newzoom.mode;
       s[1] = '\0';
       XDrawImageString(dpy, Zoom, ZoomCaller->gdata->gclist.bigfont,
	  39, 25+BigFont->max_bounds.ascent, s, 1);
    }

    if (mode & 16) {
       XDrawImageString(dpy, Zoom, ZoomCaller->gdata->gclist.bigfont, 
          areah+177, 25+ BigFont->max_bounds.ascent, (zoom_active)?"+":"-", 1);
    }

    if (mode == -1) {
    XDrawRectangle(dpy, Zoom, ZoomCaller->gdata->gclist.bigfont, 60, 30, areah+1, 10);
    XDrawRectangle(dpy, Zoom, ZoomCaller->gdata->gclist.bigfont, 40, 50, 10, areah+1);
    XDrawRectangle(dpy, Zoom, ZoomCaller->gdata->gclist.bigfont, 32, 22, 18, 18);
    XDrawRectangle(dpy, Zoom, ZoomCaller->gdata->gclist.bigfont, areah+170, 22,18,18);
    }

    XSetWindowBackground(dpy, Zoom, ZoomCaller->gdata->pixlist.textbgcolor);

    zoom_lasthint = '\0';
    showZoomHint();
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
	  ZoomCaller = NULL;
	  zoom_active = 1;
	  return;
	  }

        Context->newzoom = Context->zoom;

	ZoomCaller = Context;
	zoom_shift = 0;
	zoom_mode = 0;
	zoom_active = do_zoomsync;
	zoom_newhint = ' ';

	if (!getPlacement(Context->win, &Context->geom.x, &Context->geom.y, &w, &h)) {
	   ZoomGeom.x = Context->geom.x + horiz_shift - horiz_drift;
	   /* + (Context->geom.width - ZoomGeom.width)/2; */
	   a = Context->geom.y + Context->geom.height + 
               Context->hstrip + vert_shift;
           if (do_menu && Context == MenuCaller) 
               a += MenuGeom.height + vert_drift + vert_shift;
           b = Context->geom.y - ZoomGeom.height - vert_shift - 2*vert_drift;
	   if (do_title) b = b-18;
           if (b < map_strip ) b = MenuGeom.height;
           if (a > (int) DisplayHeight(dpy,scr) 
                   - ZoomGeom.height - vert_drift -20)
              a = b;
	   ZoomGeom.y = (placement<=NE)? a : b;              
	}

        setSizeHints(NULL, 4);
        XMoveWindow(dpy, Zoom, ZoomGeom.x, ZoomGeom.y);
        XMapRaised(dpy, Zoom);
        XMoveWindow(dpy, Zoom, ZoomGeom.x, ZoomGeom.y);
	XFlush(dpy);
        setupZoom(-1);
}

void
activateZoom(Context, rebuild)
Sundata *Context;
int rebuild;
{ 
    setZoomDimension(Context);
    zoom_newhint = ' ';
    setupZoom(zoom_mode);
    if (rebuild) {
       if (Context->zoom.width!=Context->newzoom.width ||
	   Context->zoom.height!=Context->newzoom.height ||
	   Context->zoom.dx!=Context->newzoom.dx ||
	   Context->zoom.dy!=Context->newzoom.dy) {
          Context->zoom = Context->newzoom;
          shutDown(Context, 0);
          buildMap(Context, Context->wintype, 0);
	  zoom_newhint = ' ';
          setupZoom(zoom_mode);
       } else
          Context->zoom = Context->newzoom;
    }
    if (rebuild>0)
       zoom_mode = 0;
}

void processZoomAction(Context, x, y, button, evtype)
Sundata * Context;
int x, y, button, evtype;
{
           double v1, v2;
	   int click_pos;

	   if (y>=areah+64 && y<=areah+64+map_strip) {
	      click_pos = x/chwidth;
	      if (click_pos>=N_ZOOM) 
		 zoom_newhint = '\033';
	      else
		 zoom_newhint = ZoomKey[2*click_pos];
	      if (evtype==MotionNotify) {
		 showZoomHint();
	      }
	      if (evtype==ButtonRelease) {
		 if (zoom_newhint=='W') do_zoom = -1;
		 if (click_pos<N_ZOOM) {
		    processKey(Context->win, tolower(zoom_newhint));
		    showZoomHint();
		    zoom_mode = 0;
		 } else
	            PopZoom(Context);
	      }
	      return;
	   }
	   if (button_pressed>=2) return;

	   if (x>=areah+170 && x<=areah+188 && 
               y>=22 && y<=40 && evtype==ButtonRelease) {
	       zoom_active = 1 -zoom_active;
	       zoom_mode |= 16;
	   } else
	   if (x>=60 && x<=areah+60 && y>=30 && y<=40 && button_pressed) {
              v1 = exp((double)(x-66)*log(100.00)/(double)(areah-6));
	      if (v1 != Context->newzoom.fx) {
		 Context->newzoom.fx = v1;
		 zoom_mode |= 10;
                 (void) setZoomAspect(Context, 2);
	      }
	   } else
	   if (x>=40 && x<=50 && y>=50 && y<=areah+50 && button_pressed) {
              v2  = exp((double)(y-53)*log(100.00)/(double)(areah-6));
	      if (v2 != Context->newzoom.fy) {
		 Context->newzoom.fy = v2;
		 zoom_mode |= 12;
                 (void) setZoomAspect(Context, 1);
	      }
	   }
           else
	   if (x>=60 && y>=50 && x<=60+areaw && y<=50+areah && button_pressed){
              v1 = ((double)(x-60))/((double)areaw);
              v2 = ((double)(y-50))/((double)areah);
              if (v1!=Context->newzoom.fdx || v2!=Context->newzoom.fdy) {
		 Context->newzoom.fdx = v1;
		 Context->newzoom.fdy = v2;
	         zoom_mode |= 8;
	      }
	      (void) setZoomAspect(Context, 3);
	   }
	   else {
	     zoom_newhint = ' ';
	     showZoomHint();
	     if (button_pressed) return;
	   }

	   if (zoom_mode>0) {
              setZoomDimension(Context);
	      if (zoom_active && !button_pressed &&
                  memcmp(&Context->newzoom, 
                         &Context->zoom, sizeof(ZoomSettings))) {
		 button_pressed = 0;
		 activateZoom(Context, -1);
	      } else
	         setupZoom(zoom_mode);
 	      if (!button_pressed) zoom_mode = 0;
	   }
}

void
showOptionHint()
{
	char *hint;
	int v;

	if (!do_option || option_lasthint==option_newhint) return;

	option_lasthint = option_newhint;
	v = OptionGeom.height - map_strip;

	if (option_newhint=='\033')
           hint = Label[L_ESCMENU];
	else
	if (option_newhint=='\n')
           hint = Label[L_ACTIVATE];
	else
	if (option_newhint=='?')
           hint = Label[L_INCORRECT];
	else
	   hint = Help[getNumCmd(option_newhint)];

	XClearArea(dpy, Option, 0, v+1, OptionGeom.width, map_strip-1, False);

	if (option_newhint != ' ')
 	   XDrawImageString(dpy, Option, OptionCaller->gdata->gclist.bigfont, 
              4, v + BigFont->max_bounds.ascent + 3,
              hint, strlen(hint));
}

void
setupOption(mode)
int mode;
{
    int b, i, j, opth, vskip;
    char s[80];

    XSetWindowColormap(dpy, Option, OptionCaller->gdata->cmap);
    XSetWindowBackground(dpy, Option, OptionCaller->gdata->pixlist.textbgcolor);

    opth = OptionGeom.height-2*map_strip;
    vskip = 3*map_strip/8;
    option_lasthint = '\0';

    if (mode == -1) {
       XClearArea(dpy, Option, 0,0, OptionGeom.width,OptionGeom.height, False);
       for (i=0; i<=N_OPTION; i++) {
          if (i<N_OPTION) {
	     s[0] = OptionKey[2*i];
	     s[1] = '\0';
	     b = (OptionKey[2*i+1]==';');
	     for (j=(i+1)*chwidth-b; j<=(i+1)*chwidth+b; j++)
                 XDrawLine(dpy, Option, OptionCaller->gdata->gclist.bigfont, 
                   j, opth, j, opth+map_strip);
	  } else
	     strcpy(s, Label[L_ESCAPE]);
          XDrawImageString(dpy, Option, OptionCaller->gdata->gclist.bigfont, 
	       i*chwidth + (5*chwidth)/12, 
               BigFont->max_bounds.ascent + opth + 4, s, strlen(s));
       }
       for (i=0; i<=1; i++)
          XDrawLine(dpy, Option, OptionCaller->gdata->gclist.bigfont, 
               0, opth+i*map_strip, OptionGeom.width, opth+i*map_strip);

       strcpy(s, Label[L_OPTION]);
       XDrawImageString(dpy, Option, OptionCaller->gdata->gclist.bigfont, 
	       8, BigFont->max_bounds.ascent + vskip+3, s, strlen(s));
       XDrawRectangle(dpy, Option, OptionCaller->gdata->gclist.bigfont,
                           70, vskip, OptionGeom.width-85, map_strip);
    }  

    XSetWindowBackground(dpy, Option,
       OptionCaller->gdata->pixlist.white);
    XClearArea(dpy, Option, 71,vskip+1, OptionGeom.width-86,map_strip-1,False);
    XSetWindowBackground(dpy, Option,
       OptionCaller->gdata->pixlist.textbgcolor);
    XDrawImageString(dpy, Option, OptionCaller->gdata->gclist.optionfont, 76,
       BigFont->max_bounds.ascent+vskip+3,option_string,strlen(option_string));
    i = XTextWidth(BigFont, option_string, option_caret);
    j = XTextWidth(BigFont, "_", 1);
    XFillRectangle(dpy, Option, OptionCaller->gdata->gclist.choice,
       76+i, BigFont->max_bounds.ascent+vskip+map_strip/3-1, j, 2);
    showOptionHint();
}

void
PopOption(Context)
struct Sundata * Context;
{
	int    w, h, a, b;
	
	do_option = 1 - do_option;

        if (!do_option) 
	  {
	  XUnmapWindow(dpy, Option);
	  OptionCaller = NULL;
	  return;
	  }

        OptionCaller = Context;
	if (!option_string) {
	  option_string = (char *) salloc(100*sizeof(char));
	  *option_string = '\0';
	  option_caret = 0;
	}
	if (runtime) 
           option_newhint = '\n';
	else
	   option_newhint = ' ';
	if (!getPlacement(Context->win, &Context->geom.x, &Context->geom.y, &w, &h)) {
	   OptionGeom.x = Context->geom.x + horiz_shift - horiz_drift;
	   /* To center: + (Context->geom.width - OptionGeom.width)/2; */
	   a = Context->geom.y + Context->geom.height + 
               Context->hstrip + vert_shift;
           if (do_menu && Context == MenuCaller) 
               a += MenuGeom.height + vert_drift + vert_shift;
           b = Context->geom.y - OptionGeom.height - vert_shift - 2*vert_drift;
	   if (do_title) b = b - 18;
           if (b < (int) OptionGeom.height ) b = OptionGeom.height;
           if (a > (int) DisplayHeight(dpy,scr) 
                   - 2*OptionGeom.height -vert_drift -20)
              a = b;
	   OptionGeom.y = (placement<=NE)? a : b;              
	}
        setSizeHints(NULL, 5);
        XMoveWindow(dpy, Option, OptionGeom.x, OptionGeom.y);
        XMapWindow(dpy, Option);
        XMoveWindow(dpy, Option, OptionGeom.x, OptionGeom.y);
        XFlush(dpy);
	setupOption(-1);
}

void
activateOption()
{
        Sundata *Context;
	Flags oldflags;
        char option[3][128]; /* Pointers to options */
        char *args[3];      /* Pointers to options */
	char *OldCityInit;
	char oldlang[4];
	char *oldbf, *oldsf;
        int i, j, shift, pop;
	short *ptr, *oldptr, *newptr;

	Context = OptionCaller;
        leave = 0;
        option[0][0] = '-';
        for (i=0; i<=2; i++) args[i] = option[i];
 	if (*option_string=='-') shift = 1; else shift = 0;
	i = sscanf(option_string+shift, "%s %s %s\n", 
                  option[0]+1, option[1], option[2]);
        j = strlen(option[0])-1;
	if (option[0][j]==':') option[0][j]='\0';
	oldflags = gflags;
	oldbf = BigFont_name;
	oldsf = SmallFont_name;
	OldCityInit = CityInit;
	strncpy(oldlang, language, 2);
	runtime = 1;
	i = parseArgs(i+1, args-1);
        if (i>0 || runtime<0) {
	   option_newhint = '?';
	   showOptionHint();
	   return;
	} else {
	   option_newhint = '\n';
	   showOptionHint();
	}	     
	if (strncmp(language, oldlang, 2)) readLanguage();
	if (oldbf !=BigFont_name) free(oldbf);
	if (oldsf !=SmallFont_name) free(oldsf);
	if (oldbf !=BigFont_name || oldsf !=SmallFont_name) {
           PopOption(Context);
           XFreeFont(dpy, BigFont);
	   XFreeFont(dpy, SmallFont);
	   getFonts();
	   pop = 1;
	} else {
           showOptionHint();
	   pop = 0;
	}
	shutDown(Context, 0);
	if (gflags.mono && gflags.shading>=2) gflags.shading = 1;
        gflags.darkness = (unsigned int) ((1.0-darkness) * 255.25);
	ptr = (short *) &gflags;
	oldptr = (short *) &oldflags;
	newptr = (short *) &Context->flags;
        for (i=0; i<sizeof(Flags)/sizeof(short); i++) 
            if (ptr[i]!=oldptr[i]) newptr[i] = ptr[i];
	buildMap(Context, Context->wintype, 0);
	if (pop) PopOption(Context);
	return;
}

void 
processOptionAction(Context, x, y, button, evtype)
Sundata * Context;
int x, y, button, evtype;
{
        int i, opth, vskip, click_pos;
	KeySym key;

        opth = OptionGeom.height - 2 * map_strip;
	vskip = 3*map_strip/8;

	if (evtype==ButtonRelease && x>=70 && x<=OptionGeom.width-15 &&
            y>=vskip && y<=map_strip+vskip) {
	   click_pos = (x-76)/XTextWidth(BigFont, "_", 1);
	   i = strlen(option_string);
	   if (click_pos<0) click_pos = 0;
	   if (click_pos>i) click_pos = i;
	   option_caret = click_pos;
	   setupOption(0);
	   return;
	}

	if (y>=opth && y<=opth+map_strip) {
           click_pos = x/chwidth;
	   if (click_pos>=N_OPTION)
	      option_newhint = '\033';
	   else
	      option_newhint = OptionKey[2*click_pos];
	   if (evtype==MotionNotify) 
              showOptionHint();
	   if (evtype==ButtonRelease) {
	      if (click_pos<N_OPTION) {
		 key = (KeySym)tolower(option_newhint);
		 showOptionHint();
	         if (key=='"') {
		    old_option_caret = 0;
		    old_option_length = strlen(option_string);
		    old_option_string_char = *option_string;
		    *option_string = '\0';
		    option_caret = 0;
		    setupOption(0);
		    return;
	         } else
	           processKey(Context->win, key);
	      } else
	         PopOption(Context);
	   }
	}
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

void
resetAuxilWins(Context)
Sundata * Context;
{
      if (do_menu && Context == MenuCaller) {
	 menu_lasthint = '\0';
	 setupMenu();
      }
      if (do_selector && Context == SelCaller) { 
	 do_selector = 1;
	 setupSelector(0);
      }
      if (do_zoom && Context == ZoomCaller) { 
         do_zoom = 1; 
	 setupZoom(-1);
      }
      if (do_option && Context == OptionCaller) { 
         do_option = 1; 
	 setupOption(-1);
      }
}

void
remapAuxilWins(Context)
struct Sundata * Context;
{
      int i;

      if (do_menu || do_selector || do_zoom || do_option) {
	 XFlush(dpy);
	 usleep(2*TIMESTEP);
      }

      if (do_menu) { 
	 do_menu = 1;
	 menu_lasthint = '\0';
         for (i=0; i<=1; i++) PopMenu(Context); 
      }
      if (do_selector) {
         do_selector = 1;
         for (i=0; i<=1; i++) PopSelector(Context);
      }
      if (do_zoom) {
	 do_zoom = 1;
	 zoom_lasthint = '0';
         for (i=0; i<=1; i++) PopZoom(Context);
      }
      if (do_option) {
	 do_option = 1;
	 option_lasthint = ' ';
         for (i=0; i<=1; i++) PopOption(Context);
      }
}

void RaiseAndFocus(win)
Window win;
{
     XFlush(dpy);
     XRaiseWindow(dpy, win);
     XSetInputFocus(dpy, win, RevertToPointerRoot, CurrentTime);
}

/*
 * Free GC's.
 */

void
destroyGCs(Context)
Sundata * Context;
{
         GClist *gclist;

	 if (Context->gdata->links>0) {
            --Context->gdata->links;
	    return;
	 }

         if (Context->gdata->cmap!=cmap0)
	    XFreeColormap(dpy, Context->gdata->cmap);

	 gclist = &Context->gdata->gclist;

 	 XFreeGC(dpy, gclist->bigfont);
 	 XFreeGC(dpy, gclist->smallfont);

         if (Context->flags.mono)
 	    XFreeGC(dpy, gclist->mapstore);

         if (Context->gdata->gclist.mapinvert) {
	    XFreeGC(dpy, Context->gdata->gclist.mapinvert);
	    Context->gdata->gclist.mapinvert = 0;
	 }

 	 if (Context->flags.mono<2) {
 	    XFreeGC(dpy, gclist->citycolor0);
 	    XFreeGC(dpy, gclist->citycolor1);
 	    XFreeGC(dpy, gclist->citycolor2);
 	    XFreeGC(dpy, gclist->markcolor1);
 	    XFreeGC(dpy, gclist->markcolor2);
 	    XFreeGC(dpy, gclist->linecolor);
 	    XFreeGC(dpy, gclist->tropiccolor);
 	    XFreeGC(dpy, gclist->suncolor);
            XFreeGC(dpy, gclist->dirfont);
            XFreeGC(dpy, gclist->imagefont);
	    XFreeGC(dpy, gclist->choice);
	    XFreeGC(dpy, gclist->change);
 	 }

	 free(Context->gdata);
}

/*
 * Free resources.
 */

void
shutDown(Context, all)
struct Sundata * Context;
int all;
{
        struct Sundata * ParentContext, *NextContext;

	if (all<0)
	   Context = Seed;

      repeat:
	fflush(stderr);

        NextContext = Context->next;
        ParentContext = parentContext(Context);

	if (Context->xim) {
           XDestroyImage(Context->xim); 
           Context->xim = NULL;
	}
	if (Context->ximdata) {
	   free(Context->ximdata);
	   Context->ximdata = NULL;
	}
	if (Context->pix) {
           XFreePixmap(dpy, Context->pix);
	   Context->pix = 0;
	}
        if (Context->tr1) {
           free(Context->tr1);
           Context->tr1 = NULL;
        }
        if (Context->tr2) {
           free(Context->tr2);
           Context->tr2 = NULL;
        }
        if (Context->wave) {
           free(Context->wave);
           Context->wave = NULL;
	}
        destroyGCs(Context);
	Context->flags.hours_shown = 0;
	Context->flags.firsttime = 1;

        if (all) {
	   last_time = 0;

           if (Context->win) {
	      if (do_menu && Context == MenuCaller) PopMenu(Context);
	      if (do_selector && Context == SelCaller) PopSelector(Context);
	      if (do_zoom && Context == ZoomCaller) PopZoom(Context);
	      if (do_option && Context == OptionCaller) PopOption(Context);
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
           if (Context->clock_img_file) {
              free(Context->clock_img_file);
	      Context->clock_img_file = NULL;
	   }
           if (Context->map_img_file) {
              free(Context->map_img_file);
	      Context->map_img_file = NULL;
	   }
	   if (all<0) {
	      free(Context);
	      if (NextContext) {
	         Context = NextContext;
	         goto repeat;
	      }
	      else {
	        endup:
         	 XDestroyWindow(dpy, Menu);
         	 XDestroyWindow(dpy, Selector);
         	 XDestroyWindow(dpy, Zoom);
                 if (zoompix) XFreePixmap(dpy, zoompix);
         	 XFreeFont(dpy, BigFont);
         	 XFreeFont(dpy, SmallFont);
                 XCloseDisplay(dpy);
         	 if (dirtable) free(dirtable);
         	 exit(0);
 	      }
 	   }
	   if (ParentContext)
	     ParentContext->next = Context->next;
	   else
             Seed = Context->next;
	   free(Context);
           if (Seed == NULL) goto endup;
	}
}
