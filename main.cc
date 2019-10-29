#include <GL/glew.h>
#include <GLFW/glfw3.h>


#include "shader.h"
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <iostream>
#include <algorithm>
#include <vector>
#include <limits>
#include <bits/stdc++.h> 

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <unistd.h>
#include <sys/time.h>
#include "load.h"
#include "gensphere.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>



class streamline_data_t
{
public:
  std::vector<unsigned int> ind;
  std::vector<float> xyz;
  std::vector<float> drw;
  std::vector<float> dis;
  void push (const glm::vec3 & xyz, const float d = 1.0f)
  {
    push (xyz.x, xyz.y, xyz.z, d);
  }
  void push (const float x, const float y, const float z, const float d = 1.0f)
  {
    float D = 0.0f;
    if (d > 0)
      {
        int sz = size (), last = sz - 1;
        if (sz > 0)
          {
            float x0 = xyz[3*last+0];
            float y0 = xyz[3*last+1];
            float z0 = xyz[3*last+2];
            if (fabsf (x0) + fabsf (y0) + fabsf (z0) > 0.0f)
              {
                float dx = x - x0;
                float dy = y - y0;
                float dz = z - z0;
                D = dis[last] + sqrt (dx * dx + dy * dy + dz * dz);
              }
          }
      }
    xyz.push_back (x);
    xyz.push_back (y);
    xyz.push_back (z);
    drw.push_back (d);
    dis.push_back (D);
  }
  void pop ()
  {
    xyz.pop_back ();
    xyz.pop_back ();
    xyz.pop_back ();
    drw.pop_back ();
    dis.pop_back ();
  }
  void clear ()
  {
    xyz.clear ();
    ind.clear ();
    drw.clear ();
    dis.clear ();
  }
  int size ()
  {
    return xyz.size () / 3;
  }
};

typedef struct
{
  GLuint VertexArrayID;
  GLuint vertexbuffer, elementbuffer, normalbuffer, distbuffer, valbuffer;
  GLuint size;    // Number of indices
  GLuint size_inst;
} streamline_t;

void triNeigh (const geom_t & geom, int it, int jglo[3], int itri[3])
{
  int jgloA = geom.ind[3*it+0]; 
  int jgloB = geom.ind[3*it+1]; 
  int jgloC = geom.ind[3*it+2]; 
  
  jlonlat_t jlonlatA = geom.jlonlat (jgloA);
  jlonlat_t jlonlatB = geom.jlonlat (jgloB);
  jlonlat_t jlonlatC = geom.jlonlat (jgloC);
  
  int jglo0, jglo1, jglo2;
  jlonlat_t jlonlat0, jlonlat1, jlonlat2;
  
  if (jlonlatA.jlat == jlonlatB.jlat)
    {
      jglo0    = jgloA;    jglo1    = jgloB;    jglo2    = jgloC;    
      jlonlat0 = jlonlatA; jlonlat1 = jlonlatB; jlonlat2 = jlonlatC;
    }
  else 
  if (jlonlatA.jlat == jlonlatC.jlat)
    {
      jglo0    = jgloA;    jglo1    = jgloC;    jglo2    = jgloB;    
      jlonlat0 = jlonlatA; jlonlat1 = jlonlatC; jlonlat2 = jlonlatB;
    }
  else 
  if (jlonlatB.jlat == jlonlatC.jlat)
    {
      jglo0    = jgloB;    jglo1    = jgloC;    jglo2    = jgloA;    
      jlonlat0 = jlonlatB; jlonlat1 = jlonlatC; jlonlat2 = jlonlatA;
    }
  
  bool up = jlonlat2.jlat < jlonlat0.jlat;
  
  int ntri = geom.pl[jlonlat0.jlat-1] + geom.pl[jlonlat2.jlat-1];                    // Number of triangles on this row
  int lat1 = up ? jlonlat2.jlat : jlonlat0.jlat;
  int otri = lat1 == 1 ? 0 : geom.jglooff[lat1] * 2 - geom.pl[lat1-1] - geom.pl[0];  // Offset of triangles counting
  int ktri = it - otri;                                                              // Rank of current triangle in current row
  int Ltri = ktri == 0      ? ntri-1 : ktri-1; Ltri += otri;                         // Rank of left triangle
  int Rtri = ktri == ntri-1 ?      0 : ktri+1; Rtri += otri;                         // Rank of right triangle
  int Vtri = up ? geom.trid[jglo0] : geom.triu[jglo0];                               // Rank of triangle above of under segment 01

  int itr01 = Vtri, itr12 = Rtri, itr20 = Ltri;
  
  jglo[0] = jglo0; jglo[1] = jglo1; jglo[2] = jglo2;
  itri[0] = itr01; itri[1] = itr12; itri[2] = itr20;

}



