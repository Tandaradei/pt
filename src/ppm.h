#ifndef PPM_H
#define PPM_H

#include <stdio.h>
 
void write_ppm(const char *filename, unsigned int dim_x, unsigned int dim_y, unsigned const char* data)
{
  FILE * fp = fopen(filename, "wb");
  /* write header to the file */
  fprintf(fp, "P6\n%d %d\n255\n", dim_x, dim_y);
  /* write image data bytes to the file */
  fwrite(data, sizeof(char), dim_x * dim_y * 3, fp);
  fclose(fp);
}

#endif // PPM_H