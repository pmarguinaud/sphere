#include <stdio.h>
#include <stdlib.h>

#include "bmp.h"

static int S4 (unsigned char * h)
{
  return h[0] + 256 * (h[1] + 256 * (h[2] + 256 * h[3]));
}
  
  
void bmp (const char * file, std::vector<unsigned char> * rgb, int * width, int * height)
{

  unsigned char h[54];
  
  FILE * fp = fopen (file, "r");
  fread (h, sizeof (h), 1, fp);
  
  int ioff = S4 (&h[10]); 
  int ncol = S4 (&h[18]);
  int nrow = S4 (&h[22]);
  
  rgb->resize (3 * ncol * nrow);
  
  fseek (fp, ioff, SEEK_SET);

  fread (&(*rgb)[0], 3 * ncol * nrow, 1, fp);

  for (int i = 0; i < ncol * nrow; i++)
    {
      unsigned char r = (*rgb)[3*i+2];
      unsigned char g = (*rgb)[3*i+1];
      unsigned char b = (*rgb)[3*i+0];
      (*rgb)[3*i+0] = r;
      (*rgb)[3*i+1] = g;
      (*rgb)[3*i+2] = b;
    }
  
  fclose (fp);

  *width = ncol;
  *height = nrow;
}
  
