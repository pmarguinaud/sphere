#ifndef _GENSPHERE_H
#define _GENSPHERE_H

#include <string>
#include <vector>


class geom_t
{
public:
  long int Nj = 0;
  long int * pl = NULL;
  int * jglooff = NULL;
};

void gensphere (geom_t *, int *, float **, 
                unsigned int *, unsigned int **, 
	        float **, const std::string &);

void gensphere_grib 
               (geom_t *, int *, float **, 
                unsigned int *, unsigned int **, 
                float **, const std::string &);


#endif
