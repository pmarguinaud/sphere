#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <eccodes.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>

#include "gensphere.h"

#define MODULO(x, y) ((x)%(y))
#define JDLON(JLON1, JLON2) (MODULO ((JLON1) - 1, (iloen1)) * (iloen2) - MODULO ((JLON2) - 1, (iloen2)) * (iloen1))
#define JNEXT(JLON, ILOEN) ((JLON) == (ILOEN) ? 1 : (JLON)+1)
#define JPREV(JLON, ILOEN) ((JLON)-1 > 0 ? (JLON)-1 : (ILOEN))


static void process_lat (int jlat, int iloen1, int iloen2, 
                         int jglooff1, int jglooff2,
			 unsigned int ** p_inds_strip, int dir)  
{
// iloen1 > iloen2

  int jlon1 = 1;
  int jlon2 = 1;
  bool turn = false;
  int av1 = 0, av2 = 0;

  unsigned int * inds_strip = *p_inds_strip;
  
  for (;;)
    {
      int ica = 0, icb = 0, icc = 0;
  
      int jlon1n = dir > 0 ? JNEXT (jlon1, iloen1) : JPREV (jlon1, iloen1);
      int jlon2n = dir > 0 ? JNEXT (jlon2, iloen2) : JPREV (jlon2, iloen2);

#define AV1 \
  do {                                                                        \
    ica = jglooff1 + jlon1; icb = jglooff2 + jlon2; icc = jglooff1 + jlon1n;  \
    jlon1 = jlon1n;                                                           \
    turn = turn || jlon1 == 1;                                                \
    av1++; av2 = 0;                                                           \
  } while (0)

#define AV2 \
  do {                                                                        \
    ica = jglooff1 + jlon1; icb = jglooff2 + jlon2; icc = jglooff2 + jlon2n;  \
    jlon2 = jlon2n;                                                           \
    turn = turn || jlon2 == 1;                                                \
    av2++; av1 = 0;                                                           \
  } while (0)

      int idlonc = dir * JDLON (jlon1, jlon2);
      int idlonn;
      if ((jlon1n == 1) && (jlon2n != 1))
        idlonn = +1;
      else if ((jlon1n != 1) && (jlon2n == 1))
        idlonn = -1;
      else 
        idlonn = dir * JDLON (jlon1n, jlon2n);
      
      if (idlonn > 0 || ((idlonn == 0) && (idlonc > 0)))
        AV2;
      else if (idlonn < 0 || ((idlonn == 0) && (idlonc < 0))) 
        AV1;
      else
        abort ();
      
#define RS2 \
  do { \
    *(inds_strip++) = 0xffffffff;   \
    *(inds_strip++) = ica-1;        \
    *(inds_strip++) = icb-1;        \
    *(inds_strip++) = icc-1;        \
  } while (0)

      if (idlonc == 0)
        {
          if (av2)
            {
              abort ();
            }
          else if (av1)
            {
              RS2;
            }
        }
      else if (av2 > 1)
        {
          abort ();
        }
      else if (av1 > 1)
        {
          RS2;
        }
      else
        {
          if (inds_strip)
          *(inds_strip++) = icc-1;
        }
      
      if (turn)
        {
          if (jlon1 == 1)
            while (jlon2 != 1)
              {
                abort ();
              }
          else if (jlon2 == 1)
            while (jlon1 != 1)
              {
                int jlon1n = dir > 0 ? JNEXT (jlon1, iloen1) : JPREV (jlon1, iloen1);
                AV1;
                RS2;
              }
          break;
        }

    }


  *p_inds_strip = inds_strip;
}
  

#undef AV1
#undef AV2
#undef RS1
#undef RS2