static glm::vec2 xyz2merc (const glm::vec3 & xyz)
{
  float lon = atan2 (xyz.y, xyz.x);
  float lat = asin (xyz.z);
  return glm::vec2 (lon, log (tan (M_PI / 4.0f + lat / 2.0f)));
}

static glm::vec3 merc2xyz (const glm::vec2 & merc)
{
  float lon = merc.x;
  float lat = 2.0f * atan (exp (merc.y)) - M_PI / 2.0f;
  float coslon = cos (lon), sinlon = sin (lon);
  float coslat = cos (lat), sinlat = sin (lat);
  return glm::vec3 (coslon * coslat, sinlon * coslat, sinlat);
}

static glm::vec2 merc2lonlat (const glm::vec2 & merc)
{
  float lon = merc.x;
  float lat = 2.0f * atan (exp (merc.y)) - M_PI / 2.0f;
  return glm::vec2 (glm::degrees (lon), glm::degrees (lat));
}

float lineLineIntersect (const glm::vec2 & P1, const glm::vec2 & V1,
                         const glm::vec2 & P,  const glm::vec2 & Q)
{
  float b = V1.x * (P.y - Q.y ) - V1.y * (P.x - Q.x ); 
  float a = V1.y * (Q.x - P1.x) - V1.x * (Q.y - P1.y);

  if (fabsf (b) < 1e-6)
    return std::numeric_limits<float>::infinity ();

  return a / b;
}



