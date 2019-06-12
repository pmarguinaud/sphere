#ifndef _GENSPHERE_H
#define _GENSPHERE_H

#include <string>

void gensphere1 (const int, int *, float **, unsigned int *, unsigned int **, unsigned char **, int **, int **);

class jlonlat_t
{
public:

  bool operator== (const jlonlat_t & other) const
  {
    return (jlon == other.jlon) && (jlat == other.jlat);
  }
  
  jlonlat_t () : jlon (0), jlat (0) {}
  jlonlat_t (int _jlon, int _jlat) : jlon (_jlon), jlat (_jlat) {}
  bool ok () { return (jlon > 0) && (jlat > 0); }
  static int jglo (int jlon, int jlat, int * jglooff) { return jglooff[jlat-1] + (jlon-1); }
  int jglo (int * jglooff) { return ok () ? jglooff[jlat-1] + (jlon-1) : - 1; }
  int jlon = 0;
  int jlat = 0;
};



class neigh_t
{
public:

  jlonlat_t jlonlat[8];

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
    I_E=0,
    INE=1,
    IN_=2,
    INW=3,
    I_W=4,
    ISW=5,
    IS_=6,
    ISE=7 
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
        case I_E: return "I_E";
        case INE: return "INE";
        case IN_: return "IN_";
        case INW: return "INW";
        case I_W: return "I_W";
        case ISW: return "ISW";
        case IS_: return "IS_";
        case ISE: return "ISE";
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
            case I_E: pos = INE; break; 
            case INE: pos = IN_; break; 
            case IN_: pos = INW; break;
            case INW: pos = I_W; break; 
            case I_W: pos = ISW; break;
            case ISW: pos = IS_; break; 
            case IS_: pos = ISE; break; 
            case ISE: pos = I_E; break;
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
            case INE: pos = I_E; break; 
            case IN_: pos = INE; break; 
            case INW: pos = IN_; break;
            case I_W: pos = INW; break; 
            case ISW: pos = I_W; break;
            case IS_: pos = ISW; break; 
            case ISE: pos = IS_; break; 
            case I_E: pos = ISE; break;
          }
        if (jlonlat[pos].ok ())
          return pos;
      }
  }

  static pos_t opposite (pos_t pos)
  {
    switch (pos)
      {
        case INE: return ISW; 
        case IN_: return IS_; 
        case INW: return ISE;
        case I_W: return I_E; 
        case ISW: return INE;
        case IS_: return IN_; 
        case ISE: return INW; 
        case I_E: return I_W;
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

};



void find_neighbours1 (const jlonlat_t &, int *, int, neigh_t *);

#endif
