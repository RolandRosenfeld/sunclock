/*
 * mapgen.c
 * Jean-Pierre Demailly
 * November 2000
 *
 * Everything serious is in mapdata.c
 * This is just the command line interface...
 */

#include <stdio.h>
#include <math.h>

#include "sunclock.h"

extern char * salloc();
extern void freeData();
extern void makePixmap();

extern int fill_mode;
extern int free_data;

#define XPM 1

struct Geometry MapGeom;
struct Sundata Earthmap;
struct Color LandColor, WaterColor, ArcticColor;

char ProgName[] = "mapgen";
char name[256] = "earthmap";
int invert=0;

void
print_xpm()
{
  int j;

  if (fill_mode<=1) {

    printf("/* XPM */\n"
         "static char * %s_xpm[] = {\n"
         "/* columns rows colors chars-per-pixel */\n"
         "\"%s\",\n\"%s\",\n\"%s\",\n"
         "/* pixels */\n", 
         name, Earthmap.xpmdata[0], Earthmap.xpmdata[1], Earthmap.xpmdata[2]);

    for (j=0; j<Earthmap.geom.height; j++) {
      printf("\"%s\",\n", Earthmap.xpmdata[j+3]);
    }
  }

  if (fill_mode==2) {

    printf("/* XPM */\n"
         "static char * %s_xpm[] = {\n"
         "/* columns rows colors chars-per-pixel */\n"
         "\"%s\",\n\"%s\",\n\"%s\",\n\"%s\",\n"
         "/* pixels */\n", 
         name, Earthmap.xpmdata[0], Earthmap.xpmdata[1], 
               Earthmap.xpmdata[2], Earthmap.xpmdata[3]);

    for (j=0; j<Earthmap.geom.height; j++) {
      printf("\"%s\",\n", Earthmap.xpmdata[j+4]);
    }
  }

  printf("};\n");
}

void
print_xbm()
{
  int i, j, l, a, b, t, count;
  unsigned char u;

  printf("#define %s_width %d\n"
         "#define %s_height %d\n"
	 "static unsigned char %s_bits[] = {\n",
	 name, Earthmap.geom.width, name, Earthmap.geom.height, name);
	 
  count = 0;
  for (j=0; j<(1+Earthmap.geom.width/8)*Earthmap.geom.height; j++) {
     u = Earthmap.bits[j];
     a = ((int) u)/16;
     b = ((int) u)%16;
     if (a<10) a = '0' + a; else a = a + 'a' - 10;
     if (b<10) b = '0' + b; else b = b + 'a' - 10;
     if (count == 0) printf("  ");
     printf("0x%c%c,", a, b);
     count = (++count)%12;
     printf((count == 0)? "\n" : " ");
  }
  printf("};\n");
}

void
usage()
{
  fprintf(stderr, "Usage: mapgen [options]\n"
   "where options are:\n"
   "   -width number       Select width=number (minimum is 10)\n"
   "   -height number      Select height=number (minimum is 10)\n"
   "   -landcolor string   Set land color\n"
   "   -watercolor string  Set water color\n"
   "   -arcticcolor string Set arctic area color\n"
   "   -xpm                Output as xpm (X Pixmap) data\n"
   "   -xbm                Output as xbm (X Bitmap) data\n"
   "   -name string        Select string as internal name of pix/bitmap data\n"
   "   -fillmode 0,1,2     Filling mode (0=thick contour, 1=contour, 2=fill)\n"
   "   -help               Help message\n\n"
   "A command as `./mapgen | xv - &` lets the map be viewed with xv,\n"
   "`./mapgen > earthmap.xpm` will instead produce an image file.\n\n"
  );
}

int
main (int argc, char ** argv)
{
  int i, output=1;

  strcpy(LandColor.name, "Chartreuse2");
  strcpy(WaterColor.name, "RoyalBlue");
  strcpy(ArcticColor.name, "LemonChiffon");
  Earthmap.geom.width = 720;
  Earthmap.geom.height = 360;

  for (i=1; i<argc; i++) {
    if ((!strcmp(argv[i],"-h") || !strcmp(argv[i],"-help"))) {
        usage();
	exit(0);
    } else
    if (!strcmp(argv[i], "-xpm")) {
        output=1;
	fill_mode=2;
	invert=0;
    } else
  if (!strcmp(argv[i], "-xbm")) {
        output=0;
	fill_mode=1;
	invert=1;
  }
    else
    if (!strcmp(argv[i],"-watercolor") && i<argc-1)
        strncpy(WaterColor.name, argv[++i], 80);
    else
    if (!strcmp(argv[i],"-landcolor") && i<argc-1)
        strncpy(LandColor.name, argv[++i], 80);
    else
    if (!strcmp(argv[i],"-arcticcolor") && i<argc-1)
        strncpy(ArcticColor.name, argv[++i], 80);
    else
    if (!strcmp(argv[i],"-fillmode") && i<argc-1) {
        fill_mode = atoi (argv[++i]);
	if (fill_mode<0) fill_mode = 0;
	if (fill_mode>2) fill_mode = 2;
    } else
    if (!strcmp(argv[i],"-width") && i<argc-1) {
        Earthmap.geom.width = atoi (argv[++i]);
	if (Earthmap.geom.width<10) Earthmap.geom.width = 10;
    } else
      if (!strcmp(argv[i],"-height") && i<argc-1) {
        Earthmap.geom.height = atoi (argv[++i]);
        if (Earthmap.geom.height<10) Earthmap.geom.height = 10;
    } else
    if (!strcmp(argv[i],"-name") && i<argc-1)
        strncpy(name, argv[++i], 250);
    else {
      fprintf(stderr, "mapgen: incorrect options !\n");
      usage();
      exit(1);
    }
  }
 
  free_data = 0;
  makePixmap(&Earthmap);
  
  if (output==XPM)
    print_xpm();
  else
    print_xbm();
  freeData();

  return 0;
}
