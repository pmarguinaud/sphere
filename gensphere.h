#ifndef _GENSPHERE_H
#define _GENSPHERE_H

#include <string>

void gensphere1 (const int, int *, float **, unsigned int *, unsigned int **, unsigned char **, int **, int **);

class jlonlat_t
{
public:

  typedef enum
  {
    I__=0,
    I_E=1,
    INE=2,
    IN_=3,
    INW=4,
    I_W=5,
    ISW=6,
    IS_=7,
    ISE=8 
  } pos_t;


  static std::string strpos (pos_t pos)
  {
    switch (pos)
      {
        case I__: return "I__";
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

  static pos_t pos (int i)
  {
    return (pos_t)i;
  }

  static pos_t next (pos_t pos)
  {
    switch (pos)
      {
        case I__: return I__; 
        case I_E: return INE; 
        case INE: return IN_; 
        case IN_: return INW;
        case INW: return I_W; 
        case I_W: return ISW;
        case ISW: return IS_; 
        case IS_: return ISE; 
        case ISE: return I_E;
      }
  }

  static pos_t prev (pos_t pos)
  {
    switch (pos)
      {
        case I__: return I__; 
        case INE: return I_E; 
        case IN_: return INE; 
        case INW: return IN_;
        case I_W: return INW; 
        case ISW: return I_W;
        case IS_: return ISW; 
        case ISE: return IS_; 
        case I_E: return ISE;
      }
  }

  static pos_t opposite (pos_t pos)
  {
    switch (pos)
      {
        case I__: return I__; 
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

  
  jlonlat_t () : jlon (0), jlat (0) {}
  jlonlat_t (int _jlon, int _jlat) : jlon (_jlon), jlat (_jlat) {}
  bool ok () { return (jlon > 0) && (jlat > 0); }
  static int jglo (int jlon, int jlat, int * jglooff) { return jglooff[jlat-1] + (jlon-1); }
  int jglo (int * jglooff) { return jglooff[jlat-1] + (jlon-1); }
  int jlon = 0;
  int jlat = 0;
};

void find_neighbours1 (int, int, int *, int, jlonlat_t [9]);

#endif
