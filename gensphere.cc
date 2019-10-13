#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <eccodes.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>


#include "gensphere.h"

#define MODULO(x, y) ((x)%(y))
#define JDLON(JLON1, JLON2) (MODULO ((JLON1) - 1, (iloen1)) * (iloen2) - MODULO ((JLON2) - 1, (iloen2)) * (iloen1))
#define JNEXT(JLON, ILOEN) (1 + MODULO ((JLON), (ILOEN)))


#define PRINT(a,b,c) \
  do {                                                            \
      *(inds++) = (a)-1; *(inds++) = (b)-1; *(inds++) = (c)-1;    \
  } while (0)

static 
void glgauss (const long int Nj, const long int pl[], unsigned int * ind, 
              const int nstripe, int indcnt[], int * triu = NULL, 
	      int * trid = NULL)
{
  int iglooff[Nj];
  int indcntoff[nstripe];
  
  iglooff[0] = 0;
  for (int jlat = 2; jlat <= Nj; jlat++)
    iglooff[jlat-1] = iglooff[jlat-2] + pl[jlat-2];

  indcntoff[0] = 0;
  for (int istripe = 1; istripe < nstripe; istripe++)
    indcntoff[istripe] = indcntoff[istripe-1] + indcnt[istripe-1];

  if (triu)
    for (int jlon = 1; jlon <= pl[0]; jlon++)
      triu[iglooff[0]+jlon-1] = -1;

  if (trid)
    for (int jlon = 1; jlon <= pl[Nj-1]; jlon++)
      trid[iglooff[Nj-1]+jlon-1] = -1;

//#pragma omp parallel for 
  for (int istripe = 0; istripe < nstripe; istripe++)
    {
      int jlat1 = 1 + ((istripe + 0) * (Nj-1)) / nstripe;
      int jlat2 = 0 + ((istripe + 1) * (Nj-1)) / nstripe;
      unsigned int * inds = ind + 3 * indcntoff[istripe];

      for (int jlat = jlat1; jlat <= jlat2; jlat++)
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
		  if (triu) triu[icb-1] = (inds - ind) / 3;
                  PRINT (ica, icb, icc);
		  if (trid) trid[ica-1] = (inds - ind) / 3;
                  PRINT (ica, icc, icd);
                }
            }
          else 
            {
              int jlon1 = 1;
              int jlon2 = 1;
              bool turn = false;
              for (;;)
                {
                  int ica = 0, icb = 0, icc = 0;

                  int jlon1n = JNEXT (jlon1, iloen1);
                  int jlon2n = JNEXT (jlon2, iloen2);

                  

#define AV1 \
  do {                                                                        \
    ica = jglooff1 + jlon1; icb = jglooff2 + jlon2; icc = jglooff1 + jlon1n;  \
    if (trid) trid[ica-1] = (inds - ind) / 3;                                 \
    jlon1 = jlon1n;                                                           \
    turn = turn || jlon1 == 1;                                                \
  } while (0)

#define AV2 \
  do {                                                                        \
    ica = jglooff1 + jlon1; icb = jglooff2 + jlon2; icc = jglooff2 + jlon2n;  \
    if (triu) triu[icb-1] = (inds - ind) / 3;                                 \
    jlon2 = jlon2n;                                                           \
    turn = turn || jlon2 == 1;                                                \
  } while (0)

                  int idlonc = JDLON (jlon1, jlon2);
                  int idlonn;
		  if ((jlon1n == 1) && (jlon2n != 1))
                    idlonn = +1;
		  else if ((jlon1n != 1) && (jlon2n == 1))
                    idlonn = -1;
		  else 
                    idlonn = JDLON (jlon1n, jlon2n);

                  if (idlonn > 0 || ((idlonn == 0) && (idlonc > 0)))
                    AV2;
                  else if (idlonn < 0 || ((idlonn == 0) && (idlonc < 0))) 
                    AV1;
                  else
                    abort ();
             
                  PRINT (ica, icb, icc);
                 
                  if (turn)
                    {
                      if (jlon1 == 1)
                        while (jlon2 != 1)
                          {
                            int jlon2n = JNEXT (jlon2, iloen2);
                            AV2;
                            PRINT (ica, icb, icc);
                          }
                      else if (jlon2 == 1)
                        while (jlon1 != 1)
                          {
                            int jlon1n = JNEXT (jlon1, iloen1);
                            AV1;
                           PRINT (ica, icb, icc);
                          }
                      break;
                    }

                }
         
         
            }

        }

    }

}
#undef MODULO

