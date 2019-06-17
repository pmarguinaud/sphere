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
void glgauss (const long int Nj, const long int pl[], unsigned int * ind, const int nstripe, int indcnt[])
{
  int iglooff[Nj];
  int indcntoff[nstripe];
  
  iglooff[0] = 0;
  for (int jlat = 2; jlat <= Nj-1; jlat++)
    iglooff[jlat-1] = iglooff[jlat-2] + pl[jlat-2];

  indcntoff[0] = 0;
  for (int istripe = 1; istripe < nstripe; istripe++)
    indcntoff[istripe] = indcntoff[istripe-1] + indcnt[istripe-1];

#pragma omp parallel for 
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

}
#undef MODULO

void gensphere (geom_t * geom, int * np, float ** xyz, 
                unsigned int * nt, unsigned int ** ind, 
	        float ** F, const std::string & type)
{
  const int nstripe = 8;
  int indoff[nstripe];

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
  
  *ind = (unsigned int *)malloc (3 * (*nt) * sizeof (unsigned int));
  glgauss (geom->Nj, geom->pl, *ind, nstripe, indoff);


  int v_len = 0;
  for (int jlat = 1; jlat <= geom->Nj; jlat++)
    v_len += geom->pl[jlat-1];
  
  geom->jglooff = (int *)malloc (geom->Nj * sizeof (int));
  geom->jglooff[0] = 0;
  for (int jlat = 2; jlat <= geom->Nj; jlat++)
     geom->jglooff[jlat-1] = geom->jglooff[jlat-2] + geom->pl[jlat-2];

  *xyz = (float *)malloc (3 * sizeof (float) * v_len);
  *F = (float *)malloc (sizeof (float) * v_len);
  *np  = v_len;

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

          (*xyz)[3*jglo+0] = X;
          (*xyz)[3*jglo+1] = Y;
          (*xyz)[3*jglo+2] = Z;

          (*F)[jglo] = 0;
	  if (type == "gradx")
            (*F)[jglo] = (1 + coslon) * coslat / 2.0;
	  else if (type == "lon")
            (*F)[jglo] = lon / (2 * M_PI);
	  else if (type == "lat")
            (*F)[jglo] = (1 + lat / (M_PI / 2)) / 2.0;
	  else if (type == "coslatxcos2lon")
            (*F)[jglo] = (1 + coslat * cos (2 * lon)) / 2.0;
	  else if (type == "coslatxcos3lon")
            (*F)[jglo] = (1 + coslat * cos (3 * lon)) / 2.0;
	  else if (type == "lat2xcos2lon")
            (*F)[jglo] = (1 - lat / (M_PI / 2)) * (1 + lat / (M_PI / 2)) * (1 + cos (2 * lon)) / 2.0;
	  else if (type == "XxYxZ")
            (*F)[jglo] = ((1 + X*X*X*X*X*X) / 2.0 * (1 + Y*Y*Y*Y*Y*Y) / 2.0 * (1 + Z*Z*Z*Z*Z*Z) / 2.0);
	  else if (type == "saddle0")
            {
              float lon1 = lon > M_PI ? lon - 2 * M_PI : lon; 
              (*F)[jglo] = lon1 * cos (lon1 / 2) * lat * cos (lat);
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
  
neigh_t geom_t::getNeighbours (const jlonlat_t & jlonlat) const
{
  neigh_t neigh;
  int jlon = jlonlat.jlon;
  int jlat = jlonlat.jlat;
  int ir, iq, iloen, iloen1, iloen2;
  int inum, iden;
  int jlat1, jlon1, jlat2, jlon2;
  

  neigh.add (neigh_t::I_E, INORM (jlon+1, pl[jlat-1]), jlat);
  
  if (jlat == 1) 
    {
      neigh.add (neigh_t::INE, 0, 0);
      neigh.add (neigh_t::IN_, 0, 0);
      neigh.add (neigh_t::INW, 0, 0);
    }
  else
    {
      jlon1 = jlon;
      jlat1 = jlat     ; iloen1 = pl[jlat1-1];
      jlat2 = jlat - 1 ; iloen2 = pl[jlat2-1];
      inum = iloen1 - iloen2 + jlon1 * iloen2; iden = iloen1;
      iq   = inum / iden; ir = (iq - 1) * iloen1 - (jlon1 - 1) * iloen2;
      if (ir == 0) 
        {
          neigh.add (neigh_t::INE, INORM (iq+1, iloen2), jlat2);
          neigh.add (neigh_t::IN_, INORM (iq+0, iloen2), jlat2);
          neigh.add (neigh_t::INW, INORM (iq-1, iloen2), jlat2);
        }
      else
        {
          neigh.add (neigh_t::INE, INORM (iq+1, iloen2), jlat2);
          neigh.add (neigh_t::IN_,                    0,     0);
          neigh.add (neigh_t::INW, INORM (iq+0, iloen2), jlat2);
        }
    }

  neigh.add (neigh_t::I_W, INORM (jlon-1, pl[jlat-1]), jlat);
  
  if (jlat == Nj) 
    {
      neigh.add (neigh_t::ISW, 0, 0);
      neigh.add (neigh_t::IS_, 0, 0);
      neigh.add (neigh_t::ISE, 0, 0);
    }
  else
    {
      jlon1 = jlon;
      jlat1 = jlat     ; iloen1 = pl[jlat1-1];
      jlat2 = jlat + 1 ; iloen2 = pl[jlat2-1];
      inum = iloen1 - iloen2 + jlon1 * iloen2; iden = iloen1;
      iq   = inum / iden; ir = (iq - 1) * iloen1 - (jlon1 - 1) * iloen2;
      if (ir == 0) 
        {
          neigh.add (neigh_t::ISW, INORM (iq-1, iloen2), jlat2);
          neigh.add (neigh_t::IS_, INORM (iq+0, iloen2), jlat2);
          neigh.add (neigh_t::ISE, INORM (iq+1, iloen2), jlat2);
        }
      else
        {
          neigh.add (neigh_t::ISW, INORM (iq+0, iloen2), jlat2);
          neigh.add (neigh_t::IS_,                    0,     0);
          neigh.add (neigh_t::ISE, INORM (iq+1, iloen2), jlat2);
        }
    }
  
  return neigh;
}

std::vector<neigh_t> geom_t::getNeighbours () const 
{
  std::vector<neigh_t> list;

  int size = 0;
  for (int jlat = 1; jlat <= Nj; jlat++)
    size += pl[jlat-1]; 
  
  list.resize (size);

#pragma omp parallel for
  for (int jlat = 1; jlat <= Nj; jlat++)
    for (int jlon = 1; jlon <= pl[jlat-1]; jlon++)
      {
        int ir, iq, iloen, iloen1, iloen2;
        int inum, iden;
        int jlat1, jlon1, jlat2, jlon2;
  
        int jglo = jglooff[jlat-1] + (jlon-1);
        neigh_t & neigh = list[jglo];

        neigh.add (neigh_t::I_E, INORM (jlon+1, pl[jlat-1]), jlat);
        
        if (jlat == 1) 
          {
            neigh.add (neigh_t::INE, 0, 0);
            neigh.add (neigh_t::IN_, 0, 0);
            neigh.add (neigh_t::INW, 0, 0);
          }
        else
          {
            jlon1 = jlon;
            jlat1 = jlat     ; iloen1 = pl[jlat1-1];
            jlat2 = jlat - 1 ; iloen2 = pl[jlat2-1];
            inum = iloen1 - iloen2 + jlon1 * iloen2; iden = iloen1;
            iq   = inum / iden; ir = (iq - 1) * iloen1 - (jlon1 - 1) * iloen2;
            if (ir == 0) 
              {
                neigh.add (neigh_t::INE, INORM (iq+1, iloen2), jlat2);
                neigh.add (neigh_t::IN_, INORM (iq+0, iloen2), jlat2);
                neigh.add (neigh_t::INW, INORM (iq-1, iloen2), jlat2);
              }
            else
              {
                neigh.add (neigh_t::INE, INORM (iq+1, iloen2), jlat2);
                neigh.add (neigh_t::IN_,                    0,     0);
                neigh.add (neigh_t::INW, INORM (iq+0, iloen2), jlat2);
              }
          }

        neigh.add (neigh_t::I_W, INORM (jlon-1, pl[jlat-1]), jlat);
        
        if (jlat == Nj) 
          {
            neigh.add (neigh_t::ISW, 0, 0);
            neigh.add (neigh_t::IS_, 0, 0);
            neigh.add (neigh_t::ISE, 0, 0);
          }
        else
          {
            jlon1 = jlon;
            jlat1 = jlat     ; iloen1 = pl[jlat1-1];
            jlat2 = jlat + 1 ; iloen2 = pl[jlat2-1];
            inum = iloen1 - iloen2 + jlon1 * iloen2; iden = iloen1;
            iq   = inum / iden; ir = (iq - 1) * iloen1 - (jlon1 - 1) * iloen2;
            if (ir == 0) 
              {
                neigh.add (neigh_t::ISW, INORM (iq-1, iloen2), jlat2);
                neigh.add (neigh_t::IS_, INORM (iq+0, iloen2), jlat2);
                neigh.add (neigh_t::ISE, INORM (iq+1, iloen2), jlat2);
              }
            else
              {
                neigh.add (neigh_t::ISW, INORM (iq+0, iloen2), jlat2);
                neigh.add (neigh_t::IS_,                    0,     0);
                neigh.add (neigh_t::ISE, INORM (iq+1, iloen2), jlat2);
              }
          }
    }
  
  return list;
}
 
void neigh_t::prn (const geom_t & geom, const jlonlat_t & _jlonlat) const
{
  printf ("\n\n");
  printf (" %4d %4d %4d\n", 
  	  geom.jglo (jlonlat[neigh_t::INW]),
  	  geom.jglo (jlonlat[neigh_t::IN_]),
  	  geom.jglo (jlonlat[neigh_t::INE]));
  printf (" %4d %4d %4d\n", 
          geom.jglo (jlonlat[neigh_t::I_W]),
          geom.jglo (_jlonlat),
          geom.jglo (jlonlat[neigh_t::I_E]));
  printf (" %4d %4d %4d\n", 
          geom.jglo (jlonlat[neigh_t::ISW]),
          geom.jglo (jlonlat[neigh_t::IS_]),
          geom.jglo (jlonlat[neigh_t::ISE]));
  printf ("\n\n");
}
  


void gensphere_grib (geom_t * geom, int * np, float ** xyz, 
                     unsigned int * nt, unsigned int ** ind, float ** F,
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
  
  *ind = (unsigned int *)malloc (3 * (*nt) * sizeof (unsigned int));
  glgauss (geom->Nj, geom->pl, *ind, nstripe, indoff);


  v_len = 0;
  for (int jlat = 1; jlat <= geom->Nj; jlat++)
    v_len += geom->pl[jlat-1];
  
  geom->jglooff = (int *)malloc (geom->Nj * sizeof (int));
  geom->jglooff[0] = 0;
  for (int jlat = 2; jlat <= geom->Nj; jlat++)
     geom->jglooff[jlat-1] = geom->jglooff[jlat-2] + geom->pl[jlat-2];

  *xyz = (float *)malloc (3 * sizeof (float) * v_len);
  *np  = v_len;


//#pragma omp parallel for
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


