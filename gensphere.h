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


class neigh_t
{
public:
  static const int NMAX = 12;
  jlonlat_t jlonlat[NMAX];
  jlonlat_t jlonlatb;


  typedef enum
  {
    P=+1,
    N=-1
  } rot_t;

  static rot_t inv (rot_t rot)
  {
    switch (rot)
      { 
        case P: return N;
	case N: return P;
      }
  }

  typedef enum
  {
    I__E=0,
    IN_E=1,
    INNE=2,
    IN__=3,
    INNW=4,
    IN_W=5,
    I__W=6,
    IS_W=7,
    ISSW=8,
    IS__=9,
    ISSE=10,
    IS_E=11 
  } pos_t;

  void add (pos_t _pos, int _jlon, int _jlat)
  {
    jlonlat[_pos].jlon = _jlon;
    jlonlat[_pos].jlat = _jlat;
  }

  static std::string strrot (rot_t rot)
  {
    switch (rot)
      {
        case P: return "P";
        case N: return "N";
      }
  }

  static std::string strpos (pos_t pos)
  {
    switch (pos)
      {
        case I__E: return "I__E";
        case IN_E: return "IN_E";
        case IN__: return "IN__";
        case IN_W: return "IN_W";
        case I__W: return "I__W";
        case IS_W: return "IS_W";
        case IS__: return "IS__";
        case IS_E: return "IS_E";
      }
  }


  int next (int pos, rot_t rot = P)
  {
    if (rot == N)
      return prev (pos);
    while (1)
      {
       pos++;
       if (pos == neigh_t::NMAX)
         pos = 0;
       if (jlonlat[pos].ok ())
         return pos;
     }
  }

  int prev (int pos, rot_t rot = P)
  {
    if (rot == N)
      return next (pos);
    while (1)
      {
        pos--;
	if (pos < 0)
          pos += neigh_t::NMAX;
        if (jlonlat[pos].ok ())
          return pos;
      }
  }

  bool done (bool * seen)
  {
     for (int i = 0; i < NMAX; i++)
       if ((jlonlat[i].ok ()) && ! seen[i]) return false;
     return true;
  }

  void prn (const class geom_t &, const jlonlat_t &) const;
  
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
  neigh_t getNeighbours (const class jlonlat_t &) const;
  std::vector<neigh_t> getNeighbours () const;
  int jglo (const class jlonlat_t & jlonlat) const 
  { 
    return jlonlat.ok () ? jglooff[jlonlat.jlat-1] + (jlonlat.jlon-1) : - 1; 
  }
  int opposite (const neigh_t &, int) const;
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

void gensphere (geom_t *, int *, float **, 
                unsigned int *, float **, const std::string &);

void gensphere_grib 
               (geom_t *, int *, float **, 
                unsigned int *, float **, const std::string &);


#endif
