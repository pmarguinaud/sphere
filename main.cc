#include <GL/glew.h>
#include <GLFW/glfw3.h>


#include "bmp.h"
#include "grib.h"
#include "gensphere.h"
#include "shader.h"
#include <iostream>
#include <algorithm>
#include <vector>
#include <stdexcept>
#include <cmath>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>


class tex
{
  public:

  int width, height;
  unsigned int texture;
  float Vmin, Vmax;

  tex (const std::string & path) 
  {
    std::vector<unsigned char> rgb;
    bmp (path, &rgb, &width, &height);
    glGenTextures (1, &texture);
    glBindTexture (GL_TEXTURE_2D, texture); 
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D (GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, &rgb[0]);
  }

  tex (const std::string & u, const std::string & v)
  {
    std::vector<double> val_u, val_v;
    long int nlon_u, nlat_u, nlon_v, nlat_v;
    grib (u, &val_u, &nlon_u, &nlat_u);
    grib (v, &val_v, &nlon_v, &nlat_v);

    double umax = *( std::max_element (val_u.begin (), val_u.end ()) );
    double umin = *( std::min_element (val_u.begin (), val_u.end ()) );

    double vmax = *( std::max_element (val_v.begin (), val_v.end ()) );
    double vmin = *( std::min_element (val_v.begin (), val_v.end ()) );

    Vmin = std::min (umin, vmin);
    Vmax = std::max (umax, vmax);

    if ((nlon_u != nlon_v) || (nlat_u != nlat_v))
      throw std::runtime_error ("Dimension mismatch");

    width = nlon_u; height = nlat_u;

    std::vector<unsigned char> rg (width * height * 2);

    for (int i = 0; i < width * height; i++)
      {
        double un = (val_u[i] - Vmin) / (Vmax - Vmin);
        double vn = (val_v[i] - Vmin) / (Vmax - Vmin);
        unsigned char r = un * std::numeric_limits<unsigned char>::max ();
        unsigned char g = vn * std::numeric_limits<unsigned char>::max ();
        rg[2*i+0] = r; rg[2*i+1] = g;

//      Beautiful !
//      rgba[4*i+1] = 0;
//      rgba[4*i+3] = 255;

//      rgba[4*i+0] = 255; 
//      rgba[4*i+1] =   0; 
//      rgba[4*i+2] = 120; 
//      rgba[4*i+3] =   0; 

      }

    glGenTextures (1, &texture);
    glBindTexture (GL_TEXTURE_2D, texture); 
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D (GL_TEXTURE_2D, 0, GL_RG8, width, height, 0, GL_RG, GL_UNSIGNED_BYTE, &rg[0]);
  }

  tex (int _width, int _height) : width (_width), height (_height)
  {
    glGenTextures (1, &texture);
    glBindTexture (GL_TEXTURE_2D, texture); 
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D (GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
  }

  void bind (GLuint target) const
  {
//  if (target != 0)
//    throw std::runtime_error ("Unexpected texture target");
    glActiveTexture (GL_TEXTURE0 + target);
    glBindTexture (GL_TEXTURE_2D, texture);
  }

  ~tex ()
  {
    glDeleteTextures (1, &texture);
  }

};

class texModifier
{
  public:
  unsigned framebuffer;

  texModifier (const tex & tt)
  {
    glGenFramebuffers (1, &framebuffer);
    glBindFramebuffer (GL_FRAMEBUFFER, framebuffer);
    glFramebufferTexture2D (GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tt.texture, 0);
    glViewport (0, 0, tt.width, tt.height);
  }

