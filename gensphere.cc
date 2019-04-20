#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "gensphere.h"

#define MODULO(x, y) ((x)%(y))
#define JDLON(JLON1, JLON2) (MODULO ((JLON1) - 1, (iloen1)) * (iloen2) - MODULO ((JLON2) - 1, (iloen2)) * (iloen1))
#define JNEXT(JLON, ILOEN) (1 + MODULO ((JLON), (ILOEN)))


#define PRINT(a,b,c) \
  do {                                                                                          \
    if (pass == 1)                                                                              \
      {                                                                                         \
        (*ind)++;                                                                               \
      }                                                                                         \
    else if (pass == 2)                                                                         \
      {                                                                                         \
        *(ind++) = (a)-1; *(ind++) = (b)-1; *(ind++) = (c)-1;                                   \
      }                                                                                         \
  } while (0)

static 
void glgauss (const int Nj, const int pl[], int pass, unsigned int * ind, int nstripe, int indcnt[])
{
  int iglooff[Nj];
  
  iglooff[0] = 0;
  for (int jlat = 2; jlat <= Nj-1; jlat++)
     iglooff[jlat-1] = iglooff[jlat-2] + pl[jlat-2];

  if (pass == 1)
    *ind = 0;

  for (int jlat = 1; jlat <= Nj-1; jlat++)
    {
      int iloen1 = pl[jlat - 1];
      int iloen2 = pl[jlat + 0];
      int jglooff1 = iglooff[jlat-1] + 0;
      int jglooff2 = iglooff[jlat-1] + iloen1;


      if (iloen1 == iloen2) 
        {
          for (int jlon1 = 1; jlon1 <= iloen1; jlon1++)
            {
              int jlon2 = jlon1;
              int ica = jglooff1 + jlon1;
              int icb = jglooff2 + jlon2;
              int icc = jglooff2 + JNEXT (jlon2, iloen2);
              int icd = jglooff1 + JNEXT (jlon1, iloen1);
	      PRINT (ica, icb, icc);
	      PRINT (icc, icd, ica);
            }
        }
      else 
        {
          int jlon1 = 1;
          int jlon2 = 1;
          for (;;)
            {
              int ica = 0, icb = 0, icc = 0;

  
              int idlonc = JDLON (jlon1, jlon2);
              int jlon1n = JNEXT (jlon1, iloen1);
              int jlon2n = JNEXT (jlon2, iloen2);
              int idlonn = JDLON (jlon1n, jlon2n);

#define AV1 \
  do {                                                                        \
    ica = jglooff1 + jlon1; icb = jglooff2 + jlon2; icc = jglooff1 + jlon1n;  \
    jlon1 = jlon1n;                                                           \
  } while (0)

#define AV2 \
  do {                                                                        \
    ica = jglooff1 + jlon1; icb = jglooff2 + jlon2; icc = jglooff2 + jlon2n;  \
    jlon2 = jlon2n;                                                           \
  } while (0)

              if (idlonc > 0 || ((idlonc == 0) && (idlonn > 0)))
                {
                  if (jlon2n != 1)
                    AV2;
                  else
                    AV1;
                }
              else if (idlonc < 0 || ((idlonc == 0) && (idlonn < 0))) 
                {
                  if (jlon1n != 1)
                    AV1;
                  else
                    AV2;
                }
              else
                {
                  abort ();
                }
         
              PRINT (ica, icb, icc);
             
              if ((jlon1 == 1) && (jlon2 == iloen2)) 
                {
                  ica = jglooff1 + jlon1; icb = jglooff2 + jlon2; icc = jglooff2 + jlon2n;
                  PRINT (ica, icb, icc);
                }
              else if ((jlon1 == iloen1) && (jlon2 == 1)) 
                {
                  ica = jglooff1 + jlon1; icb = jglooff2 + jlon2; icc = jglooff1 + jlon1n;
                  PRINT (ica, icb, icc);
                }
              else
                {
                  continue;
                }
              break;
          }
     
     
        }

    }


}


void gensphere (const int Nj, int * np, float ** xyz, 
                unsigned int * nt, unsigned int ** ind)
{
  int * pl = NULL;
  const int nstripe = 4;
  int indoff[nstripe];

  *xyz = NULL;

  pl = (int *)malloc (sizeof (int) * Nj);

  for (int jlat = 1; jlat <= Nj; jlat++)
    {
      float lat = M_PI * (0.5 - (float)jlat / (float)(Nj + 1));
      float coslat = cos (lat);
      pl[jlat-1] = (2. * Nj * coslat);
    }

  glgauss (Nj, pl, 1, nt, nstripe, indoff);
  *ind = (unsigned int *)malloc (3 * (*nt) * sizeof (unsigned int));
  glgauss (Nj, pl, 2, *ind, nstripe, indoff);

  int v_len = 0;
  for (int jlat = 1; jlat <= Nj; jlat++)
    v_len += pl[jlat-1];
  
  
  *xyz = (float *)malloc (3 * sizeof (float) * v_len);
  *np  = v_len;
  for (int jglo = 0, jlat = 1; jlat <= Nj; jlat++)
    {
      float lat = M_PI * (0.5 - (float)jlat / (float)(Nj + 1));
      float coslat = cos (lat); float sinlat = sin (lat);
      for (int jlon = 1; jlon <= pl[jlat-1]; jlon++, jglo++)
        {
          float lon = 2. * M_PI * (float)(jlon-1) / (float)pl[jlat-1];
          float coslon = cos (lon); float sinlon = sin (lon);
          float radius = 1.0;
          (*xyz)[3*jglo+0] = coslon * coslat * radius;
          (*xyz)[3*jglo+1] = sinlon * coslat * radius;
          (*xyz)[3*jglo+2] =          sinlat * radius;
        }
    }
  
  free (pl);

  
}
