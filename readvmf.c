/*
 * readvmf.c
 * Jean-Pierre Demailly
 * February 2001
 *
 * Copyright (C) 2001 by Jean-Pierre Demailly <demailly@ujf-grenoble.fr>
 *
 * Permission to use, copy, modify and freely distribute these data for
 * non-commercial and not-for-profit purposes is hereby granted
 * without fee, provided that both the above copyright notice and this
 * permission notice appear in all copies and in supporting
 * documentation.
 *
 * The author makes no representations about the suitability of this
 * software for any purpose. It is provided "as is" without express or
 * implied warranty.
 *
 * THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS,
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, INDIRECT
 * OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <stdio.h>
#include <math.h>

#include "sunclock.h"

#define grid(x,y) GRID[x*map->geom.height+y]

extern char *ProgName;
extern int invert, mono;

extern Display *	dpy;
extern int		scr;
extern int              color_depth;
extern int              color_pad;
extern int              bytes_per_pixel;
extern int              color_alloc_failed;
extern int              fill_mode;
extern int              verbose;

static struct Sundata *map;
static Pixel *colors;
static int *palette;

static char buf[128];	/* Buffer to hold input lines */
static int *GRID;     /* Pointer to grid data */
static int uu, cc, vv, full;
static int max_palette;

char *
salloc(nbytes)
register int			nbytes;
{
	register char *		p;

	p = malloc((unsigned)nbytes);
	if (p == (char *)NULL) {
		fprintf(stderr, "%s: %d bytes required; out of memory\n", 
                      ProgName, nbytes);
		exit(1);
	}
	return (p);
}

void
plotdata(u, v, s)
int u, v, s;
{
  int c, w;

  c = u;
  if (c<0) c+=map->geom.width;
  if (c>=map->geom.width) c-=map->geom.width;

  if (fill_mode==0) {
    if (s>0) grid(c,v) = s * 65536;
    if (s<0) grid(c,v) = -s * 65536;
    return;
  }

  if (s>0) grid(c,v) |= s;
  if (s<0) grid(c,v) |= -s;

  s *= 65536;

  if (u>uu)
     for (w=0; w<=v; w++) grid(c,w) += s;
  if (u<uu)
     for (w=0; w<=vv; w++) grid(cc,w) -= s;
  if (u==uu && v>vv)
     for (w=vv+1; w<=v; w++) grid(c,w) += s;
  if (u==uu && v<vv)
     for (w=v+1; w<=vv; w++) grid(c,w) -= s;

  uu = u;
  cc = c;
  vv = v;
}

int check(i,j, which)
int i, j, which;
{
#define MASK -65536
   which *= 65536;
   if ((grid((i+map->geom.width-1)%map->geom.width,j)&MASK)!=which) return 1;
   if ((grid((i+1)%map->geom.width,j)&MASK)!=which) return 1;
   if (j>1 && (grid(i,j-1)&MASK)!=which) return 1;
   if (j<map->geom.height-1 && (grid(i,j+1)&MASK)!=which) return 1;
   if (full) {
     if (j>1 && (grid((i+map->geom.width-1)%map->geom.width,j-1)&MASK)!=which) return 1;
     if (j>1 && (grid((i+1)%map->geom.width,j-1)&MASK)!=which) return 1;
     if (j<map->geom.height-1 && (grid((i+map->geom.width-1)%map->geom.width,j+1)&MASK)!=which) return 1;
     if (j<map->geom.height-1 && (grid((i+1)%map->geom.width,j+1)&MASK)!=which) return 1;
   }
   return 0;
}

void
filterdata()
{
int i, j, t;

    if (fill_mode==0) return;
 
    if (fill_mode==1) {

      for (j=0; j<map->geom.height; j++)
        for (i=0; i<map->geom.width; i++) {
	    t = grid(i,j);
	    full = 1;
	    if ( (t & 16383) && !check(i,j,0) ) {
	       grid(i,j) |= 16384;
	       continue;
	    }
	    full = 0;
	    t = t/65536;
	    if ((t>0) && !check(i,j,t)) grid(i,j) |= 32768; 
        }

      for (j=0; j<map->geom.height; j++)
        for (i=0; i<map->geom.width; i++) {
	    t = grid(i,j);
	    if (t&16384) 
	      grid(i,j) = (t&16383) * 65536;
	    else
	    if (t&32768) 
              grid(i,j) = 0;
	    else
	      grid(i,j) = t&MASK;
        }
      return;
    }

    /* only remains fill_mode = 2 */
    full = 1;

    for (j=0; j<map->geom.height; j++)
      for (i=0; i<map->geom.width; i++)
	  if (check(i,j,0)) grid(i,j) &= MASK;

    for (j=0; j<map->geom.height; j++)
      for (i=0; i<map->geom.width; i++) {
	  t = (grid(i,j) & 32767) * 65536;
	  grid(i,j) |= t;
      }
}

