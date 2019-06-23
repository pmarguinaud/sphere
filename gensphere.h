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


  pos_t next (pos_t pos, rot_t rot = P)
  {
    if (rot == N)
      return prev (pos);
    while (1)
      {
        switch (pos)
          {
            case I__E: pos = IN_E; break; 
            case IN_E: pos = IN__; break; 
            case IN__: pos = IN_W; break;
            case IN_W: pos = I__W; break; 
            case I__W: pos = IS_W; break;
            case IS_W: pos = IS__; break; 
            case IS__: pos = IS_E; break; 
            case IS_E: pos = I__E; break;
          }
       if (jlonlat[pos].ok ())
         return pos;
     }
  }

  pos_t prev (pos_t pos, rot_t rot = P)
  {
    if (rot == N)
      return next (pos);
    while (1)
      {
        switch (pos)
          {
            case IN_E: pos = I__E; break; 
            case IN__: pos = IN_E; break; 
            case IN_W: pos = IN__; break;
            case I__W: pos = IN_W; break; 
            case IS_W: pos = I__W; break;
            case IS__: pos = IS_W; break; 
            case IS_E: pos = IS__; break; 
            case I__E: pos = IS_E; break;
          }
        if (jlonlat[pos].ok ())
          return pos;
      }
  }

  static pos_t opposite (pos_t pos)
  {
    switch (pos)
      {
        case IN_E: return IS_W; 
        case IN__: return IS__; 
        case IN_W: return IS_E;
        case I__W: return I__E; 
        case IS_W: return IN_E;
        case IS__: return IN__; 
        case IS_E: return IN_W; 
        case I__E: return I__W;
      }
  }

  bool done (bool * seen)
  {
     if ((jlonlat[0].ok ()) && ! seen[0]) return false;
     if ((jlonlat[1].ok ()) && ! seen[1]) return false;
     if ((jlonlat[2].ok ()) && ! seen[2]) return false;
     if ((jlonlat[3].ok ()) && ! seen[3]) return false;
     if ((jlonlat[4].ok ()) && ! seen[4]) return false;
     if ((jlonlat[5].ok ()) && ! seen[5]) return false;
     if ((jlonlat[6].ok ()) && ! seen[6]) return false;
     if ((jlonlat[7].ok ()) && ! seen[7]) return false;
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
  neigh_t getNeighbours (const class jlonlat_t &) const;
  std::vector<neigh_t> getNeighbours () const;
  int jglo (const class jlonlat_t & jlonlat) const 
  { 
    return jlonlat.ok () ? jglooff[jlonlat.jlat-1] + (jlonlat.jlon-1) : - 1; 
  }
};

void gensphere (geom_t *, int *, float **, 
                unsigned int *, unsigned int **, 
	        float **, const std::string &);

void gensphere_grib 
               (geom_t *, int *, float **, 
                unsigned int *, unsigned int **, 
                float **, const std::string &);


#endif
