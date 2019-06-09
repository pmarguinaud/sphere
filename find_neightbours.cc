

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
  
  jlonlat_t (int _jlon, int _jlat) : jlon (_jlon), jlat (_jlat) {}
  int jlon;
  int jlat;
};

void find_neighbours1 (int jlat, int jlon, int * pl, int Nj, jlonlat_t jlonlat[9])
{
  int ir, iq, iloen, iloen1, iloen2;
  int inum, iden;
  int jlat1, jlon1, jlat2, jlon2;
  
#define MODULO(A, B) (((A) % (B) < 0) ? (((A) % (B)) + (B)) : ((A) % (B)))
#define INORM(KLO, KLOEN) (1 + MODULO (KLO-1, KLOEN))
  
  jlonlat[jlonlat_t::I__] = {INORM (jlon+0, pl[jlat-1]), jlat};
  jlonlat[jlonlat_t::I_W] = {INORM (jlon-1, pl[jlat-1]), jlat};
  jlonlat[jlonlat_t::I_E] = {INORM (jlon+1, pl[jlat-1]), jlat};
  
  if (jlat == 1) 
    {
      iloen = pl[jlat-1];
      iq = jlon + iloen / 2; ir = MODULO (iloen, 2);
      if (ir == 0) 
        {
          jlonlat[jlonlat_t::INW] = jlonlat_t (INORM (iq+1, iloen), jlat);
          jlonlat[jlonlat_t::IN_] = jlonlat_t (INORM (iq+0, iloen), jlat);
          jlonlat[jlonlat_t::INE] = jlonlat_t (INORM (iq-1, iloen), jlat);
        }
      else
        {
          jlonlat[jlonlat_t::INW] = jlonlat_t (INORM (iq+1, iloen), jlat);
          jlonlat[jlonlat_t::IN_] = jlonlat_t (                  0,    0);
          jlonlat[jlonlat_t::INE] = jlonlat_t (INORM (iq+0, iloen), jlat);
        }
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
          jlonlat[jlonlat_t::INW] = jlonlat_t (INORM (iq-1, iloen2), jlat2);
          jlonlat[jlonlat_t::IN_] = jlonlat_t (INORM (iq+0, iloen2), jlat2);
          jlonlat[jlonlat_t::INE] = jlonlat_t (INORM (iq+1, iloen2), jlat2);
        }
      else
        {
          jlonlat[jlonlat_t::INW] = jlonlat_t (INORM (iq+0, iloen2), jlat2);
          jlonlat[jlonlat_t::IN_] = jlonlat_t (                   0,     0);
          jlonlat[jlonlat_t::INE] = jlonlat_t (INORM (iq+1, iloen2), jlat2);
        }
    }
  
  if (jlat == Nj) 
    {
      iloen = pl[jlat-1];
      iq = jlon + iloen / 2; ir = MODULO (iloen, 2);
      if (ir == 0) 
        {
          jlonlat[jlonlat_t::ISW] = jlonlat_t (INORM (iq+1, iloen), jlat);
          jlonlat[jlonlat_t::IS_] = jlonlat_t (INORM (iq+0, iloen), jlat);
          jlonlat[jlonlat_t::ISE] = jlonlat_t (INORM (iq-1, iloen), jlat);
        }
      else
        {
          jlonlat[jlonlat_t::ISW] = jlonlat_t (INORM (iq+1, iloen), jlat);
          jlonlat[jlonlat_t::IS_] = jlonlat_t (                  0,    0);
          jlonlat[jlonlat_t::ISE] = jlonlat_t (INORM (iq+0, iloen), jlat);
        }
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
          jlonlat[jlonlat_t::ISW] = jlonlat_t (INORM (iq-1, iloen2), jlat2);
          jlonlat[jlonlat_t::IS_] = jlonlat_t (INORM (iq+0, iloen2), jlat2);
          jlonlat[jlonlat_t::ISE] = jlonlat_t (INORM (iq+1, iloen2), jlat2);
        }
      else
        {
          jlonlat[jlonlat_t::ISW] = jlonlat_t (INORM (iq+0, iloen2), jlat2);
          jlonlat[jlonlat_t::IS_] = jlonlat_t (                   0,     0);
          jlonlat[jlonlat_t::ISE] = jlonlat_t (INORM (iq+1, iloen2), jlat2);
        }
    }
  
}
 