  ~texModifier ()
  {
    glDeleteFramebuffers (1, &framebuffer);
    glBindFramebuffer (GL_FRAMEBUFFER, 0); 
  }
};

class dotterCompute
{
public:
  GLuint programID;
  dotterCompute ()
  {
    programID = shader (nullptr, nullptr, 
R"CODE(
#version 430 core

layout (local_size_x = 32, local_size_y = 32, local_size_z = 1) in;
layout (rgba16f) uniform image2D imgOutput;

void main () 
{
  ivec2 center = ivec2 (800, 400);
  int r = 10;
  
  int ix, iy;

  vec4 value = vec4 (1., 1., 1., 1.);
  for (ix = -r; ix <= r; ix++)
    {
      for (iy = -r; iy <= r; iy++)
        {
          float dist = sqrt (ix * ix + iy * iy);
          if (dist < r)
            {
              ivec2 texelCoord = ivec2 (ix, iy) + center;
              imageStore (imgOutput, texelCoord, value);
            }
        }
    }
}
)CODE");


  } 
  void apply (tex & tt)
  {
    glUseProgram (programID);
    tt.bind (0);
    glBindImageTexture (0, tt.texture, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA16F);

    glDispatchCompute (1, 1, 1);
    glMemoryBarrier (GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
  }
  ~dotterCompute ()
  {
    glDeleteProgram (programID);
  }
};

class faderCompute
{
public:
  GLuint programID;
  faderCompute ()
  {
    programID = shader (nullptr, nullptr, 
R"CODE(
#version 430 core

layout (local_size_x = 32, local_size_y = 32, local_size_z = 1) in;
layout (rgba16f) uniform image2D imgOutput;

uniform float scale = 1.0;

void main () 
{
  vec4 value;

  ivec2 texelCoord = ivec2 (gl_GlobalInvocationID.xy);
  
  value = imageLoad (imgOutput, texelCoord);

  value.a = value.a * scale;

  imageStore (imgOutput, texelCoord, value);
}
)CODE");


  } 
  void apply (tex & tt, float scale)
  {
    glUseProgram (programID);
    tt.bind (0);
    glBindImageTexture (0, tt.texture, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA16F);
    glUniform1f (glGetUniformLocation (programID, "scale"), scale);

    int W = (tt.width+31)/32, H = (tt.height+31)/32;
    glDispatchCompute (W, H, 1);
    glMemoryBarrier (GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
  }
  ~faderCompute ()
  {
    glDeleteProgram (programID);
  }
};

class moverCompute
{
public:
  GLuint programID;
  float R;
  moverCompute (float _R) : R (_R)
  {
    programID = shader (nullptr, nullptr, 
R"CODE(
#version 430 core

layout (local_size_x = 32, local_size_y = 32, local_size_z = 1) in;

layout (std430, binding=0) buffer lonLat
{
  float lonlat[];
};

uniform sampler2D texuv;
uniform float Vmin;
uniform float Vmax;
uniform float R = 0.014;

const float pi = 3.1415927;

float rand (const vec2 co) 
{
  float t = dot (vec2 (12.9898, 78.233), co);
  return fract (sin (t) * (4375.85453 + t));
}

void main () 
{
  int j = int (gl_GlobalInvocationID.x);

  float x = 0.5 *(lonlat[2*j+0]+1.0);
  float y = 0.5 *(lonlat[2*j+1]+1.0);

  float coslat = cos (0.5 * pi * (lonlat[2*j+1]));

  vec2 seed = vec2 (x, y);

  vec4 uv = texture (texuv, vec2 (x, y));
  float u = uv[0]; 
  float v = uv[1]; 

  float drop = (0.8 + 0.2 * sqrt (u * u + v * v));

  u = (Vmin + u * (Vmax - Vmin)) / Vmax; u = u / coslat;
  v = (Vmin + v * (Vmax - Vmin)) / Vmax;

  if ((rand (seed) > drop))
    {
      x = rand (seed + 1.3);
      y = rand (seed + 2.1);
      lonlat[2*j+0] = clamp (2.0 * x - 1.0, -1.0, +1.0);
      lonlat[2*j+1] = clamp (2.0 * y - 1.0, -1.0, +1.0);
    }
  else
    {
     
//    u = 1.;
//    v = 0.;
     
      lonlat[2*j+0] = lonlat[2*j+0] + (R/2) * u;
      lonlat[2*j+1] = lonlat[2*j+1] + (R/2) * v;
     
      if (lonlat[2*j+1] > +1.0)
        {
          lonlat[2*j+0] = +1.0 + lonlat[2*j+0];
          lonlat[2*j+1] = +1.0 - (lonlat[2*j+1] - 1.0);
        }
      if (lonlat[2*j+1] < -1.0)
        {
          lonlat[2*j+0] = +1.0 + lonlat[2*j+0];
          lonlat[2*j+1] = -1.0 - (lonlat[2*j+1] + 1.0);
        }
     
      lonlat[2*j+0] = -1.0 + mod (lonlat[2*j+0] + 1.0, 2.0);
    }
}
)CODE");


  } 
  void apply (GLuint bufferid, unsigned int buffer_size, tex & ttuv)
  {
    glUseProgram (programID);
    ttuv.bind (0);
    glUniform1i (glGetUniformLocation (programID, "texuv"), 0);
    glUniform1f (glGetUniformLocation (programID, "Vmin"), ttuv.Vmin);
    glUniform1f (glGetUniformLocation (programID, "Vmax"), ttuv.Vmax);
    glUniform1f (glGetUniformLocation (programID, "R"), R);
    glBindBufferBase (GL_SHADER_STORAGE_BUFFER, 0, bufferid);

    int W = (buffer_size+31)/32;
    glDispatchCompute (W, 1, 1);
    glMemoryBarrier (GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
  }
  ~moverCompute ()
  {
    glDeleteProgram (programID);
  }
};

class checkerRender
{
public:
  const unsigned int nt = 2;
  const int Nx, Ny;
  GLuint programID;
  GLuint VertexArrayID;
  checkerRender (int _Nx, int _Ny) : Nx (_Nx), Ny (_Ny)
  {
    programID = shader 
(
R"CODE(
#version 420 core

in float instance;

out vec4 color;

void main ()
{
  color = vec4 (0., 0., 0., 0.);

  int i = int (instance) % 3;
  
  color[i] = 1.;  

  color.a = 1.;
}
)CODE",
R"CODE(
#version 420 core

uniform int Nx = 0;
uniform int Ny = 0;

out float instance;

void main ()
{
  int i = gl_VertexID;
  int j = gl_InstanceID;
  float dx = 2. / float (Nx);
  float dy = 2. / float (Ny);

  instance = j;

  int jx = j % Nx;
  int jy = j / Nx;

  vec2 vertexPos;
  if (i == 0) 
    vertexPos = vec2 (0.0f, 0.0f);
  else if (i == 1)
    vertexPos = vec2 (  dx, 0.0f);
  else if (i == 2)
    vertexPos = vec2 (  dx,   dy);
  else if (i == 3)
    vertexPos = vec2 (0.0f,   dy);

  float x = float (jx) * dx;
  float y = float (jy) * dy;

  vertexPos = vertexPos + vec2 (x-1.0, y-1.0);

  gl_Position = vec4 (vertexPos.x, vertexPos.y, 0., 1.);
}
)CODE");

