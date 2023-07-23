#include <eccodes.h>
#include <cstdio>
#include "grib.h"

void grib (const std::string & path, std::vector<double> * v, long int * nlon, long int * nlat)
{
  codes_handle * h = nullptr;
  size_t v_len;
  int err;
  FILE * fp = fopen (&path[0], "r");
  h = codes_handle_new_from_file (0, fp, PRODUCT_GRIB, &err);
  fclose (fp);
  codes_get_size (h, "values", &v_len);
  v->resize (v_len);
  codes_get_double_array (h, "values", &(*v)[0], &v_len);
  codes_get_long (h, "Ni", nlon);
  codes_get_long (h, "Nj", nlat);
  codes_handle_delete (h);
}
