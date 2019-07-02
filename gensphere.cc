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
          bool dbg = false; //jlat == 4;
     
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
              bool turn = false;
              for (;;)
                {
                  int ica = 0, icb = 0, icc = 0;

                  int jlon1n = JNEXT (jlon1, iloen1);
                  int jlon2n = JNEXT (jlon2, iloen2);

                  

#define AV1 \
  do {                                                                        \
    ica = jglooff1 + jlon1; icb = jglooff2 + jlon2; icc = jglooff1 + jlon1n;  \
    jlon1 = jlon1n;                                                           \
    turn = turn || jlon1 == 1;                                                \
    if (dbg) printf (" AV1 ");                                                \
  } while (0)

#define AV2 \
  do {                                                                        \
    ica = jglooff1 + jlon1; icb = jglooff2 + jlon2; icc = jglooff2 + jlon2n;  \
    jlon2 = jlon2n;                                                           \
    turn = turn || jlon2 == 1;                                                \
    if (dbg) printf (" AV2 ");                                                \
  } while (0)

                  int idlonc = JDLON (jlon1, jlon2);
                  int idlonn;
		  if ((jlon1n == 1) && (jlon2n != 1))
                    idlonn = +1;
		  else if ((jlon1n != 1) && (jlon2n == 1))
                    idlonn = -1;
		  else 
                    idlonn = JDLON (jlon1n, jlon2n);

if (dbg) printf (" jlon1 = %4d, jlon2 = %4d, idlonc = %4d, idlonn = %4d", jlon1, jlon2, idlonc, idlonn);
if (dbg) printf (" jlon1n = %4d, jlon2n = %4d", jlon1n, jlon2n);

                  if (idlonn > 0 || ((idlonn == 0) && (idlonc > 0)))
                    AV2;
                  else if (idlonn < 0 || ((idlonn == 0) && (idlonc < 0))) 
                    AV1;
                  else
                    abort ();
             
if (dbg) printf (" jlon1 = %4d, jlon2 = %4d", jlon1, jlon2);
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
if (dbg) printf ("\n");

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


neigh_t geom_t::getNeighbours (const jlonlat_t & jlonlat) const
{
  neigh_t neigh;
  int jlon = jlonlat.jlon;
  int jlat = jlonlat.jlat;
  int ir, iq, iloen, iloen1, iloen2;
  int inum, iden;
  int jlat1, jlon1, jlat2;
  int jlon__E = INORM (jlon+1, pl[jlat-1]);
  int jlon__W = INORM (jlon-1, pl[jlat-1]);
  int jlon_NE;
  int jlon_NW;
  int jlon_SE;
  int jlon_SW;

  neigh.jlonlatb = jlonlat;
  
  for (int i = 0; i < neigh_t::NMAX; i++)
    neigh.jlonlat[i] = jlonlat_t (0, 0);

  int n = 0;
  neigh.jlonlat[n++] = jlonlat_t (jlon__E, jlat);

  if (jlat > 1)
    {
      jlat1 = jlat     ; iloen1 = pl[jlat1-1];
      jlat2 = jlat - 1 ; iloen2 = pl[jlat2-1];

      IQR (iq, ir, iloen1, iloen2, jlon);
      jlon_NE = INORM (iq+1, iloen2);                                    // NE of current point
      jlon_NW = ir == 0 ? INORM (iq-1, iloen2) : INORM (iq+0, iloen2);   // NW of current point
    
      if (GT (jlon, iloen1, jlon__E, iloen1, jlon_NE, iloen2))           // If E > NE, then take NW of E instead of NE
        {
          IQR (iq, ir, iloen1, iloen2, jlon__E);
          jlon_NE = INORM (iq+0, iloen2);   
	}
      if (LT (jlon, iloen1, jlon__W, iloen1, jlon_NW, iloen2))           // If W < NW, then take NE of W instead of NW
        {
          IQR (iq, ir, iloen1, iloen2, jlon__W);
          jlon_NW = ir == 0 ? INORM (iq+0, iloen2) : INORM (iq+1, iloen2);
	}

      for (int jlon = jlon_NE; ;)
        {
          neigh.jlonlat[n++] = jlonlat_t (jlon, jlat2);
          if (jlon == jlon_NW)
            break;
          jlon = INORM (jlon-1, iloen2);
        }
    }

  neigh.jlonlat[n++] = jlonlat_t (jlon__W, jlat);

  if (jlat < Nj)
    {
      jlat1 = jlat     ; iloen1 = pl[jlat1-1];
      jlat2 = jlat + 1 ; iloen2 = pl[jlat2-1];
     
      IQR (iq, ir, iloen1, iloen2, jlon);
      jlon_SE = INORM (iq+1, iloen2);                                    // SE of current point
      jlon_SW = ir == 0 ? INORM (iq-1, iloen2) : INORM (iq+0, iloen2);   // SW of current point

      if (GT (jlon, iloen1, jlon__E, iloen1, jlon_SE, iloen2))
        {
          IQR (iq, ir, iloen1, iloen2, jlon__E);
          jlon_SE = INORM (iq+0, iloen2);   
	}
      if (LT (jlon, iloen1, jlon__W, iloen1, jlon_SW, iloen2))
        {
          IQR (iq, ir, iloen1, iloen2, jlon__W);
          jlon_SW = ir == 0 ? INORM (iq+0, iloen2) : INORM (iq+1, iloen2);
	}

      for (int jlon = jlon_SW; ;)
        {
          neigh.jlonlat[n++] = jlonlat_t (jlon, jlat2);
          if (jlon == jlon_SE)
            break;
          jlon = INORM (jlon+1, iloen2);
        }
    }

  if (n > neigh_t::NMAX)
    abort ();

#ifdef UNDEF
  printf (" (%4d, %4d) :", jlat, jlon);
  for (int i = 0; i < neigh_t::NMAX; i++)
    {
      if (! neigh.jlonlat[i].ok ())
        break;
      printf (" (%4d, %4d)", neigh.jlonlat[i].jlat, neigh.jlonlat[i].jlon);
    }
  printf ("\n");
#endif

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
      list[jglooff[jlat-1]+jlon-1] = getNeighbours (jlonlat_t (jlon, jlat));
  
  return list;
}
 
void neigh_t::prn (const geom_t & geom, const jlonlat_t & _jlonlat) const
{
  printf ("\n\n");
  printf (" %4d %4d %4d %4d %4d\n", 
  	  geom.jglo (jlonlat[neigh_t::IN_W]),
  	  geom.jglo (jlonlat[neigh_t::INNW]),
  	  geom.jglo (jlonlat[neigh_t::IN__]),
  	  geom.jglo (jlonlat[neigh_t::INNE]),
  	  geom.jglo (jlonlat[neigh_t::IN_E]));
  printf (" %4d     %4d     %4d\n", 
          geom.jglo (jlonlat[neigh_t::I__W]),
          geom.jglo (_jlonlat),
          geom.jglo (jlonlat[neigh_t::I__E]));
  printf (" %4d %4d %4d %4d %4d\n", 
          geom.jglo (jlonlat[neigh_t::IS_W]),
          geom.jglo (jlonlat[neigh_t::ISSW]),
          geom.jglo (jlonlat[neigh_t::IS__]),
          geom.jglo (jlonlat[neigh_t::ISSE]),
          geom.jglo (jlonlat[neigh_t::IS_E]));
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

int geom_t::opposite (const neigh_t & neigh0, int pos0) const
{
  const jlonlat_t & jlonlat1 = neigh0.jlonlat[pos0];
  if (! jlonlat1.ok ())
    abort ();
  neigh_t neigh1 = getNeighbours (jlonlat1);
  for (int pos1 = 0; pos1 < neigh_t::NMAX; pos1++)
    if (neigh1.jlonlat[pos1] == neigh0.jlonlatb)
      return pos1;
  abort ();
}

int triangleUp (const geom_t & geom, const jlonlat_t & jlonlat)
{
  bool dbg = true;

  if (jlonlat.jlat == 1)
    return 0;

  int jlat1 = jlonlat.jlat+0; int iloen1 = geom.pl[jlat1-1];
  int jlat2 = jlonlat.jlat-1; int iloen2 = geom.pl[jlat2-1];

  int jlonn = INORM (jlonlat.jlon + 1, iloen1);
  int iq, ir;
  int jlon2;

  if (iloen1 == iloen2)
    {
      if (dbg) printf (" iloen1 = iloen2 ");
      jlon2 = jlonlat.jlon;
    }
  else if (iloen1 < iloen2)
    {
      if (dbg) printf (" iloen1 < iloen2 ");
      IQR (iq, ir, iloen1, iloen2, jlonn);
      jlon2 = ir == 0 ? INORM (iq-1, iloen2) : INORM (iq+0, iloen2); // NW
    }
  else if (iloen1 > iloen2)
    {
      if (dbg) printf (" iloen1 > iloen2 ");
      IQR (iq, ir, iloen1, iloen2, jlonlat.jlon);
      jlon2 = ir == 0 ? INORM (iq+0, iloen2) : INORM (iq+1, iloen2); // N or NE
    }
  if (dbg) printf (" jlon = %4d, %12.4f, jlonn = %4d, %12.4f jlon2 = %4d, %12.4f ir = %4d ", 
          jlonlat.jlon, 
          2 * (jlonlat.jlon-1) * 180.0f / iloen1, 
	  jlonn, 
          2 * (jlonn-1) * 180.0f / iloen1, 
	  jlon2, 
	  2 * (jlon2-1) * 180.0f / iloen2,
	  ir
	  );
  if (jlonn == 1)
    {
      jlonn += iloen1;
      if (jlon2 == 1)
        jlon2 += iloen2;
    }
//printf (" jlat1 = %4d, %4d %4d %4d ", jlat1, geom.jglooff[jlat1-1], geom.pl[jlat1-2], geom.pl[0]);
  return geom.jglooff[jlat1-1] * 2 - geom.pl[jlat1-2] - geom.pl[0]
       + jlonn + jlon2 - 2;
}