    glUseProgram (programID);
    
    glGenVertexArrays (1, &VertexArrayID);
    glBindVertexArray (VertexArrayID);
  }

  void render (tex & tt) const
  {
    glViewport (0, 0, tt.width, tt.height);
    glUseProgram (programID);
    glBindVertexArray (VertexArrayID);
    glUniform1i (glGetUniformLocation (programID, "Nx"), Nx);
    glUniform1i (glGetUniformLocation (programID, "Ny"), Ny);
    const std::vector<unsigned int> ind = {0, 1, 2, 0, 2, 3}; 
    glDrawElementsInstanced (GL_TRIANGLES, 3 * nt, GL_UNSIGNED_INT, &ind[0], Nx * Ny);
  }

  void render () const
  {
    glUseProgram (programID);
    glBindVertexArray (VertexArrayID);
    glUniform1i (glGetUniformLocation (programID, "Nx"), Nx);
    glUniform1i (glGetUniformLocation (programID, "Ny"), Ny);
    const std::vector<unsigned int> ind = {0, 1, 2, 0, 2, 3}; 
    glDrawElementsInstanced (GL_TRIANGLES, 3 * nt, GL_UNSIGNED_INT, &ind[0], Nx * Ny);
  }

  ~checkerRender ()
  {
    glDeleteVertexArrays (1, &VertexArrayID);
    glDeleteProgram (programID);
  }
};

class checkerCompute
{
public:
  const int Nx, Ny;
  GLuint programID;
  checkerCompute (int _Nx, int _Ny) : Nx (_Nx), Ny (_Ny)
  {
    programID = shader (nullptr, nullptr,
R"CODE(
#version 430 core

layout (local_size_x = 32, local_size_y = 32, local_size_z = 1) in;
layout (rgba16f) uniform image2D imgOutput;

uniform int Nx;
uniform int Ny;
uniform int Sx;
uniform int Sy;

void main ()
{
  ivec2 texelCoord = ivec2 (gl_GlobalInvocationID.xy);
  
  int jx = int (Nx * float (texelCoord.x) / float (Sx));
  int jy = int (Ny * float (texelCoord.y) / float (Sy));

  int j = jx + Nx * jy;

  vec4 value = vec4 (0., 0., 0., 1.);
  
  j = j % 3;

  value[j] = 1.;

  imageStore (imgOutput, texelCoord, value);
}
)CODE");

    glUseProgram (programID);
  }

