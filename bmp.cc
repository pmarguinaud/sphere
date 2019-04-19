#include <stdio.h>
#include <stdlib.h>

static int S4 (unsigned char * h)
{
  return h[0] + 256 * (h[1] + 256 * (h[2] + 256 * h[3]));
}
  
  
void bmp (const char * file, unsigned char ** rgb, int * width, int * height)
{

  unsigned char h[54];
  
  FILE * fp = fopen (file, "r");
  fread (h, sizeof (h), 1, fp);
  
  int ioff = S4 (&h[10]); 
  int ncol = S4 (&h[18]);
  int nrow = S4 (&h[22]);
  
  printf (" ioff = %d\n", ioff);
  printf (" ncol = %d\n", ncol);
  printf (" nrow = %d\n", nrow);
  
  *rgb = (unsigned char *)malloc (3 * ncol * nrow * sizeof (unsigned char));
  
  fseek (fp, ioff, SEEK_SET);

  fread ((*rgb), 3 * ncol * nrow, 1, fp);
  
  fclose (fp);

  *width = ncol;
  *height = nrow;
}
  