void process (int it0, const float * ru, const float * rv, bool * seen, 
              const geom_t & geom, const float * xyz, streamline_data_t * stream)
{
  std::vector<glm::vec3> listf, listb, * list = &listf;

  int it = it0; 
  std::valarray<float> w0 (3), w (3);
  glm::vec2 M, V;   // Current values
  glm::vec2 M0, V0; // Previous values
  float V0MM00;

  int count = 0;
  
  while (1)
    {
 

      if (seen[it])
        goto last;

      seen[it] = true;

      //
      {
        int jglo[3], itri[3];
        triNeigh (geom, it, jglo, itri);
        glm::vec2 Mn;
        glm::vec2 P[3];
   
        // Mercator projection
        for (int i = 0; i < 3; i++)
          P[i] = xyz2merc (glm::vec3 (xyz[3*jglo[i]+0], xyz[3*jglo[i]+1], xyz[3*jglo[i]+2]));
   
        // First point of the list; see which segment to start with, initialize weights, V and M
        if (listf.size () == 0)
          {
            int i0;
            float s0 = std::numeric_limits<float>::max ();
            for (int i = 0; i < 3; i++)
              {
                int j = (i + 1) % 3;
                int k = (i + 2) % 3;
                w[i] = w[j] = 0.5f; w[k] = 0.0f;
                V = glm::vec2 (w[0] * ru[jglo[0]] + w[1] * ru[jglo[1]] + w[2] * ru[jglo[2]],
                               w[0] * rv[jglo[0]] + w[1] * rv[jglo[1]] + w[2] * rv[jglo[2]]) / w.sum ();
                V = glm::normalize (V);
                glm::vec2 U = glm::normalize (P[j] - P[i]);
                float s = fabsf (glm::dot (U, V));
                if (s < s0)
                  {
                    s0 = s;
                    i0 = i;
                  }
              }
            
            int j0 = (i0 + 1) % 3;
            int k0 = (i0 + 2) % 3;
     
            w[i0] = w[j0] = 0.5f; w[k0] = 0.0f;
   
            M = (w[0] * P[0] + w[1] * P[1] + w[2] * P[2]) / w.sum ();
   
            w0 = w;

            V = glm::vec2 (w[0] * ru[jglo[0]] + w[1] * ru[jglo[1]] + w[2] * ru[jglo[2]],
                           w[0] * rv[jglo[0]] + w[1] * rv[jglo[1]] + w[2] * rv[jglo[2]]) / w.sum ();
   
            list->push_back (glm::vec3 (M.x, M.y, glm::length (V)));

          }


        if (list->size () >= 2)
          {
            float V0MM0 = glm::dot (V0, M-M0);
            if ((list == &listf) && (list->size () == 2))
              {
                V0MM00 = V0MM0;
              }
            else if (list == &listf)
              {
                if (V0MM00 * V0MM0 < 0.0f)
                  goto last;
              }
            else if (list == &listb)
              {
                if (V0MM00 * V0MM0 > 0.0f)
                  goto last;
              }
          }
   
        // Fix periodicity issue
        for (int i = 0; i < 3; i++)
          {
            if (M.x - P[i].x > M_PI)
              P[i].x += 2.0f * M_PI;
            else if (P[i].x - M.x > M_PI)
              P[i].x -= 2.0f * M_PI;
          }

        M0 = M; V0 = V;

        // Try all edges : intersection of vector line with triangle edges
        for (int i = 0; i < 3; i++)
          {
            int j = (i + 1) % 3;
            int k = (i + 2) % 3;

            if ((w[i] != 0.0f) && (w[j] != 0.0f)) // Exit triangle through other edge
              continue;

            float lambda = lineLineIntersect (M, V, P[i], P[j]);

            if ((lambda < 0.0f) || (1.0f < lambda)) // Cross edge line between P[i] and P[j]
              continue;

            M = P[i] * lambda + P[j] * (1.0f - lambda);

            V = glm::vec2 (lambda * ru[jglo[i]] + (1.0f - lambda) * ru[jglo[j]],
                           lambda * rv[jglo[i]] + (1.0f - lambda) * rv[jglo[j]]);

            w[0] = w[1] = w[2] = 0.0f;
 
            list->push_back (glm::vec3 (M.x, M.y, glm::length (V)));

            int itn = itri[i], jglon[3], itrin[3];
            
            if (itn < 0)
              {
                list->pop_back ();
                continue;
              }

            if (seen[itn])
              {
                list->pop_back ();
                continue;
              }
            
            triNeigh (geom, itn, jglon, itrin);

            // Find weights for next triangle
            for (int k = 0; k < 3; k++)
              {
                if (jglon[k] == jglo[i])
                  w[k] = lambda;
                if (jglon[k] == jglo[j])
                  w[k] = 1.0f - lambda;
              }

            it = itn;

            goto found;
          }

       }

      goto last;

found:
  
      count++;

      continue;

last:

      if ((list == &listf) && (listf.size () > 0))
        {
          M = listf[0]; w = w0;
          list = &listb;
          int jglo[3], itri[3];
          triNeigh (geom, it0, jglo, itri);

          for (int i = 0; i < 3; i++)
            {
              int j = (i + 1) % 3;
              if ((w[i] > 0.0f) && (w[j] > 0.0f))
                {
                  it = itri[i];
                  break;
                }
            }

          if (it == -1)
            break;

          continue;
        }

      break;

    }

  if (V0MM00 > 0.0f)
    {
      for (int i = listb.size () - 1; i >= 0; i--)
        stream->push (merc2xyz (glm::vec2 (listb[i].x, listb[i].y)), listb[i].z);
      for (int i = 0; i < listf.size (); i++)
        stream->push (merc2xyz (glm::vec2 (listf[i].x, listf[i].y)), listf[i].z);
    }
  else
    {
      for (int i = listf.size () - 1; i >= 0; i--)
        stream->push (merc2xyz (glm::vec2 (listf[i].x, listf[i].y)), listf[i].z);
      for (int i = 0; i < listb.size (); i++)
        stream->push (merc2xyz (glm::vec2 (listb[i].x, listb[i].y)), listb[i].z);
    }

  if (stream->dis[stream->size ()-1] < 0.1000f) // About 5 degrees
    {
      for (int i = 0; i < listf.size () + listb.size (); i++)
        stream->pop ();
    }
  else
    {
      if (listf.size () + listb.size () > 0)
        stream->push (0.0f, 0.0f, 0.0f, 0.0f);
    }
  

}