  void apply (tex & tt) const
  {
    glUseProgram (programID);
    glUniform1i (glGetUniformLocation (programID, "Nx"), Nx);
    glUniform1i (glGetUniformLocation (programID, "Ny"), Ny);
    glUniform1i (glGetUniformLocation (programID, "Sx"), tt.width);
    glUniform1i (glGetUniformLocation (programID, "Sy"), tt.height);
    tt.bind (0);
    glBindImageTexture (0, tt.texture, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA16F);

    int W = (tt.width+31)/32, H = (tt.height+31)/32;
    glDispatchCompute (W, H, 1);
    glMemoryBarrier (GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
  }

  ~checkerCompute ()
  {
    glDeleteProgram (programID);
  }
};

class dotterRender
{
public:
  float R;
  GLuint programID;
  GLuint VertexArrayID;
  std::vector<unsigned int> ind;
  dotterRender (float _R = 0.01) : R (_R)
  {
    ind = {0, 1, 2, 0, 2, 3};

    programID = shader 
(
R"CODE(
#version 440 core

in vec2 coords;
in float skip;
out vec4 color;

void main ()
{
  if (skip > 0)
    discard;
  color = vec4 (1., 1., 1., 1.);
  color.a = exp (- 2 * sqrt (4 * coords.x * coords.x + coords.y * coords.y));
}
)CODE",
R"CODE(
#version 440 core

out vec2 coords;
out float skip;

uniform float R;

const float pi = 3.1415927;

uniform float Vmin;
uniform float Vmax;

layout (std430, binding=0) buffer lonLat
{
  float lonlat[];
};

void main ()
{
  int j = gl_InstanceID;

  coords = vec2 (0., 0.);
  skip = 0.;

//if (j != 250)
//  {
//    skip = 1.;
//    return;
//  }

  int i = gl_VertexID;

  float coslat = cos (0.5 * pi * (lonlat[2*j+1]));

  vec2 pos;

  if (i == 0)
    pos = vec2 (-1.0, -2.0);
  else if (i == 1)
    pos = vec2 (+1.0, -2.0);
  else if (i == 2)
    pos = vec2 (+1.0, +2.0);
  else if (i == 3)
    pos = vec2 (-1.0, +2.0);

  coords = vec2 (pos.x, pos.y);

  pos = pos * R;

  pos = vec2 (pos.x / coslat, pos.y) + vec2 (lonlat[2*j+0], lonlat[2*j+1]);

  gl_Position = vec4 (pos.x, pos.y, 0., 1.);
 
}
)CODE");

