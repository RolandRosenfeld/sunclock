/* +-------------------------------------------------------------------+ */
/* | This file is derived from                                         | */
/* | Xpaint's readJPEG routines, copyrighted                           | */
/* | by David Koblas (koblas@netcom.com)	        	       | */
/* |								       | */
/* | Permission to use, copy, modify, and to distribute this software  | */
/* | and its documentation for any purpose is hereby granted without   | */
/* | fee, provided that the above copyright notice appear in all       | */
/* | copies and that both that copyright notice and this permission    | */
/* | notice appear in supporting documentation.	 There is no	       | */
/* | representations about the suitability of this software for	       | */
/* | any purpose.  this software is provided "as is" without express   | */
/* | or implied warranty.					       | */
/* |								       | */
/* +-------------------------------------------------------------------+ */

#include <stdio.h>
#include <setjmp.h>
#include <stdlib.h>
#include <jpeglib.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include "sunclock.h"

extern Display *	dpy;
extern Visual		visual;
extern int		scr;
extern int              color_depth;
extern int              color_pad;
extern int              bytes_per_pixel;
extern int              color_alloc_failed;
extern int              verbose;

extern char *           salloc();

struct error_mgr {
  struct jpeg_error_mgr pub;    /* "public" fields */

  jmp_buf setjmp_buffer;        /* for return to caller */
};

typedef struct error_mgr * error_ptr;

void
error_exit (j_common_ptr cinfo)
{
  /* cinfo->err really points to a my_error_mgr struct, so coerce pointer */
  error_ptr err = (error_ptr) cinfo->err;

  /* Always display the message. */
  /* We could postpone this until after returning, if we chose. */
  (*cinfo->err->output_message) (cinfo);

  /* Return control to the setjmp point */
  longjmp(err->setjmp_buffer, 1);
}