static bool verbose = false;
static float lonc = 0.0f;
static float latc = 0.0f;
static float fov = 20.0f;
static float R = 6.0f;
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

  float * xyz;
  int np; 
  unsigned int nt;

  geom_t geom;

  const int width = 1024, height = 1024;
  int w, h;
  unsigned char * rx = NULL, * ry = NULL;
  float * Fx = NULL, * Fy = NULL;

  if (endsWith (argv[1], std::string (".grb")))
    {
      gensphere_grib (&geom, &np, &xyz, &nt, &Fx, &Fy, argv[1], argv[2]);
    }
  else
    {
      geom.Nj = atoi (argv[1]);
      std::string type = "gradx";
      if (argc > 2)
        type = argv[2];
      gensphere (&geom, &np, &xyz, &nt, &Fx, &Fy, type);
    }


  int size = 0;
  for (int jlat = 1; jlat <= geom.Nj-1; jlat++)
    size += geom.pl[jlat-1];

  std::cout << " size = " << size << std::endl;

  float maxval = *std::max_element (Fx, Fx + size);
  float minval = *std::min_element (Fx, Fx + size);

  float normmax = 0.0f;
  for (int i = 0; i < size; i++) 
    normmax = std::max (normmax, Fx[i] * Fx[i] + Fy[i] * Fy[i]);

  normmax = sqrt (normmax);

  std::cout << " normmax = " << normmax << std::endl;

  for (int i = 0; i < size; i++)
    {
       Fx[i] = Fx[i] / normmax;
       Fy[i] = Fy[i] / normmax;
    }

  streamline_data_t stream_data;

  std::cout << " np = " << np << " nt = " << nt << std::endl;

  {
    bool * seen = (bool *)malloc (sizeof (bool) * nt);
    for (int i = 0; i < nt; i++)
      seen[i] = false;

    
if(0)
{
    int dit = std::max (1, (int)(nt / 8000));
    for (int it = 0; it < nt; it += dit)
      process (it, Fx, Fy, seen, geom, xyz, &stream_data);
}
else
{

    int dit = std::max (1, (int)sqrt (nt / 40000.0f));
std::cout << " dit = " << dit << std::endl;
    int itoff = 0;
    for (int jlat = 1; jlat <= geom.Nj-1; jlat++)
      {
        if ((jlat - 1) % dit == 0)
          {
            for (int i = 0; i < 2 * geom.pl[jlat-1]; i += dit)
              {
                int it = itoff + i;
                if (it >= nt)
                  abort ();
                process (it, Fx, Fy, seen, geom, xyz, &stream_data);
              }
          }
        itoff += 2 * geom.pl[jlat-1];
      }
}

    free (seen);
  }
  
  
  std::cout << " size = " << stream_data.size () << std::endl;

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
  glCullFace (GL_BACK);
  glFrontFace (GL_CCW);
  glEnable (GL_CULL_FACE);
  glDepthFunc (GL_LESS); 



  // Sphere
  GLuint VertexArrayID;
  GLuint vertexbuffer, colorbufferx, colorbuffery, elementbuffer;

  glGenVertexArrays (1, &VertexArrayID);
  glBindVertexArray (VertexArrayID);

  glGenBuffers (1, &vertexbuffer);
  glBindBuffer (GL_ARRAY_BUFFER, vertexbuffer);
  glBufferData (GL_ARRAY_BUFFER, 3 * np * sizeof (float), xyz, GL_STATIC_DRAW);
  glEnableVertexAttribArray (0); 
  glVertexAttribPointer (0, 3, GL_FLOAT, GL_FALSE, 0, NULL); 

  const int ncol = 1;
  glGenBuffers (1, &colorbufferx);
  glBindBuffer (GL_ARRAY_BUFFER, colorbufferx);
  glBufferData (GL_ARRAY_BUFFER, ncol * np * sizeof (unsigned char), rx, GL_STATIC_DRAW);
  glEnableVertexAttribArray (1); 
  glVertexAttribPointer (1, ncol, GL_UNSIGNED_BYTE, GL_TRUE, ncol * sizeof (unsigned char), NULL); 

  glGenBuffers (1, &colorbuffery);
  glBindBuffer (GL_ARRAY_BUFFER, colorbuffery);
  glBufferData (GL_ARRAY_BUFFER, ncol * np * sizeof (unsigned char), ry, GL_STATIC_DRAW);
  glEnableVertexAttribArray (2); 
  glVertexAttribPointer (2, ncol, GL_UNSIGNED_BYTE, GL_TRUE, ncol * sizeof (unsigned char), NULL); 

  
  glGenBuffers (1, &elementbuffer);
  glBindBuffer (GL_ELEMENT_ARRAY_BUFFER, elementbuffer);
  glBufferData (GL_ELEMENT_ARRAY_BUFFER, 3 * nt * sizeof (unsigned int), geom.ind , GL_STATIC_DRAW);


  streamline_t stream;

  stream.size = stream_data.ind.size ();

  stream.size_inst = stream_data.size () - 1;

  glGenVertexArrays (1, &stream.VertexArrayID);
  glBindVertexArray (stream.VertexArrayID);
  
  glGenBuffers (1, &stream.vertexbuffer);
  glBindBuffer (GL_ARRAY_BUFFER, stream.vertexbuffer);
  glBufferData (GL_ARRAY_BUFFER, stream_data.xyz.size () * sizeof (float), 
                stream_data.xyz.data (), GL_STATIC_DRAW);

  glEnableVertexAttribArray (0); 
  glVertexAttribPointer (0, 3, GL_FLOAT, GL_FALSE, 0, NULL); 
  glVertexAttribDivisor (0, 1);

  glEnableVertexAttribArray (1); 
  glVertexAttribPointer (1, 3, GL_FLOAT, GL_FALSE, 0, (const void *)(3 * sizeof (float))); 
  glVertexAttribDivisor (1, 1);

  glEnableVertexAttribArray (2); 
  glVertexAttribPointer (2, 3, GL_FLOAT, GL_FALSE, 0, (const void *)(6 * sizeof (float))); 
  glVertexAttribDivisor (2, 1);


  glGenBuffers (1, &stream.normalbuffer);
  glBindBuffer (GL_ARRAY_BUFFER, stream.normalbuffer);
  glBufferData (GL_ARRAY_BUFFER, stream_data.drw.size () * sizeof (float), 
                stream_data.drw.data (), GL_STATIC_DRAW);

  glEnableVertexAttribArray (3); 
  glVertexAttribPointer (3, 1, GL_FLOAT, GL_FALSE, 0, NULL); 
  glVertexAttribDivisor (3, 1);

  glEnableVertexAttribArray (4); 
  glVertexAttribPointer (4, 1, GL_FLOAT, GL_FALSE, 0, (const void *)(sizeof (float))); 
  glVertexAttribDivisor (4, 1);

  glGenBuffers (1, &stream.distbuffer);
  glBindBuffer (GL_ARRAY_BUFFER, stream.distbuffer);
  glBufferData (GL_ARRAY_BUFFER, stream_data.dis.size () * sizeof (float), 
                stream_data.dis.data (), GL_STATIC_DRAW);

  glEnableVertexAttribArray (5); 
  glVertexAttribPointer (5, 1, GL_FLOAT, GL_FALSE, 0, NULL); 
  glVertexAttribDivisor (5, 1);

  glEnableVertexAttribArray (6); 
  glVertexAttribPointer (6, 1, GL_FLOAT, GL_FALSE, 0, (const void *)(sizeof (float))); 
  glVertexAttribDivisor (6, 1);


  stream_data.clear ();

  GLuint programID = shader 
