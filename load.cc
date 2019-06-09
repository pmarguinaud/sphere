#include "load.h"
#include <eccodes.h>

void glgrib_load_rgb (const char * file, unsigned char ** col)
{
  FILE * in = NULL;
  int ncol = 3;
  
  in = fopen (file, "r");

  *col = NULL;

  for (int ifld = 0; ifld < 3; ifld++)
    {   
      int err = 0;
      size_t v_len = 0;
      codes_handle * h = codes_handle_new_from_file (0, in, PRODUCT_GRIB, &err);
      codes_get_size (h, "values", &v_len);
     
      double vmin, vmax, vmis;
      double * v = (double *)malloc (v_len * sizeof (double));

      codes_get_double_array (h, "values", v, &v_len);
      codes_get_double (h, "maximum",      &vmax);
      codes_get_double (h, "minimum",      &vmin);
      codes_get_double (h, "missingValue", &vmis);
      codes_handle_delete (h);
     
      if (*col == NULL)
        *col = (unsigned char *)malloc (ncol * sizeof (unsigned char) * v_len);

      for (int jglo = 0; jglo < v_len; jglo++)
        (*col)[ncol*jglo+ifld] = 255 * v[jglo];
     
      free (v);
    }   

  fclose (in);
}