static 
void glgauss (const long int Nj, const long int pl[], 
              unsigned int * ind_strip,
              const int nstripe, int ind_stripcnt[], 
	      int ind_stripcnt_per_lat[], int ind_stripoff_per_lat[], 
	      bool doopenmp = false)
{
  int iglooff[Nj];
  int ind_stripcntoff[nstripe];
  
  iglooff[0] = 0;
  for (int jlat = 2; jlat <= Nj; jlat++)
    iglooff[jlat-1] = iglooff[jlat-2] + pl[jlat-2];

  ind_stripcntoff[0] = 0;
  for (int istripe = 1; istripe < nstripe; istripe++)
    ind_stripcntoff[istripe] = ind_stripcntoff[istripe-1] + ind_stripcnt[istripe-1];

#pragma omp parallel for if (doopenmp)
  for (int istripe = 0; istripe < nstripe; istripe++)
    {
      int jlat1 = 1 + ((istripe + 0) * (Nj-1)) / nstripe;
      int jlat2 = 0 + ((istripe + 1) * (Nj-1)) / nstripe;

      for (int jlat = jlat1; jlat <= jlat2; jlat++)
        {
          int iloen1 = pl[jlat - 1];
          int iloen2 = pl[jlat + 0];
          int jglooff1 = iglooff[jlat-1] + 0;
          int jglooff2 = iglooff[jlat-1] + iloen1;
          unsigned int * inds_strip = ind_strip + ind_stripoff_per_lat[jlat-1];


          if (iloen1 == iloen2) 
            {
              *(inds_strip++) = jglooff1;
              *(inds_strip++) = jglooff2;
   
              for (int jlon1 = 1; jlon1 <= iloen1; jlon1++)
                {
                  int jlon2 = jlon1;
                  int ica = jglooff1 + jlon1;
                  int icb = jglooff2 + jlon2;
                  int icc = jglooff2 + JNEXT (jlon2, iloen2);
                  int icd = jglooff1 + JNEXT (jlon1, iloen1);
                  *(inds_strip++) = icd-1;
                  *(inds_strip++) = icc-1;
                }
            }
          else if (iloen1 > iloen2)
            {
              process_lat (jlat, iloen1, iloen2, jglooff1, jglooff2, &inds_strip, +1);
            }
          else if (iloen1 < iloen2)
            {
              process_lat (jlat, iloen2, iloen1, jglooff2, jglooff1, &inds_strip, -1);
            }

          unsigned int * inds_strip_max = ind_strip 
		                        + ind_stripoff_per_lat[jlat-1] 
		                        + ind_stripcnt_per_lat[jlat-1];

          if (inds_strip >= inds_strip_max)
            abort ();

          for (; inds_strip < inds_strip_max; inds_strip++)
            *inds_strip = 0xffffffff;

        }

    }

}
#undef MODULO