int
readJPEG(path, Context)
char *path;
Sundata * Context;
{
#define RANGE 252
    struct jpeg_decompress_struct cinfo;
    struct error_mgr jerr;
    FILE *input_file;
    float ratio;
    int i, j, k, l, m, prev, next, size;
    JSAMPROW scanline[1];
    char *scan, *c;
    unsigned char r, g, b;
    long lr[RANGE], lg[RANGE], lb[RANGE], lnum[RANGE];
    char pix[RANGE];
    XColor xc;

    if ((input_file = fopen(path, "r")) == NULL) return 1;

    cinfo.err = jpeg_std_error(&jerr.pub);
    jerr.pub.error_exit = error_exit;

    if (setjmp(jerr.setjmp_buffer)) {
	/* If we get here, the JPEG code has signaled an error.
	 * We need to clean up the JPEG object, close the input file,
	 * and return.
	 */
        jpeg_destroy_decompress(&cinfo);
	fclose(input_file);
        Context->xim = 0;
	return 2;
    }

    jpeg_create_decompress(&cinfo);
    jpeg_stdio_src(&cinfo, input_file);
    jpeg_read_header(&cinfo, TRUE);

    if (cinfo.jpeg_color_space == JCS_GRAYSCALE) return 3;

    ratio = ((float) cinfo.image_width/(float) Context->geom.width + 
             (float) cinfo.image_height/(float) Context->geom.height )/1.8;
    if (ratio>=8.0) 
      cinfo.scale_denom = 8;
    else
    if (ratio>=4.0) 
      cinfo.scale_denom = 4;
    else
    if (ratio>=2.0) 
      cinfo.scale_denom = 2;
    else
      cinfo.scale_denom = 1;

    jpeg_start_decompress(&cinfo);

    Context->xim = XCreateImage(dpy, &visual, 
              DefaultDepth(dpy, scr), ZPixmap, 0, NULL, 
              Context->geom.width, Context->geom.height, color_pad, 0);
    XFlush(dpy);
    if (!Context->xim) return 4;

    bytes_per_pixel = (Context->xim->bits_per_pixel/8);
    size = Context->xim->bytes_per_line * Context->geom.height;
    Context->xim->data = (char *) salloc(size);
    scan = (char *) salloc(3 * cinfo.output_width * sizeof(char));

    if (verbose)
       fprintf(stderr, "Loading %s\n%d %d  -->  %d %d,    %d bytes per pixel\n"
            "Rescaling JPEG data by 1/%d\n", 
	    path,
            cinfo.image_width, cinfo.image_height, 
            Context->geom.width, Context->geom.height, 
            bytes_per_pixel, cinfo.scale_denom);

    prev = -1;
    scanline[0] = (JSAMPROW) scan;

    if (color_depth<=8) 
      for (l=0; l<RANGE; l++) {
        lr[l] = lg[l] = lb[l] = lnum[l] = 0;
      }

    while (cinfo.output_scanline < cinfo.output_height) {
	(void) jpeg_read_scanlines(&cinfo, scanline, (JDIMENSION) 1);
	if (cinfo.output_scanline >= cinfo.output_height) 
           next = Context->geom.height-1;
	else
	   next = ((2*cinfo.output_scanline-1) * 
                    Context->geom.height)/(2*cinfo.output_height);
	for (l = prev+1; l<= next; l++) {
	  c = Context->xim->data + l * Context->xim->bytes_per_line;
	  k = 0;
	  if (color_depth>16) {
#ifdef BIGENDIAN
            k = bytes_per_pixel - 3;
#endif
            for (i = 0; i < Context->geom.width; i++) {
    	       j = 3 * ((i * cinfo.output_width)/Context->geom.width);
#ifdef BIGENDIAN
	       c[k] = scan[j];
               c[k+1] = scan[j+1];
	       c[k+2] = scan[j+2];
#else
	       c[k] = scan[j+2];
               c[k+1] = scan[j+1];
	       c[k+2] = scan[j];
#endif
	       k +=  bytes_per_pixel;
	    }
          } else
	  if (color_depth==16)
             for (i = 0; i < Context->geom.width; i++) {
	       j = 3 * ((i * cinfo.output_width)/Context->geom.width);
	       r = scan[j];
	       g = scan[j+1];
	       b = scan[j+2];
	    /* blue  c[k] = 31;  c[k+1] = 0;
	       green c[k] = 224  (low weight) c[k+1] = 7 (high weight)
	       red   c[k] = 0;   c[k+1] = 248; */
#ifdef BIGENDIAN
               c[k+1] = (((b&248)>>3) | ((g&28)<<3));
	       c[k] = (((g&224)>>5) | (r&248));
#else
               c[k] = (((b&248)>>3) | ((g&28)<<3));
	       c[k+1] = (((g&224)>>5) | (r&248));
#endif
	       k += 2;
	     }
          else
	  if (color_depth==15)
             for (i = 0; i < Context->geom.width; i++) {
	       j = 3 * ((i * cinfo.output_width)/Context->geom.width);
	       r = scan[j];
	       g = scan[j+1];
	       b = scan[j+2];
	    /* blue  c[k] = 31;  c[k+1] = 0;
	       green c[k] = 224  (low weight) c[k+1] = 7 (high weight)
	       red   c[k] = 0;   c[k+1] = 248; */
#ifdef BIGENDIAN
               c[k+1] = (b&248)>>3 | (g&56)<<2;
	       c[k] = (g&192)>>6 | (r&248)>>1;
#else
               c[k] = (b&248)>>3 | (g&56)<<2;
	       c[k+1] = (g&192)>>6 | (r&248)>>1;
#endif
	       k += 2;
	     }
	  else {
             for (i = 0; i < Context->geom.width; i++) {
	       j = 3 * ((i * cinfo.output_width)/Context->geom.width);
	       r = scan[j];
	       g = scan[j+1];
	       b = scan[j+2];
	       /* c[k] = (((7*g)/256)<<4) | ((r&192)>>4) | ((b&192)>>6); */
	       c[k] = (unsigned char) 
                      (((7*g)/256)*36)+(((6*r)/256)*6)+((6*b)/256);
	       m = (unsigned char)c[k];
               lr[m] += r;
               lg[m] += g;
               lb[m] += b;
               lnum[m] += 1;
	       k += 1;
	     }
	  }
	}
	prev = next;
    }
    free(scan);

    jpeg_finish_decompress(&cinfo);
    jpeg_destroy_decompress(&cinfo);

    fclose(input_file);

    if (jerr.pub.num_warnings > 0) {	
	longjmp(jerr.setjmp_buffer, 1);
    }

    if (color_depth<=8) {
      xc.flags = DoRed | DoGreen | DoBlue;
      k = 0;
      for (m=0; m<RANGE; m++) if (lnum[m]) {
        xc.red = (lr[m]/lnum[m])*257;
        xc.green = (lg[m]/lnum[m])*257;
        xc.blue = (lb[m]/lnum[m])*257;
	if (!XAllocColor(dpy, Context->cmap, &xc)) color_alloc_failed = 1;
	pix[m] = (char) xc.pixel;
	Context->daypixel[k] = (unsigned char) xc.pixel;
	++k;
      }
      Context->ncolors = k;
      for (i=0; i<size; i++) 
	 Context->xim->data[i] = pix[(unsigned char)Context->xim->data[i]];
    }

    return 0;
}

int
testJPEG(char *file)
{
    unsigned char buf[2];
    FILE *fd = fopen(file, "r");
    int ret = 0;

    if (fd == NULL)
	return 0;

    if (2 == fread(buf, sizeof(char), 2, fd)) {
	if (buf[0] == 0xff && buf[1] == 0xd8)
	    ret = 1;
    }
    fclose(fd);

    return ret;
}
