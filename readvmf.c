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
#include <string.h>
#include <math.h>

#include "sunclock.h"

extern char *ProgName;

extern Display *	dpy;
extern Colormap         tmp_cmap;

extern int		scr;
extern int		bigendian;
extern int              color_depth;
extern int              color_pad;
extern int              bytes_per_pixel;
extern int              color_alloc_failed;
extern int              verbose;

static struct Sundata *map;
static int mapwidth;
static int mapheight;

static Pixel *colors;
static int *palette;

static char buf[128];	/* Buffer to hold input lines */
static int *grid;     /* Pointer to grid data */
static int uu, cc, vv, vv1, vv2, full;
static int max_palette;

int screemx(x,y) 
int x, y;
{
if (x<=0 || x>=mapwidth) fprintf(stderr, "Horror x=%d y=%d\n", x, y);
return x;
}

int screemy(x,y) 
int x,y;
{
if (y<0 || y>=mapheight) fprintf(stderr, "Horror y=%d\n", y);
return y;
}

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
  int v1=0, v2=0, c, w, ind;

  c = u;
  if (c<0) c+=map->zoom.width;
  if (c>=map->zoom.width) c-=map->zoom.width;
  c -= map->zoom.dx-1;
  v -= map->zoom.dy;

  if (map->flags.fillmode==0) {
     if (c>=0 && c<=mapwidth && v>=0 && v<mapheight) {
        ind = c*mapheight+v;
        if (s>0) grid[ind] = s * 65536;
        if (s<0) grid[ind] = -s * 65536;
     }
     return;
  }

  if (c>=0 && c<=mapwidth) {
     ind = c*mapheight;

     if (v>=0 && v<mapheight) {
        if (s>0) grid[ind+v] |= s;
        if (s<0) grid[ind+v] |= -s;
     }

     s *= 65536;
      
     if (v<0) v1=0; else v1=v+1;
     if (v>=mapheight) v2=mapheight-1; else v2=v;

     if (u==uu) {
        if (v>vv)
           for (w=vv1; w<=v2; w++) grid[ind+w] += s;
        if (v<vv)
           for (w=v1; w<=vv2; w++) grid[ind+w] -= s;
     }
     if (u>uu)
        for (w=0; w<=v2; w++) grid[ind+w] += s;
  }

  if (u<uu && cc>=0 && cc<=mapwidth) {
     ind = cc*mapheight;
     for (w=0; w<=vv2; w++) grid[ind+w] -= s;
  }

  uu = u;
  cc = c;
  vv = v;
  vv1 = v1;
  vv2 = v2;
}

int check(i, j, which)
int i, j, which;
{
#define MASK -65536
   int ind, indp, inds;

   which *= 65536;

   ind = i*mapheight+j;
   if (j>0 && (grid[ind-1]&MASK)!=which) return 1;
   if (j<mapheight-1 && (grid[ind+1]&MASK)!=which) return 1;

   indp = (i-1)*mapheight+j;
   if (i>0 && (grid[indp]&MASK)!=which) return 1;

   inds = (i+1)*mapheight+j;
   if (i<mapwidth && (grid[inds]&MASK)!=which) return 1;

   if (full) {
     if (j>0) {
        if (i>0 && (grid[indp-1]&MASK)!=which) return 1;
        if (i<mapwidth && (grid[inds-1]&MASK)!=which) return 1;
     }
     if (j<mapheight-1) {
        if (i>0 && (grid[indp+1]&MASK)!=which) return 1;
        if (i<mapwidth && (grid[inds+1]&MASK)!=which) return 1;
     }
   }
   return 0;
}

void
filterdata()
{
int i, j, t, ind;

    if (map->flags.fillmode==0) return;

    if (map->flags.fillmode==1) {

      for (j=0; j<mapheight; j++)
        for (i=0; i<=mapwidth; i++) {
            ind = i*mapheight+j;
	    t = grid[ind];
	    full = 1;
	    if ( (t & 16383) && !check(i,j,0) ) {
	       grid[ind] |= 16384;
	       continue;
	    }
	    full = 0;
	    t = t/65536;
	    if ((t>0) && !check(i,j,t)) grid[ind] |= 32768; 
        }

      for (j=0; j<mapheight; j++)
        for (i=0; i<=mapwidth; i++) {
            ind = i*mapheight+j;
	    t = grid[ind];
	    if (t&16384) 
	      grid[ind] = (t&16383) * 65536;
	    else
	    if (t&32768) 
              grid[ind] = 0;
	    else
	      grid[ind] = t&MASK;
        }
      goto correct;
    }

    /* only remains fillmode = 2 */
    full = 1;

    for (j=0; j<mapheight; j++)
      for (i=0; i<=mapwidth; i++)
	  if (check(i,j,0)) grid[i*mapheight+j] &= MASK;

    for (j=0; j<mapheight; j++)
      for (i=0; i<=mapwidth; i++) {
          ind = i*mapheight+j;
	  t = (grid[ind] & 32767) * 65536;
	  grid[ind] |= t;
      }

 correct:
    if (map->zoom.width > map->geom.width && map->zoom.dx == 0) {
      for (j=0; j<mapheight; j++) grid[j+mapheight] = grid[j+2*mapheight];
    }
}