void gensphere (geom_t * geom, int * np, float ** xyz, 
                unsigned int * nt, float ** Fx, float ** Fy, const std::string & type)
{
  const int nstripe = 8;
  int indoff[nstripe];
  bool init = geom->pl == NULL;

  if (init)
    {
      *xyz = NULL;
     
      geom->pl = (long int *)malloc (sizeof (long int) * geom->Nj);
     
      for (int jlat = 1; jlat <= geom->Nj; jlat++)
        {
          float lat = M_PI * (0.5 - (float)jlat / (float)(geom->Nj + 1));
          float coslat = cos (lat);
          geom->pl[jlat-1] = (2. * geom->Nj * coslat);
        }
     
      *nt = 0;
      for (int jlat = 1; jlat < geom->Nj; jlat++)
        *nt += geom->pl[jlat-1] + geom->pl[jlat];
     
      for (int istripe = 0; istripe < nstripe; istripe++)
        {
          int jlat1 = 1 + ((istripe + 0) * (geom->Nj-1)) / nstripe;
          int jlat2 = 0 + ((istripe + 1) * (geom->Nj-1)) / nstripe;
          indoff[istripe] = 0;
          for (int jlat = jlat1; jlat <= jlat2; jlat++)
            indoff[istripe] += geom->pl[jlat-1] + geom->pl[jlat];
        }
      
      int v_len = 0;
      for (int jlat = 1; jlat <= geom->Nj; jlat++)
        v_len += geom->pl[jlat-1];
      *np  = v_len;
      
      geom->triu = (int *)malloc ((*np) * sizeof (int));
      geom->trid = (int *)malloc ((*np) * sizeof (int));
      for (int i = 0; i < *np; i++) geom->triu[i] = -2;
      for (int i = 0; i < *np; i++) geom->trid[i] = -2;
      geom->ind = (unsigned int *)malloc (3 * (*nt) * sizeof (unsigned int));
      glgauss (geom->Nj, geom->pl, geom->ind, nstripe, indoff, geom->triu, geom->trid);
     
     
      geom->jglooff = (int *)malloc ((1 + geom->Nj) * sizeof (int));
      geom->jglooff[0] = 0;
      for (int jlat = 2; jlat <= geom->Nj + 1; jlat++)
         geom->jglooff[jlat-1] = geom->jglooff[jlat-2] + geom->pl[jlat-2];
     
      *xyz = (float *)malloc (3 * sizeof (float) * v_len);
      *Fx = (float *)malloc (sizeof (float) * v_len);
      *Fy = (float *)malloc (sizeof (float) * v_len);
    }

#pragma omp parallel for
  for (int jlat = 1; jlat <= geom->Nj; jlat++)
    {
      float lat = M_PI * (0.5 - (float)jlat / (float)(geom->Nj + 1));
      float coslat = cos (lat); float sinlat = sin (lat);
      for (int jlon = 1; jlon <= geom->pl[jlat-1]; jlon++)
        {
          float lon = 2. * M_PI * (float)(jlon-1) / (float)geom->pl[jlat-1];
          float coslon = cos (lon); float sinlon = sin (lon);
          float radius = 1.0;
          int jglo = geom->jglooff[jlat-1] + jlon - 1;
          float X = coslon * coslat * radius;
          float Y = sinlon * coslat * radius;
          float Z =          sinlat * radius;

	  if (init)
            {
              (*xyz)[3*jglo+0] = X;
              (*xyz)[3*jglo+1] = Y;
              (*xyz)[3*jglo+2] = Z;
	    }

          (*Fx)[jglo] = 0;
          (*Fy)[jglo] = 0;
	  if (type == "constx")
            {
              (*Fx)[jglo] = 1.;
            }
	  else if (type == "consty")
            {
              (*Fy)[jglo] = 1.;
            }
	  else if (type == "constxy")
            {
              (*Fx)[jglo] = 1.;
              (*Fy)[jglo] = 1.;
            }
	  else if (type == "gradx")
            {
              (*Fx)[jglo] = (1 + coslon) * coslat / 2.0;
            }
	  else if (type == "lon")
            {
              (*Fx)[jglo] = lon / (2 * M_PI);
            }
	  else if (type == "lat")
            {
              (*Fx)[jglo] = (1 + lat / (M_PI / 2)) / 2.0;
            }
	  else if (type == "coslatxcos2lon")
            {
              (*Fx)[jglo] = (1 + coslat * cos (2 * lon)) / 2.0;
            }
	  else if (type == "coslatxcos3lon")
            {
              (*Fx)[jglo] = (1 + coslat * cos (3 * lon)) / 2.0;
            }
	  else if (type == "lat2xcos2lon")
            {
              (*Fx)[jglo] = (1 - lat / (M_PI / 2)) * (1 + lat / (M_PI / 2)) * (1 + cos (2 * lon)) / 2.0;
            }
	  else if (type == "XxYxZ")
            {
              (*Fx)[jglo] = ((1 + X*X*X*X*X*X) / 2.0 * (1 + Y*Y*Y*Y*Y*Y) / 2.0 * (1 + Z*Z*Z*Z*Z*Z) / 2.0);
            }
	  else if (type == "saddle0")
            {
              float lon1 = lon > M_PI ? lon - 2 * M_PI : lon; 
              (*Fx)[jglo] = lon1 * cos (lon1 / 2) * lat * cos (lat);
	    }
	  else
            abort ();

        }
    }
  
}