(
R"CODE(
#version 330 core

out vec4 color;

void main()
{
  color.r = 0.;
  color.g = 0.;
  color.b = 0.;
  color.a = 1.;
//color.r = fragmentColor.r;
//color.g = fragmentColor.g;
//color.b = fragmentColor.b;
//color.a = fragmentColor.a;
}
)CODE",
R"CODE(
#version 330 core

layout(location = 0) in vec3 vertexPos;
layout(location = 1) in float colx;
layout(location = 2) in float coly;

out vec4 fragmentColor;

uniform mat4 MVP;

void main()
{
  vec3 pos = 0.99 * vertexPos;
  gl_Position = MVP * vec4 (pos, 1);
}
)CODE");

  GLuint programID_l_inst = shader 
(
R"CODE(
#version 330 core

out vec4 color;
in vec3 col;
in float instid;
in float norm;
in float dist;

void main()
{
  vec3 grey = vec3 (0.3f, 0.3f, 0.3f);
  vec3 green= vec3 (0.0f, 1.0f, 0.0f);
  if (norm == 0.)
    discard;
  color.rgb = norm * green + (1.0f - norm) * grey;
if(false){
  if (mod (instid, 2) < 1)
    {
      color.r = 1.0;
      color.g = 0.0;
      color.b = 0.0;
    }
  else
    {
      color.r = 0.3;
      color.g = 0.3;
      color.b = 1.0;
    }
}
  color.a = 1.;
}
)CODE",
R"CODE(
#version 330 core

