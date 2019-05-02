#include <GL/glew.h>
#include <GLFW/glfw3.h>


#include "bmp.h"
#include "gensphere.h"
#include "shader.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <openssl/md5.h>
#include <unistd.h>


static const char * type2str (GLint type)
{
#define case_type(x) case x: return #x
  switch (type)
    {
      case_type (GL_FLOAT);
      case_type (GL_FLOAT_VEC2);
      case_type (GL_FLOAT_VEC3);
      case_type (GL_FLOAT_VEC4);
      case_type (GL_DOUBLE);
      case_type (GL_DOUBLE_VEC2);
      case_type (GL_DOUBLE_VEC3);
      case_type (GL_DOUBLE_VEC4);
      case_type (GL_INT);
      case_type (GL_INT_VEC2);
      case_type (GL_INT_VEC3);
      case_type (GL_INT_VEC4);
      case_type (GL_UNSIGNED_INT);
      case_type (GL_UNSIGNED_INT_VEC2);
      case_type (GL_UNSIGNED_INT_VEC3);
      case_type (GL_UNSIGNED_INT_VEC4);
      case_type (GL_BOOL);
      case_type (GL_BOOL_VEC2);
      case_type (GL_BOOL_VEC3);
      case_type (GL_BOOL_VEC4);
      case_type (GL_FLOAT_MAT2);
      case_type (GL_FLOAT_MAT3);
      case_type (GL_FLOAT_MAT4);
      case_type (GL_FLOAT_MAT2x3);
      case_type (GL_FLOAT_MAT2x4);
      case_type (GL_FLOAT_MAT3x2);
      case_type (GL_FLOAT_MAT3x4);
      case_type (GL_FLOAT_MAT4x2);
      case_type (GL_FLOAT_MAT4x3);
      case_type (GL_DOUBLE_MAT2);
      case_type (GL_DOUBLE_MAT3);
      case_type (GL_DOUBLE_MAT4);
      case_type (GL_DOUBLE_MAT2x3);
      case_type (GL_DOUBLE_MAT2x4);
      case_type (GL_DOUBLE_MAT3x2);
      case_type (GL_DOUBLE_MAT3x4);
      case_type (GL_DOUBLE_MAT4x2);
      case_type (GL_DOUBLE_MAT4x3);
      case_type (GL_SAMPLER_1D);
      case_type (GL_SAMPLER_2D);
      case_type (GL_SAMPLER_3D);
      case_type (GL_SAMPLER_CUBE);
      case_type (GL_SAMPLER_1D_SHADOW);
      case_type (GL_SAMPLER_2D_SHADOW);
      case_type (GL_SAMPLER_1D_ARRAY);
      case_type (GL_SAMPLER_2D_ARRAY);
      case_type (GL_SAMPLER_1D_ARRAY_SHADOW);
      case_type (GL_SAMPLER_2D_ARRAY_SHADOW);
      case_type (GL_SAMPLER_2D_MULTISAMPLE);
      case_type (GL_SAMPLER_2D_MULTISAMPLE_ARRAY);
      case_type (GL_SAMPLER_CUBE_SHADOW);
      case_type (GL_SAMPLER_BUFFER);
      case_type (GL_SAMPLER_2D_RECT);
      case_type (GL_SAMPLER_2D_RECT_SHADOW);
      case_type (GL_INT_SAMPLER_1D);
      case_type (GL_INT_SAMPLER_2D);
      case_type (GL_INT_SAMPLER_3D);
      case_type (GL_INT_SAMPLER_CUBE);
      case_type (GL_INT_SAMPLER_1D_ARRAY);
      case_type (GL_INT_SAMPLER_2D_ARRAY);
      case_type (GL_INT_SAMPLER_2D_MULTISAMPLE);
      case_type (GL_INT_SAMPLER_2D_MULTISAMPLE_ARRAY);
      case_type (GL_INT_SAMPLER_BUFFER);
      case_type (GL_INT_SAMPLER_2D_RECT);
      case_type (GL_UNSIGNED_INT_SAMPLER_1D);
      case_type (GL_UNSIGNED_INT_SAMPLER_2D);
      case_type (GL_UNSIGNED_INT_SAMPLER_3D);
      case_type (GL_UNSIGNED_INT_SAMPLER_CUBE);
      case_type (GL_UNSIGNED_INT_SAMPLER_1D_ARRAY);
      case_type (GL_UNSIGNED_INT_SAMPLER_2D_ARRAY);
      case_type (GL_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE);
      case_type (GL_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE_ARRAY);
      case_type (GL_UNSIGNED_INT_SAMPLER_BUFFER);
      case_type (GL_UNSIGNED_INT_SAMPLER_2D_RECT);
    }
#undef case_type
  return "GL_UNKNOWN";
}

