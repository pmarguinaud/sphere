#ifndef _GENSPHERE_H
#define _GENSPHERE_H

#include <string>
#include <vector>

class jlonlat_t
{
public:

  bool operator== (const jlonlat_t & other) const
  {
    return (jlon == other.jlon) && (jlat == other.jlat);
  }
  
  jlonlat_t () : jlon (0), jlat (0) {}
  jlonlat_t (int _jlon, int _jlat) : jlon (_jlon), jlat (_jlat) {}
  bool ok () const { return (jlon > 0) && (jlat > 0); }
  int jlon = 0;
  int jlat = 0;


};


class geom_t
{
public:
  long int Nj = 0;
  long int * pl = NULL;
  int * jglooff = NULL;
  unsigned int * ind = NULL;
  int * triu = NULL;
  int * trid = NULL;
  int jglo (const class jlonlat_t & jlonlat) const 
  { 
    return jlonlat.ok () ? jglooff[jlonlat.jlat-1] + (jlonlat.jlon-1) : - 1; 
  }
  int size () const { return jglooff[Nj-1] + pl[Nj-1]; }
  jlonlat_t jlonlat (int jglo) const 
  {
    int jlat1 = 0, jlat2 = Nj, jlat;
    while (jlat2 != jlat1 + 1)
      {
        int jlatm = (jlat1 + jlat2) / 2;
	if ((jglooff[jlat1] <= jglo) && (jglo < jglooff[jlatm]))
          jlat2 = jlatm;
        else if ((jglooff[jlatm] <= jglo) && (jglo < jglooff[jlat2]))
          jlat1 = jlatm;
      }
    jlat = 1 + jlat1;
    int jlon = 1 + jglo - jglooff[jlat-1];
    return jlonlat_t (jlon, jlat);
  }
};

void gensphere (geom_t *, int *, unsigned short **, 
                unsigned int *, float **, const std::string &);

void gensphere_grib 
               (geom_t *, int *, unsigned short **, 
                unsigned int *, float **, const std::string &);


#endif