layout(location = 0) in vec3 vertexPos0;
layout(location = 1) in vec3 vertexPos1;
layout(location = 2) in vec3 vertexPos2;
layout(location = 3) in float norm0;
layout(location = 4) in float norm1;
layout(location = 5) in float dist0;
layout(location = 6) in float dist1;

out vec3 col;
out float instid;
out float norm;
out float dist;


uniform mat4 MVP;

void main()
{
  vec3 vertexPos;
  vec3 t0, t1;

  t0 = normalize (vertexPos1 - vertexPos0);
  t1 = normalize (vertexPos2 - vertexPos1);

  if ((gl_VertexID == 0) || (gl_VertexID == 2))
    vertexPos = vertexPos0;
  else if ((gl_VertexID == 1) || (gl_VertexID == 3) || (gl_VertexID >= 4))
    vertexPos = vertexPos1;  

  vec3 p = normalize (vertexPos);
  vec3 n0 = cross (t0, p);
  vec3 n1 = cross (t1, p);

  float c = max (0.002, 0.015 * norm0);

  if ((gl_VertexID >= 4) && (dot (cross (n0, n1), vertexPos) < 0.))
    c = 0.0;

  if (gl_VertexID == 2)
    vertexPos = vertexPos + c * n0;
  if (gl_VertexID == 3)
    vertexPos = vertexPos + c * n0;
  if (gl_VertexID == 4)
    vertexPos = vertexPos + c * normalize (n0 + n1);
  if (gl_VertexID == 5)
    vertexPos = vertexPos + c * n1;


  gl_Position =  MVP * vec4 (vertexPos, 1);

  if (gl_VertexID == 4)
    {
      col.r = 1.;
      col.g = 0.;
      col.b = 0.;
    }
  else
    {
      col.r = 0.;
      col.g = 1.;
      col.b = 0.;
    }


  instid = gl_InstanceID;
  norm = min (norm0, norm1);

  if ((gl_VertexID == 0) && (gl_VertexID == 2))
    {
      dist = dist0;
    }
  else
    {
      dist = dist1;
    }



}
)CODE");


  if(0){
  GLfloat lineWidthRange[2] = {0.0f, 0.0f};
  glGetFloatv(GL_ALIASED_LINE_WIDTH_RANGE, lineWidthRange);
  std::cout << lineWidthRange[0] << " " << lineWidthRange[1] << std::endl;
  }


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

      if(1){
      // Sphere
      glUseProgram (programID);
      glUniformMatrix4fv (glGetUniformLocation (programID, "MVP"), 1, GL_FALSE, &MVP[0][0]);
      glBindVertexArray (VertexArrayID);
      if (wireframe)
        glPolygonMode (GL_FRONT_AND_BACK, GL_LINE);
      else
        glPolygonMode (GL_FRONT_AND_BACK, GL_FILL);
      glDrawElements (GL_TRIANGLES, 3 * nt, GL_UNSIGNED_INT, NULL);
      glPolygonMode (GL_FRONT_AND_BACK, GL_FILL);
      }

      // Line 
      //
      glUseProgram (programID_l_inst);
      glUniformMatrix4fv (glGetUniformLocation (programID_l_inst, "MVP"), 
			  1, GL_FALSE, &MVP[0][0]);

   

      bool wide = true;

      glBindVertexArray (stream.VertexArrayID);
      if (! wide)
        {
          glDrawArraysInstanced (GL_LINE_STRIP, 0, 2, stream.size_inst);
        }
      else
        {
          unsigned int ind[12] = {1, 0, 2, 3, 1, 2, 1, 3, 4, 1, 4, 5};
          glDrawElementsInstanced (GL_TRIANGLES, 12, GL_UNSIGNED_INT, ind, stream.size_inst);
        }
      glBindVertexArray (0);


      glfwSwapBuffers (window);
      glfwPollEvents (); 
  
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