void calc_md5 (const char * buf, int len, unsigned char out[])
{
  MD5_CTX c;
  MD5_Init (&c);
  MD5_Update (&c, buf, len);
  MD5_Final (out, &c);
}

int main (int argc, char * argv[])
{
  int Nj = atoi (argv[1]);
  int np; 
  int * pl;
  unsigned int nt;
  unsigned int * ind;
  const int width = 1024, height = 1024;
  int w, h;
  unsigned char * rgb = NULL;
  unsigned char md5[MD5_DIGEST_LENGTH];
  float * latitudes;
  int jglooff[Nj+1];

//bmp ("10px-SNice.svg.bmp", &rgb, &w, &h);
//bmp ("800px-SNice.svg.bmp", &rgb, &w, &h);
  bmp ("Whole_world_-_land_and_oceans_8000.bmp", &rgb, &w, &h);

  gensphere1 (Nj, &np, NULL, &nt, &ind, &pl, &latitudes);

  jglooff[0] = 0;
  for (int jlat = 2; jlat <= Nj+1; jlat++)
    jglooff[jlat-1] = jglooff[jlat-2] + pl[jlat-2];

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
  glCullFace (GL_BACK);
  glFrontFace (GL_CCW);
  glEnable (GL_CULL_FACE);
  glDepthFunc (GL_LESS); 



  GLuint VertexArrayID;
  GLuint elementbuffer;

  glGenVertexArrays (1, &VertexArrayID);
  glBindVertexArray (VertexArrayID);

  glGenBuffers (1, &elementbuffer);
  glBindBuffer (GL_ELEMENT_ARRAY_BUFFER, elementbuffer);
  glBufferData (GL_ELEMENT_ARRAY_BUFFER, 3 * nt * sizeof (unsigned int), ind , GL_STATIC_DRAW);

  GLuint programID = shader 
(
R"CODE(
#version 330 core

in vec4 fragmentColor;

out vec4 color;

void main()
{
  color.r = fragmentColor.r;
  color.g = fragmentColor.g;
  color.b = fragmentColor.b;
  color.a = fragmentColor.a;
}
)CODE",
R"CODE(
#version 330 core


out vec4 fragmentColor;

layout (shared) uniform pl_block
{
  int pl[8000];     
};

layout (shared) uniform jglooff_block
{
  int jglooff[8000];     
};

layout (shared) uniform latitudes_block
{
  float latitudes[8000];     
};

uniform mat4 MVP;
uniform sampler2D texture;
uniform int Nj;

void main ()
{
  const float pi = 3.1415926;
  int jglo = gl_VertexID;

  int jlat, jlon;

  // Lookup latitude & longitude indices
  if ((jglooff[0] <= jglo) && (jglo <= jglooff[Nj]))
    {
      int jlat1 = 0, jlat2 = Nj;
  
      while (jlat1 < jlat2-1)
        {
          int jlatm = (jlat1 + jlat2) / 2;
          if ((jglooff[jlat1] <= jglo) && (jglo <= jglooff[jlatm]))
            jlat2 = jlatm;
          else
            jlat1 = jlatm;
        }
  
      jlat = jlat1;

      jlon = jglo - jglooff[jlat];

    }
  else
    {
      // Should not happen !!
    }

  float lon, lat;

  lon = 2.0f * pi * float (jlon) / pl[jlat];
  lat = latitudes[jlat];

  float sinlat = sin (lat), coslat = cos (lat);
  float sinlon = sin (lon), coslon = cos (lon);

  vec3 xyz = vec3 (coslon * coslat, sinlon * coslat, sinlat);

  gl_Position =  MVP * vec4 (xyz, 1);

  float xlon = (lon / pi + 1.0) * 0.5;
  float xlat = lat / pi + 0.5;

  vec4 col = texture2D (texture, vec2 (xlon, xlat));
  fragmentColor.r = col.r;
  fragmentColor.g = col.g;
  fragmentColor.b = col.b;
  fragmentColor.a = 1.;

}
)CODE");

  glUseProgram (programID);

  glm::mat4 Projection = glm::perspective (glm::radians (20.0f), 1.0f / 1.0f, 0.1f, 100.0f);
  glm::mat4 View       = glm::lookAt (glm::vec3 (6.0f,0.0f,0.0f), glm::vec3 (0,0,0), glm::vec3 (0,0,1));
  glm::mat4 Model      = glm::mat4 (1.0f);

  glm::mat4 MVP = Projection * View * Model; 

  glUniformMatrix4fv (glGetUniformLocation (programID, "MVP"), 1, GL_FALSE, &MVP[0][0]);
  glUniform1i (glGetUniformLocation (programID, "Nj"), Nj);

  unsigned int texture;
  glGenTextures (1, &texture);
  glBindTexture (GL_TEXTURE_2D, texture); 
  glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	// set texture wrapping to GL_REPEAT (default wrapping method)
  glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexImage2D (GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, rgb);

  glUniform1i (glGetUniformLocation (programID, "texture"), 0);



  unsigned int pl_buf;
  glGenBuffers (1, &pl_buf);
  glBindBuffer (GL_UNIFORM_BUFFER, pl_buf);
  glBufferData (GL_UNIFORM_BUFFER, Nj * sizeof (int), NULL, GL_STATIC_DRAW); 
  
  unsigned int pl_block = glGetUniformBlockIndex (programID, "pl_block");
  glUniformBlockBinding (programID, pl_block, 1);
  glBindBufferBase (GL_UNIFORM_BUFFER, 1, pl_buf); 
  
  glBindBuffer (GL_UNIFORM_BUFFER, pl_buf);
  glBufferSubData (GL_UNIFORM_BUFFER, 0, Nj * sizeof (int), pl); 
  glBindBuffer (GL_UNIFORM_BUFFER, 0);


  unsigned int jglooff_buf;
  glGenBuffers (1, &jglooff_buf);
  glBindBuffer (GL_UNIFORM_BUFFER, jglooff_buf);
  glBufferData (GL_UNIFORM_BUFFER, (Nj + 1) * sizeof (int), NULL, GL_STATIC_DRAW); 
  
  unsigned int jglooff_block = glGetUniformBlockIndex (programID, "jglooff_block");
  glUniformBlockBinding (programID, jglooff_block, 2);
  glBindBufferBase (GL_UNIFORM_BUFFER, 2, jglooff_buf); 
  
  glBindBuffer (GL_UNIFORM_BUFFER, jglooff_buf);
  glBufferSubData (GL_UNIFORM_BUFFER, 0, (Nj + 1) * sizeof (int), jglooff); 
  glBindBuffer (GL_UNIFORM_BUFFER, 0);


  unsigned int latitudes_buf;
  glGenBuffers (1, &latitudes_buf);
  glBindBuffer (GL_UNIFORM_BUFFER, latitudes_buf);
  glBufferData (GL_UNIFORM_BUFFER, Nj * sizeof (float), NULL, GL_STATIC_DRAW); 
  
  unsigned int latitudes_block = glGetUniformBlockIndex (programID, "latitudes_block");
  glUniformBlockBinding (programID, latitudes_block, 3);
  glBindBufferBase (GL_UNIFORM_BUFFER, 3, latitudes_buf); 
  
  glBindBuffer (GL_UNIFORM_BUFFER, latitudes_buf);
  glBufferSubData (GL_UNIFORM_BUFFER, 0, Nj * sizeof (float), latitudes); 
  glBindBuffer (GL_UNIFORM_BUFFER, 0);


  while (1) 
    {   
      glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

      glBindVertexArray (VertexArrayID);
      glDrawElements (GL_TRIANGLES, 3 * nt, GL_UNSIGNED_INT, NULL);

      glfwSwapBuffers (window);
      glfwPollEvents (); 
  
      if (glfwGetKey (window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        break;
      if (glfwWindowShouldClose (window) != 0)  
        break;
    }   


  glfwTerminate ();


  return 0;
}