void gensphere (geom_t * geom, int * np, unsigned short ** lonlat, 
                unsigned int * nt, float ** F,
                const std::string & file)
{
  const int nstripe = 16;
  int indcnt[nstripe];
  int ind_stripcnt[nstripe];

  *lonlat = NULL;

  FILE * in = fopen (file.c_str (), "r");

  int err = 0;
  size_t v_len = 0;
  codes_handle * h = codes_handle_new_from_file (0, in, PRODUCT_GRIB, &err);

  size_t pl_len;
  codes_get_long (h, "Nj", &geom->Nj);
  codes_get_size (h, "pl", &pl_len);

  geom->pl = (long int *)malloc (sizeof (long int) * pl_len);
  codes_get_long_array (h, "pl", geom->pl, &pl_len);

  fclose (in);



  *nt = 0;
  for (int jlat = 1; jlat < geom->Nj; jlat++)
    *nt += geom->pl[jlat-1] + geom->pl[jlat];

  geom->ind_stripcnt_per_lat = (int *)malloc ((geom->Nj + 1) * sizeof (int));
  geom->ind_stripoff_per_lat = (int *)malloc ((geom->Nj + 1) * sizeof (int));

  geom->ind_stripoff_per_lat[0] = 0;
  for (int jlat = 1; jlat <= geom->Nj+1; jlat++)
    {
      if (jlat <= geom->Nj)
        {
          int len = geom->pl[jlat-1] + geom->pl[jlat] + 4 * (2 + abs (geom->pl[jlat-1] - geom->pl[jlat]));
          geom->ind_stripcnt_per_lat[jlat-1] = len;
        }
      if (jlat > 1)
        geom->ind_stripoff_per_lat[jlat-1] = geom->ind_stripcnt_per_lat[jlat-2] + geom->ind_stripoff_per_lat[jlat-2];
//    printf (" %8d -> %8d, %8d\n",
//            jlat-1, geom->ind_stripoff_per_lat[jlat-1], geom->ind_stripcnt_per_lat[jlat-1]);
    }
    

  for (int istripe = 0; istripe < nstripe; istripe++)
    {
      int jlat1 = 1 + ((istripe + 0) * (geom->Nj-1)) / nstripe;
      int jlat2 = 0 + ((istripe + 1) * (geom->Nj-1)) / nstripe;
      indcnt[istripe] = 0;
      for (int jlat = jlat1; jlat <= jlat2; jlat++)
        indcnt[istripe] += geom->pl[jlat-1] + geom->pl[jlat];
      ind_stripcnt[istripe] = 0;
      for (int jlat = jlat1; jlat <= jlat2; jlat++)
        ind_stripcnt[istripe] += geom->ind_stripcnt_per_lat[jlat-1];
    }
  geom->ind_strip_size = 0;
  for (int jlat = 1; jlat <= geom->Nj; jlat++)
    geom->ind_strip_size += geom->ind_stripcnt_per_lat[jlat-1];
  
  v_len = 0;
  for (int jlat = 1; jlat <= geom->Nj; jlat++)
    v_len += geom->pl[jlat-1];
  *np  = v_len;
  
  bool do_ind = false;

  if (do_ind) {
  geom->triu = (int *)malloc ((*np) * sizeof (int));
  geom->trid = (int *)malloc ((*np) * sizeof (int));
  for (int i = 0; i < *np; i++) geom->triu[i] = -2;
  for (int i = 0; i < *np; i++) geom->trid[i] = -2;
  geom->ind = (unsigned int *)malloc (3 * (*nt) * sizeof (unsigned int));
  }else{
  geom->ind_strip = (unsigned int *)malloc (geom->ind_strip_size * sizeof (unsigned int));
  memset (geom->ind_strip, 0xff, geom->ind_strip_size * sizeof (unsigned int));
  }

  glgauss (geom->Nj, geom->pl, geom->ind_strip, nstripe, 
           ind_stripcnt, geom->ind_stripcnt_per_lat, geom->ind_stripoff_per_lat,
	   true);

  
//printf (" ind_strip_size = %d, ind_size = %d, %12.5f\n", geom->ind_strip_size, 3 * (*nt), (float)(3 * (*nt))/(float)(geom->ind_strip_size));

  geom->jglooff = (int *)malloc ((1 + geom->Nj) * sizeof (int));
  geom->jglooff[0] = 0;
  for (int jlat = 2; jlat <= geom->Nj+1; jlat++)
     geom->jglooff[jlat-1] = geom->jglooff[jlat-2] + geom->pl[jlat-2];

  *lonlat = (unsigned short *)malloc (2 * sizeof (unsigned short) * v_len);


#pragma omp parallel for
  for (int jlat = 1; jlat <= geom->Nj; jlat++)
    {
      float lat = M_PI * (0.5 - (float)jlat / (float)(geom->Nj + 1));
      for (int jlon = 1; jlon <= geom->pl[jlat-1]; jlon++)
        {
          float lon = 2. * M_PI * (float)(jlon-1) / (float)geom->pl[jlat-1];
          int jglo = geom->jglooff[jlat-1] + jlon - 1;
          (*lonlat)[2*jglo+0] = (unsigned short int)(std::numeric_limits<unsigned short int>::max() * (lon + 0.0f       ) / (2.0f * M_PI));
          (*lonlat)[2*jglo+1] = (unsigned short int)(std::numeric_limits<unsigned short int>::max() * (lat + M_PI / 2.0f) / (1.0f * M_PI));
        }
    }
  
  *F = (float *)malloc (sizeof (float) * v_len);
  double * v = (double *)malloc (sizeof (double) * v_len);
  codes_get_size (h, "values", &v_len);
  codes_get_double_array (h, "values", v, &v_len);
  double vmis;
  codes_get_double (h, "missingValue", &vmis);

  for (int i = 0; i < *np; i++)
    (*F)[i] = v[i] == vmis ? 0. : v[i];

  codes_handle_delete (h);


}