    glUseProgram (programID);
    glGenVertexArrays (1, &VertexArrayID);
    glBindVertexArray (VertexArrayID);
  }

  void render (tex & tt, GLuint bufferid, unsigned int buffer_size) const
  {
    glViewport (0, 0, tt.width, tt.height);
    glUseProgram (programID);
    glBindVertexArray (VertexArrayID);
    glUniform1f (glGetUniformLocation (programID, "R"), R);
    glBindBufferBase (GL_SHADER_STORAGE_BUFFER, 0, bufferid);
    glDrawElementsInstanced (GL_TRIANGLES, 3 * 2, GL_UNSIGNED_INT, &ind[0], buffer_size);
  }

  ~dotterRender ()
  {
    glDeleteVertexArrays (1, &VertexArrayID);
    glDeleteProgram (programID);
  }
};

class faderRender
{
public:
  const unsigned int nt = 2;
  GLuint programID;
  GLuint VertexArrayID;
  faderRender () 
  {
    programID = shader 
(
R"CODE(
#version 420 core

out vec4 color;
in vec2 texPos;
uniform sampler2D tex;

void main ()
{
  color = texture (tex, texPos);
  color.a = color.a * 0.99;
}
)CODE",
R"CODE(
#version 420 core

out vec2 texPos;

void main ()
{
  int i = gl_VertexID;

  if (i == 0) 
    texPos = vec2 (0.0f, 0.0f);
  else if (i == 1)
    texPos = vec2 (1.0f, 0.0f);
  else if (i == 2)
    texPos = vec2 (1.0f, 1.0f);
  else if (i == 3)
    texPos = vec2 (0.0f, 1.0f);

  vec2 vertexPos = 2 * texPos + vec2 (-1.0, -1.0);

  gl_Position = vec4 (vertexPos.x, vertexPos.y, 0., 1.);
}
)CODE");

    glUseProgram (programID);
    
    glGenVertexArrays (1, &VertexArrayID);
    glBindVertexArray (VertexArrayID);
  }

  void render (tex & tt) const
  {
    glViewport (0, 0, tt.width, tt.height);
    glUseProgram (programID);
    glBindVertexArray (VertexArrayID);
    const std::vector<unsigned int> ind = {0, 1, 2, 0, 2, 3};
    glDrawElements (GL_TRIANGLES, 3 * nt, GL_UNSIGNED_INT, &ind[0]);
  }

  void render () const
  {
    glUseProgram (programID);
    glBindVertexArray (VertexArrayID);
    const std::vector<unsigned int> ind = {0, 1, 2, 0, 2, 3};
    glDrawElements (GL_TRIANGLES, 3 * nt, GL_UNSIGNED_INT, &ind[0]);
  }

  ~faderRender ()
  {
    glDeleteVertexArrays (1, &VertexArrayID);
    glDeleteProgram (programID);
  }
};

class sphere
{
  public:
  int Nj;
  int np; 
  std::vector<float> xyz;
  unsigned int nt; 
  std::vector<unsigned int> ind;
  GLuint VertexArrayID;
  GLuint vertexbuffer, elementbuffer;
  GLuint programID;
  float lon = 0., lat = 0., fov = 20.0f;