char *
blacknwhite_image()
{
  int i, j, k, l, u;
  char *bits;

  bits = (char *)
        salloc(((mapwidth+7)/8)*mapheight*sizeof(char));

  k = 0;
  for (j=0; j<mapheight; j++) {
    l = 1;
    u = 0;
    for (i=0; i<mapwidth; i++) {
      if (grid[(i+1)*mapheight+j]<65536) u = u+l;
      l = l+l;
      if (l==256 || i==mapwidth-1) {
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

  w = mapwidth;
  h = mapheight;

  image = XCreateImage(dpy,&visual, 
      DefaultDepth(dpy, scr), ZPixmap, 0, NULL, w, h, color_pad, 0);
  if (!image) return NULL;

  bytes_per_pixel = (image->bits_per_pixel/8);
  image->data = (char *) salloc(image->bytes_per_line * h);

  if (color_depth>16)
  for (j=0; j<mapheight; j++) {
    c = image->data + j * image->bytes_per_line;
    for (i=0; i<mapwidth; i++) {
       k = bytes_per_pixel * i;
       l = grid[(i+1)*mapheight+j]/65536;
       if (l>=0 && l<=max_palette) 
           l = palette[l];
       else
	   l = max_palette+1;
       if (bigendian) {
          c[k+1] = (colors[l] >> 16) & 255;
          c[k+2] = (colors[l] >> 8) & 255;
          c[k+3] = colors[l];
       } else {
          c[k] = colors[l];
          c[k+1] = (colors[l] >> 8) & 255;
          c[k+2] = (colors[l] >> 16) & 255;
       }
    }
  }
  else
  if (color_depth>8)
  for (j=0; j<mapheight; j++) {
    c = image->data + j * mapwidth * bytes_per_pixel;
    for (i=0; i<mapwidth; i++) {
       k = bytes_per_pixel * i;
       l = grid[(i+1)*mapheight+j]/65536;
       if (l>=0 && l<=max_palette) 
           l = palette[l];
       else
	   l = max_palette+1;
       if (bigendian) {
          c[k] = colors[l] / 256;
          c[k+1] = colors[l] & 255;
       } else {
          c[k] = colors[l] & 255;
          c[k+1] = colors[l] / 256;
       }
    }
  }
  else
  for (j=0; j<mapheight; j++) {
    c = image->data + j * mapwidth * bytes_per_pixel;
    for (i=0; i<mapwidth; i++) {
       k = bytes_per_pixel * i;
       l = grid[(i+1)*mapheight+j]/65536;
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
  int num_colors, correct, maxgrid;
  int color, i, j, num, count, u=0, v=0, up, vp;
  int m, min, max, addumin, addvmin, addumax, addvmax, diffu, diffv, sum;
  int ix, iy, ix0=0, iy0=0;
  double theta, phi;
  XColor c, e;
  char *str;
  FILE *fd;

  if (!path) return 1;

  map = Context;
  mapwidth = Context->geom.width;
  if (mapwidth <=10) return 4;
  mapheight = Context->geom.height;
  if (mapheight<= 5) return 4;

  if (verbose)
    fprintf(stderr, "Loading vector map %s\n", path);
  fd = fopen(path, "r");
  if (fd == NULL) {
     return 1;
  }

  str = fgets(buf, 120, fd);
  if (!str || strncmp(buf, "%!VMF", 5)) {
     fclose(fd);
     return 5;
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
  maxgrid = (mapwidth+1)*mapheight;
  grid = (int *) salloc(maxgrid*sizeof(int));

  for (j=0; j<num_colors; j++) {
    register Status s;
    str = getdata(fd);
    if (!str) goto abort;
    if (!map->flags.mono) {
      s = XAllocNamedColor(dpy, tmp_cmap, str, &c, &e);
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
  if (map->flags.fillmode)
     correct = atoi(str) * 65536;
  else
     correct = 0;

  for (i=0; i<maxgrid; i++) grid[i] = correct;

  count = 0;

  while(1) {
    str = getdata(fd);
    if (!str) goto abort;
    num = atoi(str);
    if (!num) break;
    str = getdata(fd);
    if (!str) goto abort;
    color = atoi(str);
#ifdef DEBUG
    fprintf(stderr, "Loop %d, num %d, color %d\n", count, num, color);
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
      theta = 0.5 + ix / 65520.001;
      phi = 0.5 - iy / 65520.001;
      cc = uu = up = u;

      if (cc<0) cc+=map->zoom.width;
      if (cc>=map->zoom.width) cc-=map->zoom.width;      
      cc -= map->zoom.dx-1;

      vp = v;
      vv = v - map->zoom.dy;
      vv1 = vv + 1;
      if (vv1<0) vv1 = 0;
      vv2 = vv;
      if (vv2>=mapheight) vv2 = mapheight-1;

      u = (int) (theta * (double) map->zoom.width);
      v = (int) (phi * (double) map->zoom.height);
      if (j) {
#ifdef DEBUG
        fprintf(stderr, "Point %d/%d, Segment %d %d %d %d\n", 
                 j, num, up, vp, u, v);
#endif
        diffu = abs(u-up);
        if (diffu>map->zoom.width/2) {
	   if (u>up) 
              u -= map->zoom.width;
	   else
	      u += map->zoom.width;
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
          plotdata(up, vp, color);
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

  if (map->flags.mono)
     Context->bits = blacknwhite_image();
  else
     Context->xim = pixmap_image();

 abort:
  free(grid); 
  free(colors);
  free(palette);
  if (color_alloc_failed) return 6;
  if ((map->flags.mono && Context->bits==NULL) || 
      (!map->flags.mono && Context->xim==NULL)) 
     return 2;
  else
     return 0;
}