static inline int MODULO (int A, int B) 
{
  return (((A) % (B) < 0) ? (((A) % (B)) + (B)) : ((A) % (B)));
}

static inline int INORM (int KLO, int KLOEN) 
{
  return (1 + MODULO (KLO-1, KLOEN));
}

static inline void IQR (int & iq, int & ir, 
                        int iloen1, int iloen2, int jlon1)
{
  int inum = iloen1 - iloen2 + jlon1 * iloen2, iden = iloen1;
  iq   = inum / iden; ir = (iq - 1) * iloen1 - (jlon1 - 1) * iloen2;
}

static inline bool GT (int p1, int q1, int p2, int q2, int p3, int q3) 
{
  p1--; p2--; p3--;
  if (p2 * q1 < p1 * q2)
    p2 += q2;
  if (p3 * q1 < p1 * q3)
    p3 += q3;
  return p2 * q3 - p3 * q2 > 0;
}

static inline bool LT (int p1, int q1, int p2, int q2, int p3, int q3) 
{
  p1--; p2--; p3--;
  if (p2 * q1 < p1 * q2)
    p2 += q2;
  if (p3 * q1 < p1 * q3)
    p3 += q3;
  return p2 * q3 - p3 * q2 < 0;
}


void gensphere_grib (geom_t * geom, int * np, float ** xyz, 
                     unsigned int * nt, float ** F,
                     const std::string & file)
{
  const int nstripe = 8;
  int indoff[nstripe];

  *xyz = NULL;

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

  for (int istripe = 0; istripe < nstripe; istripe++)
    {
      int jlat1 = 1 + ((istripe + 0) * (geom->Nj-1)) / nstripe;
      int jlat2 = 0 + ((istripe + 1) * (geom->Nj-1)) / nstripe;
      indoff[istripe] = 0;
      for (int jlat = jlat1; jlat <= jlat2; jlat++)
        indoff[istripe] += geom->pl[jlat-1] + geom->pl[jlat];
    }
  
  v_len = 0;
  for (int jlat = 1; jlat <= geom->Nj; jlat++)
    v_len += geom->pl[jlat-1];
  *np  = v_len;
  
  geom->triu = (int *)malloc ((*np) * sizeof (int));
  geom->trid = (int *)malloc ((*np) * sizeof (int));
  for (int i = 0; i < *np; i++) geom->triu[i] = -2;
  for (int i = 0; i < *np; i++) geom->trid[i] = -2;
  geom->ind = (unsigned int *)malloc (3 * (*nt) * sizeof (unsigned int));
  glgauss (geom->Nj, geom->pl, geom->ind, nstripe, indoff, geom->triu, geom->trid);


  geom->jglooff = (int *)malloc ((1 + geom->Nj) * sizeof (int));
  geom->jglooff[0] = 0;
  for (int jlat = 2; jlat <= geom->Nj+1; jlat++)
     geom->jglooff[jlat-1] = geom->jglooff[jlat-2] + geom->pl[jlat-2];

  *xyz = (float *)malloc (3 * sizeof (float) * v_len);


#pragma omp parallel for
  for (int jlat = 1; jlat <= geom->Nj; jlat++)
    {
      float lat = M_PI * (0.5 - (float)jlat / (float)(geom->Nj + 1));
      float coslat = cos (lat); float sinlat = sin (lat);
      for (int jlon = 1; jlon <= geom->pl[jlat-1]; jlon++)
        {
          float lon = 2. * M_PI * (float)(jlon-1) / (float)geom->pl[jlat-1];
          float coslon = cos (lon); float sinlon = sin (lon);
          float radius = 1.0;
          int jglo = geom->jglooff[jlat-1] + jlon - 1;
          float X = coslon * coslat * radius;
          float Y = sinlon * coslat * radius;
          float Z =          sinlat * radius;

          (*xyz)[3*jglo+2] = Z;
          (*xyz)[3*jglo+1] = Y;
          (*xyz)[3*jglo+0] = X;


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


