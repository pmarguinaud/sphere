#include <GL/glew.h>
#include <GLFW/glfw3.h>


#include "shader.h"
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <algorithm>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <unistd.h>
#include <png.h>
#include <sys/time.h>
#include "load.h"
#include "gensphere.h"


void write_png (const std::string & filename, int width, int height, unsigned char * pixels) 
{
  png_byte * png_bytes = NULL;
  png_byte ** png_rows = NULL;
  const size_t format_nchannels = 3;
  size_t nvals = format_nchannels * width * height;
  FILE * fp = fopen (filename.c_str (), "w");

  png_bytes = (png_byte *)malloc (nvals * sizeof (png_byte));
  png_rows = (png_byte **)malloc (height * sizeof (png_byte *));

  for (int i = 0; i < nvals; i++)
    png_bytes[i] = pixels[i];

  for (int i = 0; i < height; i++)
     png_rows[height-i-1] = &png_bytes[i*width*format_nchannels];

  png_structp png = png_create_write_struct (PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

  if (! png)  
    abort (); 

  png_infop info = png_create_info_struct (png);

  if (! info) 
    abort (); 

  if (setjmp (png_jmpbuf (png))) 
    abort (); 

  png_init_io (png, fp);

  png_set_IHDR (png, info, width, height, 8, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE,
                PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

  png_write_info (png, info);
  png_write_image (png, png_rows);
  png_write_end (png, NULL);
  png_destroy_write_struct (&png, &info);

  fclose (fp);

  free (png_bytes);
  free (png_rows);
}


unsigned int framebuffer;
unsigned int renderbuffer;
unsigned int texturebuffer;

void framebuffer_part1 (int width, int height)
{
  glGenFramebuffers (1, &framebuffer);
  glBindFramebuffer (GL_FRAMEBUFFER, framebuffer);
  
  glGenTextures (1, &texturebuffer);
  glBindTexture (GL_TEXTURE_2D, texturebuffer);
  glTexImage2D (GL_TEXTURE_2D, 0, GL_RGB, width,
                height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
  
  glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glFramebufferTexture2D (GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                          GL_TEXTURE_2D, texturebuffer, 0);
  
  glGenRenderbuffers (1, &renderbuffer);
  glBindRenderbuffer (GL_RENDERBUFFER, renderbuffer);
  
  glRenderbufferStorage (GL_RENDERBUFFER, GL_DEPTH32F_STENCIL8,
                         width, height);
  glFramebufferRenderbuffer (GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
                             GL_RENDERBUFFER, renderbuffer);
  
  if (glCheckFramebufferStatus (GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    throw std::runtime_error (std::string ("Framebuffer is not complete"));

}

void snapshot (int width, int height)  
{
  unsigned char * rgb = new unsigned char[width * height * 4];

  glReadPixels (0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, rgb);

  for (int i = 0; i < width * height; i++)
    for (int j = 0; j < 3; j++)
      rgb[3*i+j] = rgb[4*i+j];

  static int count = 0;
  char tmp[64];

  sprintf (tmp, "%10.10d.png", count++);
    
  std::string filename (tmp);
  write_png (filename, width, height, rgb);

  delete [] rgb;
}

void framebuffer_part2 (int width, int height)
{
  glDeleteRenderbuffers (1, &renderbuffer);
  glDeleteTextures (1, &texturebuffer);
  glDeleteFramebuffers (1, &framebuffer);
  glBindFramebuffer (GL_FRAMEBUFFER, 0);

}



static bool verbose = false;
static float lonc = 0.0f;
static float latc = 0.0f;
static float R = 6.0f;
static float fov = 20.0f;
static bool wireframe = false;
static bool rotate = false;

static 
void key_callback (GLFWwindow * window, int key, int scancode, int action, int mods)
{
  if (action == GLFW_PRESS || action == GLFW_REPEAT)
    {
      switch (key)
        {
          case GLFW_KEY_UP:
            latc += 5;
if (verbose)
std::cout << lonc << " " << latc << std::endl;
	    break;
          case GLFW_KEY_DOWN:
            latc -= 5;
if (verbose)
std::cout << lonc << " " << latc << std::endl;
	    break;
          case GLFW_KEY_LEFT:
            lonc -= 5;
if (verbose)
std::cout << lonc << " " << latc << std::endl;
	    break;
          case GLFW_KEY_RIGHT:
            lonc += 5;
if (verbose)
std::cout << lonc << " " << latc << std::endl;
	    break;
	  case GLFW_KEY_SPACE:
            lonc  = 0; latc = 0; R = 6.0; fov = 20.0f;
	    break;
	  case GLFW_KEY_F1:
            fov += 1;
if (verbose)
std::cout << fov << std::endl;
	    break;
	  case GLFW_KEY_F2:
            fov -= 1;
if (verbose)
std::cout << fov << std::endl;
	    break;
	  case GLFW_KEY_F3:
            wireframe = ! wireframe;
	    break;
	  case GLFW_KEY_TAB:
            rotate = ! rotate;
	    break;
	}
    }
}

bool endsWith (std::string const & fullString, std::string const & ending) 
{
  if (fullString.length () >= ending.length ()) 
    return (0 == fullString.compare (fullString.length () 
            - ending.length (), ending.length (), ending));
  return false;
}


int main (int argc, char * argv[])
{
  unsigned short * lonlat;
  int np; 
  unsigned int nt;
 
  geom_t geom;

  const int width = 1024, height = 1024;
  int w, h;
  unsigned char * r = NULL;
  float * F = NULL;

  gensphere (&geom, &np, &lonlat, &nt, &F, argv[1]);

  int size = 0;
  for (int jlat = 1; jlat <= geom.Nj; jlat++)
    size += geom.pl[jlat-1];


  float minval = 233.15;
  float maxval = 313.15;
  r = (unsigned char *)malloc (sizeof (unsigned char) * size);
  for (int i = 0; i < size; i++)
    r[i] = 255 * (std::min (maxval, std::max (minval, F[i])) - minval) / (maxval - minval);


  if (! glfwInit ()) 
    {   
      fprintf (stderr, "Failed to initialize GLFW\n");
      return -1;
    }   

  GLFWwindow * window;
  
  glfwWindowHint (GLFW_SAMPLES, 4);
  glfwWindowHint (GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint (GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint (GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
  glfwWindowHint (GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  window = glfwCreateWindow (width, height, "", NULL, NULL);
    
  if (window == NULL)
    { 
      fprintf (stderr, "Failed to open GLFW window\n");
      glfwTerminate ();
      return -1;
    }
  
  glfwSetKeyCallback (window, key_callback);

  glfwMakeContextCurrent (window);
  
  glewExperimental = true; 
  if (glewInit () != GLEW_OK)
    {
      fprintf (stderr, "Failed to initialize GLEW\n");
      glfwTerminate ();
      return -1;
    }


  glClearColor (0.0f, 0.0f, 0.0f, 0.0f);
  glEnable (GL_DEPTH_TEST);
  glEnable (GL_BLEND);
  glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

if(1){
  glCullFace (GL_BACK);
  glFrontFace (GL_CCW);
  glEnable (GL_CULL_FACE);
}

  glDepthFunc (GL_LESS); 



  // Sphere
  GLuint VertexArrayID;
  GLuint vertexbuffer, colorbuffer, elementbuffer;

  glGenVertexArrays (1, &VertexArrayID);
  glBindVertexArray (VertexArrayID);

  glGenBuffers (1, &vertexbuffer);
  glBindBuffer (GL_ARRAY_BUFFER, vertexbuffer);
  glBufferData (GL_ARRAY_BUFFER, 2 * np * sizeof (unsigned short), lonlat, GL_STATIC_DRAW);
  glEnableVertexAttribArray (0); 
  glVertexAttribPointer (0, 2, GL_UNSIGNED_SHORT, GL_TRUE, 0, NULL); 
  free (lonlat);
  lonlat = NULL;


  const int ncol = 1;
  glGenBuffers (1, &colorbuffer);
  glBindBuffer (GL_ARRAY_BUFFER, colorbuffer);
  glBufferData (GL_ARRAY_BUFFER, ncol * np * sizeof (unsigned char), r, GL_STATIC_DRAW);
  glEnableVertexAttribArray (1); 
  glVertexAttribPointer (1, ncol, GL_UNSIGNED_BYTE, GL_TRUE, ncol * sizeof (unsigned char), NULL); 
  free (r);
  r = NULL;

  glGenBuffers (1, &elementbuffer);
  glBindBuffer (GL_ELEMENT_ARRAY_BUFFER, elementbuffer);
  glBufferData (GL_ELEMENT_ARRAY_BUFFER, geom.ind_strip_size * sizeof (unsigned int), geom.ind_strip , GL_STATIC_DRAW);

  GLuint programID = shader 
(
R"CODE(
#version 330 core

in float fragmentVal;

out vec4 fragmentColor;

void main()
{
  vec4 b = vec4 (0.0f, 0.0f, 1.0f, 1.0);
  vec4 w = vec4 (1.0f, 1.0f, 1.0f, 1.0);
  vec4 r = vec4 (1.0f, 0.0f, 0.0f, 1.0);

  float x1 = fragmentVal / 0.5f;
  float x2 = (fragmentVal - 0.5f) / 0.5f;
  bool bb = fragmentVal > 0.5f;
  float wb = bb ? 0.0f : 1.0f - x1;
  float ww = bb ? 1.0f - x2 : x1;
  float wr = bb ? x2 : 0.0f;

  fragmentColor = wb * b + ww * w + wr * r;
}
)CODE",
R"CODE(
#version 330 core

layout(location = 0) in vec2 vertexPos;
layout(location = 1) in float vertexVal;

out float fragmentVal;

uniform mat4 MVP;


void main()
{
  const float pi =  3.14159265358979323846;

  float lon = 2 * pi * vertexPos.x;
  float lat = 1 * pi * vertexPos.y - pi / 2.;

  float coslon = cos (lon), sinlon = sin (lon);
  float coslat = cos (lat), sinlat = sin (lat);

  float X = coslon * coslat;
  float Y = sinlon * coslat;
  float Z =          sinlat;

  vec3 pos = 0.99 * vec3 (X, Y, Z);
  gl_Position = MVP * vec4 (pos, 1);

  fragmentVal = vertexVal;
}
)CODE");

  int count = 0;
  while (1) 
    {   
      glm::mat4 Projection = glm::perspective (glm::radians (fov), 1.0f, 0.1f, 100.0f);
      glm::mat4 View       = glm::lookAt (glm::vec3 (R * cos (lonc * M_PI / 180.0f) * cos (latc * M_PI / 180.0f), 
                                                     R * sin (lonc * M_PI / 180.0f) * cos (latc * M_PI / 180.0f),
                                                     R * sin (latc * M_PI / 180.0f)), 
                                          glm::vec3 (0,0,0), glm::vec3 (0,0,1));
      glm::mat4 Model      = glm::mat4 (1.0f);
      glm::mat4 MVP = Projection * View * Model; 

      glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

      // Sphere
      glUseProgram (programID);
      glUniformMatrix4fv (glGetUniformLocation (programID, "MVP"), 1, GL_FALSE, &MVP[0][0]);
      glBindVertexArray (VertexArrayID);
      if (wireframe)
        glPolygonMode (GL_FRONT_AND_BACK, GL_LINE);
      else
        glPolygonMode (GL_FRONT_AND_BACK, GL_FILL);

      glEnable (GL_PRIMITIVE_RESTART);
      glPrimitiveRestartIndex (0xffffffff);

      int jlat1 = 0;
      int jlat2 = geom.Nj;
   
      int k1 = geom.ind_stripoff_per_lat[jlat1];
      int k2 = geom.ind_stripoff_per_lat[jlat2];

      glDrawElements (GL_TRIANGLE_STRIP, k2 - k1, GL_UNSIGNED_INT, (void *)(sizeof (unsigned int) * k1));

      glPolygonMode (GL_FRONT_AND_BACK, GL_FILL);


      glfwSwapBuffers (window);
      glfwPollEvents (); 

      if(0)
      if (count > 2)
      snapshot (width, height);

      if(0)
      if (count++ > 300)
	break;
  
      if (glfwGetKey (window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        break;
      if (glfwWindowShouldClose (window) != 0)  
        break;

      if (rotate)
        lonc += 1;
    }   


  glfwTerminate ();


  return 0;
}