  sphere (int _Nj)
  {
    Nj = _Nj;

    gensphere (Nj, &np, &xyz, &nt, &ind);

    programID = shader 
(
R"CODE(
#version 420 core

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
#version 420 core

layout(location = 0) in vec3 vertexPos;

out vec4 fragmentColor;

uniform mat4 MVP;
uniform sampler2D tex;
uniform float scale;

void main()
{
  float lon = (atan (vertexPos.y, vertexPos.x) / 3.1415926 + 1.0) * 0.5;
  float lat = asin (vertexPos.z) / 3.1415926 + 0.5;
  gl_Position =  MVP * vec4 (scale * vertexPos, 1);

  vec4 col = texture (tex, vec2 (lon, lat));
  fragmentColor.r = col.r;
  fragmentColor.g = col.g;
  fragmentColor.b = col.b;
  fragmentColor.a = col.a;
}
)CODE");

    glUseProgram (programID);

    glGenVertexArrays (1, &VertexArrayID);
    glBindVertexArray (VertexArrayID);

    glGenBuffers (1, &vertexbuffer);
    glBindBuffer (GL_ARRAY_BUFFER, vertexbuffer);
    glBufferData (GL_ARRAY_BUFFER, 3 * np * sizeof (float), &xyz[0], GL_STATIC_DRAW);
    glEnableVertexAttribArray (0); 
    glVertexAttribPointer (0, 3, GL_FLOAT, GL_FALSE, 0, nullptr); 
    
    glGenBuffers (1, &elementbuffer);
    glBindBuffer (GL_ELEMENT_ARRAY_BUFFER, elementbuffer);
    glBufferData (GL_ELEMENT_ARRAY_BUFFER, 3 * nt * sizeof (unsigned int), &ind[0], GL_STATIC_DRAW);

  }

  void render (const tex & tt, float scale = 1.0) const
  {
    glUseProgram (programID);
    glBindVertexArray (VertexArrayID);

    glm::mat4 Projection = glm::perspective (glm::radians (fov), 1.0f / 1.0f, 0.1f, 100.0f);

    const float deg2rad = M_PI / 180.0f;
    const float R = 6.0;
    float coslon = std::cos (deg2rad * lon), sinlon = std::sin (deg2rad * lon);
    float coslat = std::cos (deg2rad * lat), sinlat = std::sin (deg2rad * lat);
    float X = R * coslon * coslat;
    float Y = R * sinlon * coslat;
    float Z = R *          sinlat;

    glm::mat4 View       = glm::lookAt (glm::vec3 (X,Y,Z), glm::vec3 (0,0,0), glm::vec3 (0,0,1));
    glm::mat4 Model      = glm::mat4 (1.0f);
    glm::mat4 MVP = Projection * View * Model; 
    glUniformMatrix4fv (glGetUniformLocation (programID, "MVP"), 1, GL_FALSE, &MVP[0][0]);
    glUniform1f (glGetUniformLocation (programID, "scale"), scale);

    tt.bind (0);
    glUniform1i (glGetUniformLocation (programID, "tex"), 0);
//  glPolygonMode (GL_FRONT_AND_BACK, GL_LINE);
    glDrawElements (GL_TRIANGLES, 3 * nt, GL_UNSIGNED_INT, nullptr);
  }

  ~sphere ()
  {
    glDeleteBuffers (1, &vertexbuffer);
    glDeleteBuffers (1, &elementbuffer);
    glDeleteVertexArrays (1, &VertexArrayID);
    glDeleteProgram (programID);
  }

  void incLon (float dlon)
  {
    lon += dlon;
  }

  void incLat (float dlat)
  {
    lat += dlat;
  }