char *
blacknwhite_image()
{
  int i, j, k, l, u;
  char *bits;

  bits = (char *)
        salloc(((map->geom.width+7)/8)*map->geom.height*sizeof(char));

  k = 0;
  for (j=0; j<map->geom.height; j++) {
    l = 1;
    u = 0;
    for (i=0; i<map->geom.width; i++) {
      if (grid(i,j)>=65536) u = u+l;
      l = l+l;
      if (l==256 || i==map->geom.width-1) {
        bits[k] = u;
	u = 0;
	l = 1;
        ++k;
      }
    }
  }
  return bits;
}

XImage *
pixmap_image()
{
  int i, j, k, l, w, h;
  char *c;
  XImage *image;
  Visual visual;

  visual = *DefaultVisual(dpy, scr);

  w = map->geom.width;
  h = map->geom.height;

  image = XCreateImage(dpy,&visual, 
      DefaultDepth(dpy, scr), ZPixmap, 0, NULL, w, h, color_pad, 0);
  if (!image) return NULL;

  bytes_per_pixel = (image->bits_per_pixel/8);
  image->data = (char *) salloc(image->bytes_per_line * h);

  if (color_depth>16)
  for (j=0; j<map->geom.height; j++) {
    c = image->data + j * image->bytes_per_line;
    for (i=0; i<map->geom.width; i++) {
       k = bytes_per_pixel * i;
       l = grid(i,j)/65536;
       if (l>=0 && l<=max_palette) 
           l = palette[l];
       else
	   l = max_palette+1;
#ifdef BIGENDIAN
       c[k+1] = (colors[l] >> 16) & 255;
       c[k+2] = (colors[l] >> 8) & 255;
       c[k+3] = colors[l];
#else
       c[k] = colors[l];
       c[k+1] = (colors[l] >> 8) & 255;
       c[k+2] = (colors[l] >> 16) & 255;
#endif
    }
  }
  else
  if (color_depth>8)
  for (j=0; j<map->geom.height; j++) {
    c = image->data + j * map->geom.width * bytes_per_pixel;
    for (i=0; i<map->geom.width; i++) {
       k = bytes_per_pixel * i;
       l = grid(i,j)/65536;
       if (l>=0 && l<=max_palette) 
           l = palette[l];
       else
	   l = max_palette+1;
#ifdef BIGENDIAN
       c[k] = colors[l] / 256;
       c[k+1] = colors[l] & 255;
#else
       c[k] = colors[l] & 255;
       c[k+1] = colors[l] / 256;
#endif
    }
  }
  else
  for (j=0; j<map->geom.height; j++) {
    c = image->data + j * map->geom.width * bytes_per_pixel;
    for (i=0; i<map->geom.width; i++) {
       k = bytes_per_pixel * i;
       l = grid(i,j)/65536;
       if (l>=0 && l<=max_palette) 
           l = palette[l];
       else
	   l = max_palette+1;
       c[k] = colors[l] & 255;
    }
  }

  return image;
}

char *
getdata(fd)
FILE * fd;
{
  int i, j;
  char c;

 repeat:
  i = fgetc(fd);
  if (i==EOF) return NULL;

  c = (char) i;

  if (c=='#') {
     while (i!=EOF && (char) i!= '\n') i = fgetc(fd);
     if (i==EOF) return NULL;
     goto repeat;
  }

  while(isspace(c)) goto repeat;
  
  j = 0;
  while(!isspace(c) && j<125) {
    buf[j] = c;
    ++j;
    i = fgetc(fd);
    if (i==EOF) break;
    c = (char) i;
  }

  buf[j] = '\0';
  return buf;
}

int
readVMF(path, Context)
char * path;
struct Sundata * Context;
{
  int num_colors, correct;
  int sign, i, j, num, count, u=0, v=0, up, vp;
  int m, min, max, addumin, addvmin, addumax, addvmax, diffu, diffv, sum;
  int ix, iy, ix0=0, iy0=0;
  double theta, phi;
  XColor c, e;
  char *str;
  FILE *fd;

  if (!path) return 1;

  map = Context;
  if (map->geom.width <=10) return 4;
  if (map->geom.height<= 5) return 4;

  if (verbose)
    fprintf(stderr, "Loading vector map %s\n", path);
  fd = fopen(path, "r");
  if (fd == NULL) {
     return 1;
  }

  str = getdata(fd);
  if (!str) {
     fclose(fd);
     return 5;
  }
  Context->ncolors = num_colors = atoi(str);

