#ifndef _GENSPHERE_H
#define _GENSPHERE_H

void gensphere1 (const int, int *, float **, unsigned int *, unsigned int **, unsigned char **, int **, int **);

class jlonlat_t
{
public:

  typedef enum
  {
    I__=0,
    I_E=1,
    I_W=2,
    INE=3,
    INW=4,
    ISE=5,
    ISW=6,
    IS_=7,
    IN_=8
  } jlonlat_pos_t;

  jlonlat_pos_t opposite (jlonlat_pos_t pos)
  {
    switch (pos)
      {
        case I__: return I__; case I_E: return I_W; case I_W: return I_E;
        case INE: return ISW; case INW: return ISE; case ISE: return INW;
        case ISW: return INE; case IS_: return IN_; case IN_: return IS_;
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