  void incFov (float dfov)
  {
    fov += dfov;
  }

};

int main (int argc, char * argv[])
{
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

  const int width = 1024, height = 1024;
  window = glfwCreateWindow (width, height, "", nullptr, nullptr);
    
  if (window == nullptr)
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
  glEnable (GL_MULTISAMPLE);

  tex ttland ("Whole_world_-_land_and_oceans_8000.bmp");
  tex ttuv ("sfc_200_200u.grib2", "sfc_200_200v.grib2");

  tex ttck (1600, 800);
  checkerRender ck (8, 4);

  if (1){
    checkerCompute ck2 (8, 4);
//  ck2.apply (ttck);

//  dotterCompute dd;
//  dd.apply (ttck);


  }else if (1){
    texModifier texm (ttck);

    glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    ck.render (ttck);

  }else if (1){
  while (1) 
    {   
      glViewport (0, 0, ttck.width, ttck.height);

      glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

      ck.render ();

      glfwSwapBuffers (window);
      glfwPollEvents (); 
  
      if (glfwGetKey (window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        break;
      if (glfwWindowShouldClose (window) != 0)  
        break;
    }   
    return 0;
  }


  int Nj = atoi (argv[1]);

  sphere ss (Nj);

  faderCompute ff;
  faderRender ff2;



  GLuint bufferid;
  int nx = 400; 
  int ny = nx / 2 + 1;
  unsigned int buffer_size = 0;

  for (int iy = 1; iy < ny-1; iy++)
  for (int ix = 0; ix < int (nx * std::cos (M_PI/2 * float (iy - ny/2) / float (ny/2))); ix++)
    buffer_size++;

  std::cout << buffer_size << std::endl;

  float lonlat[2 * buffer_size];

  for (int iy = 1, i = 0; iy < ny-1; iy++)
    {
      int nnx = int (nx * std::cos (M_PI/2 * float (iy - ny/2) / float (ny/2))); 
      for (int ix = 0; ix < nnx; ix++, i++)
        {
          lonlat[2*i+0] = -1. + 2. * (float)ix / (float)nnx;
          lonlat[2*i+1] = -1. + 2. * (float)iy / (float)ny;
        }
    }

  glGenBuffers (1, &bufferid);
  glBindBuffer (GL_ARRAY_BUFFER, bufferid);
  glBufferData (GL_ARRAY_BUFFER, 2 * sizeof (float) * buffer_size, lonlat, GL_STATIC_DRAW);
  glBindBuffer (GL_ARRAY_BUFFER, 0);


//const float R = 0.080;
  const float R = 0.004;
  dotterRender dd (R);
  moverCompute mv (R);

  while (1) 
    {   
      if (1)
        {
          ff.apply (ttck, 0.99);
          {
            texModifier texm (ttck);
            dd.render (ttck, bufferid, buffer_size);
          }
          mv.apply (bufferid, buffer_size, ttuv);
        }
      else
        {
          texModifier texm (ttck);
          ttck.bind (0);
          ff2.render (ttck);
        }

      glViewport (0, 0, width, height);
      glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

      ss.render (ttland);
      ss.render (ttck, 1.01);
//    ss.render (ttuv, 1.01);

      glfwSwapBuffers (window);
      glfwPollEvents (); 
  
      if (glfwGetKey (window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        break;
      if (glfwGetKey (window, GLFW_KEY_UP) == GLFW_PRESS)
        ss.incLat (+1);
      if (glfwGetKey (window, GLFW_KEY_DOWN) == GLFW_PRESS)
        ss.incLat (-1);
      if (glfwGetKey (window, GLFW_KEY_LEFT) == GLFW_PRESS)
        ss.incLon (-1);
      if (glfwGetKey (window, GLFW_KEY_RIGHT) == GLFW_PRESS)
        ss.incLon (+1);
      if (glfwGetKey (window, GLFW_KEY_O) == GLFW_PRESS)
        ss.incFov (+1);
      if (glfwGetKey (window, GLFW_KEY_P) == GLFW_PRESS)
        ss.incFov (-1);
      if (glfwGetKey (window, GLFW_KEY_SPACE) == GLFW_PRESS)
        {
          ss.lat = 0.;
          ss.lon = 0.;
        }
      if (glfwWindowShouldClose (window) != 0)  
        break;
    }   



  glfwTerminate ();


  return 0;
}