  str = getdata(fd);
  if (!str) {
     fclose(fd);
     return 5;
  }
  max_palette = atoi(str);

  colors = (long *) salloc(num_colors*sizeof(Pixel));
  palette = (int *) salloc((max_palette+2)*sizeof(int));
  GRID = (int *) salloc(map->geom.width*map->geom.height*sizeof(int));

  for (j=0; j<num_colors; j++) {
    register Status s;
    str = getdata(fd);
    if (!str) goto abort;
    if (!mono) {
      s = XAllocNamedColor(dpy, map->cmap, str, &c, &e);
	if (s != (Status)1) {
		fprintf(stderr, "%s: warning: can't allocate color `%s'\n",
			ProgName, buf);
		color_alloc_failed = 1;
		goto abort;
	}
      colors[j] = c.pixel;
    if (color_depth<=8)
       Context->daypixel[j] = (unsigned char) c.pixel;
#ifdef DEBUG
    printf("%d : %s \n", j, str);
#endif
    }
  }

  str = getdata(fd);
  if (!str) goto abort;
  v = atoi(str);
  if (v<0 || v>=num_colors) goto abort;

  for (j=0; j<=max_palette+1; j++) palette[j] = v;

  while (strcmp(str, ".")) {
    str = getdata(fd);
    if (!str) goto abort;
    if (*str=='.') continue;
    if (*str=='c') {
       v = atoi(str+1);
       if (v<0 || v>=num_colors) goto abort;
    }
    else {
       u = atoi(str);
       if (u<0 || u>max_palette) goto abort;
       palette[u] = v;
    }
#ifdef DEBUG
    printf("%d : %d \n", u, v);
#endif
  }

  str = getdata(fd);
  if (!str) goto abort;
  if (fill_mode)
     correct = atoi(str) * 65536;
  else
     correct = 0;

  for (i=0; i<map->geom.width; i++) 
    for (j=0; j<map->geom.height; j++) grid(i,j) = correct;

  count = 0;

  while(1) {
    str = getdata(fd);
    if (!str) goto abort;
    num = atoi(str);
    if (!num) break;
    str = getdata(fd);
    if (!str) goto abort;
    sign = atoi(str);
#ifdef DEBUG
    fprintf(stderr, "Loop %d, num %d, sign %d\n", count, num, sign);
#endif
    for (j=0; j<=num; j++) {
      if (j<num) {
	str = getdata(fd);
        if (!str) goto abort;
        ix = atoi(str);
	str = getdata(fd);
        if (!str) goto abort;
        iy = atoi(str);
	if (j == 0) {
	  ix0 = ix;
	  iy0 = iy;
	}
      } else {
	ix = ix0;
        iy = iy0;
      }
      theta = 0.5 + ix / 65520.0;
      phi = 0.5 - iy / 65520.0;
      uu = cc = up = u;
      if (cc<0) cc+=map->geom.width;
      if (cc>=map->geom.width) cc-=map->geom.width;
      vv = vp = v;
      u = (int) (theta * (map->geom.width-1));
      v = (int) (phi * (map->geom.height-1));
      if (j) {
#ifdef DEBUG
        fprintf(stderr, "Point %d/%d, Segment %d %d %d %d\n", 
                 j, num, up, vp, u, v);
#endif
        diffu = abs(u-up);
        if (diffu>map->geom.width/2) {
	   if (u>up) 
              u -= map->geom.width;
	   else
	      u += map->geom.width;
           diffu = abs(u-up);
	}
        diffv = abs(v-vp);
        addumin = (u>up)? 1:-1;
        addvmin = (v>vp)? 1:-1;
        if (diffu>diffv) {
	  max = diffu ; 
          min = diffv;
	  addumax = addumin;
	  addvmax = 0;
	} else {
          max = diffv ; 
          min = diffu;
	  addumax = 0;
	  addvmax = addvmin;
	}
	sum = max/2-max;
	for (m=0; m<max; m++) {
          sum = sum + min;
          if (sum>=max/2) {
	      sum -= max;
	      up += addumin;
              vp += addvmin;
	    } else {
	      up += addumax; 
              vp += addvmax;
	    }
          plotdata(up, vp, sign);
	}
#ifdef DEBUG
        if (up!=u || vp!=v) 
	   fprintf(stderr, "Ending up with %d %d !!!\n", up, vp);
#endif
      }
    }
    ++count;
  }

  fclose(fd);
  filterdata();

  if (invert)
     Context->bits = blacknwhite_image();
  else
     Context->xim = pixmap_image();

 abort:
  free(GRID); 
  free(colors);
  free(palette);
  if (color_alloc_failed) return 6;
  if ((invert && Context->bits==NULL) || (!invert && Context->xim==NULL)) 
     return 2;
  else
     return 0;
}
