#include <GL/glew.h>
#include <GLFW/glfw3.h>


#include "bmp.h"
#include "gensphere.h"
#include "shader.h"
#include <stdio.h>
#include <stdlib.h>
#include <iostream>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <unistd.h>
#include <sys/time.h>


using namespace glm;

const float PI = 3.141592653589793;

float msfnz (float eccent, float sinphi, float cosphi) {
  float con = eccent * sinphi;
  return cosphi / (sqrt(1 - con * con));
}

const float rad2deg = 180. / PI;
const float deg2rad = PI / 180.;

struct lcc_t {
  float lon0;
  float lat0; 
  float lat1; 
  float lat2;
  float x0; 
  float y0; 
  float z0;
  float a; 
  float b; 
  float e;
  float k0; 
  float ns; 
  float f0; 
  float rh;
};

float phi2z (float eccent, float ts) {
  float eccnth = 0.5 * eccent;
  float con, dphi;
  float phi = PI*0.5-2.0*atan(ts);
  for (int i = 0; i <= 15; i++) {
    con = eccent * sin(phi);
    dphi = PI*0.5-2.0*atan(ts*pow((1.0-con)/(1.0+con),eccnth))-phi;
    phi += dphi;
    if (abs(dphi) <= 0.0000000001) return phi;
  }
  return -9999.0;
}

float tsfnz (float eccent, float phi, float sinphi) {
  float con = eccent * sinphi;
  float com = 0.5 * eccent;
  con = pow(((1.0-con)/(1.0+con)),com);
  return tan(0.5*(PI*0.5-phi))/con;
}

vec3 lcc_inverse (lcc_t t, vec3 p) {
  float rh1, con;
  float x = (p.x - t.x0) / t.k0;
  float y = t.rh - (p.y - t.y0) / t.k0;
  if (t.ns > 0.0) {
    rh1 = sqrt(x*x+y*y);
    con = 1.0;
  } else {
    rh1 = -sqrt(x*x+y*y);
    con = -1.0;
  }
  float theta = abs(rh1) > 1e-10
    ? atan(con*x, con*y)
    : 0.0
  ;
  float ts;
  if (abs(rh1) > 1e-10 || t.ns > 0.0) {
    con = 1.0 / t.ns;
    ts = pow(rh1 / (t.a*t.f0), con);
    return vec3(
      (theta/t.ns + t.lon0)*180.0/PI,
      phi2z(t.e, ts)*180.0/PI,
      (p.z-t.z0)/t.k0
    );
  } else {
    return vec3(
      (theta/t.ns + t.lon0)*180.0/PI,
      -90.0,(p.z-t.z0)/t.k0);
  }
}

vec3 lcc_inverse (lcc_t t, vec2 p) {
  return lcc_inverse(t,vec3(p,0));
}

vec3 lcc_forward (lcc_t t, vec3 p) {
  float lon = p.x/180.0*PI, lat = p.y/180.0*PI;
  if (abs(2.0*abs(lat)-PI) <= 1e-10) {
    lat = sign(lat) * (PI*0.5 - 2e-10);
  }
  float con = abs(abs(lat) - PI*0.5);
  float ts, rh1;
  if (con > 1e-10) {
    rh1 = t.a * t.f0 * pow(tsfnz(t.e, lat, sin(lat)), t.ns);
  } else {
    con = lat * t.ns;
    rh1 = 0.0;
  }
  float theta = t.ns * (lon - t.lon0);
  return vec3(
    t.x0+t.k0*rh1*sin(theta),
    t.y0+t.k0*(t.rh-rh1*cos(theta)),
    t.z0+t.k0*p.z
  );
}
vec3 lcc_forward (lcc_t t, vec2 p) {
  return lcc_forward(t,vec3(p.x, p.y,0));
}

int lamb = 1;
static
void key_callback (GLFWwindow * window, int key, int scancode, int action, int mods)
{
  if ((key == GLFW_KEY_SPACE) && (action == GLFW_PRESS))
    lamb = ! lamb;
}


int main (int argc, char * argv[])
{
  int Nj = atoi (argv[1]);
  int np; 
  float * xyz;
  unsigned int nt;
  unsigned int * ind;
  const int width = 1024, height = 1024;
  int w0, h0;
  unsigned char * rgb0 = NULL;
  int w1, h1;
  unsigned char * rgb1 = NULL;

  bmp ("Whole_world_-_land_and_oceans_8000.bmp", &rgb0, &w0, &h0);
  bmp ("SC1000_0040_6950_L93_E100.bmp", &rgb1, &w1, &h1);

  gensphere1 (Nj, &np, &xyz, &nt, &ind);

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
  glfwSetKeyCallback (window, key_callback);
  
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
  GLuint vertexbuffer, colorbuffer, elementbuffer;

  glGenVertexArrays (1, &VertexArrayID);
  glBindVertexArray (VertexArrayID);

  glGenBuffers (1, &vertexbuffer);
  glBindBuffer (GL_ARRAY_BUFFER, vertexbuffer);
  glBufferData (GL_ARRAY_BUFFER, 3 * np * sizeof (float), xyz, GL_STATIC_DRAW);
  glEnableVertexAttribArray (0); 
  glVertexAttribPointer (0, 3, GL_FLOAT, GL_FALSE, 0, NULL); 
  
  glGenBuffers (1, &elementbuffer);
  glBindBuffer (GL_ELEMENT_ARRAY_BUFFER, elementbuffer);
  glBufferData (GL_ELEMENT_ARRAY_BUFFER, 3 * nt * sizeof (unsigned int), ind , GL_STATIC_DRAW);

  GLuint programID = shader 
(
R"CODE(
#version 330 core

in vec3 fragmentPos;
out vec4 color;
uniform sampler2D texture0;
uniform sampler2D texture1;


const float PI = 3.141592653589793;
const float rad2deg = 180. / PI;

struct lcc_t {
  float lon0;
  float lat0; 
  float lat1; 
  float lat2;
  float x0; 
  float y0; 
  float z0;
  float a; 
  float b; 
  float e;
  float k0; 
  float ns; 
  float f0; 
  float rh;
};

float phi2z (float eccent, float ts) {
  float eccnth = 0.5 * eccent;
  float con, dphi;
  float phi = PI*0.5-2.0*atan(ts);
  for (int i = 0; i <= 15; i++) {
    con = eccent * sin(phi);
    dphi = PI*0.5-2.0*atan(ts*pow((1.0-con)/(1.0+con),eccnth))-phi;
    phi += dphi;
    if (abs(dphi) <= 0.0000000001) return phi;
  }
  return -9999.0;
}

float tsfnz (float eccent, float phi, float sinphi) {
  float con = eccent * sinphi;
  float com = 0.5 * eccent;
  con = pow(((1.0-con)/(1.0+con)),com);
  return tan(0.5*(PI*0.5-phi))/con;
}

vec3 lcc_inverse (lcc_t t, vec3 p) {
  float rh1, con;
  float x = (p.x - t.x0) / t.k0;
  float y = t.rh - (p.y - t.y0) / t.k0;
  if (t.ns > 0.0) {
    rh1 = sqrt(x*x+y*y);
    con = 1.0;
  } else {
    rh1 = -sqrt(x*x+y*y);
    con = -1.0;
  }
  float theta = abs(rh1) > 1e-10
    ? atan(con*x, con*y)
    : 0.0
  ;
  float ts;
  if (abs(rh1) > 1e-10 || t.ns > 0.0) {
    con = 1.0 / t.ns;
    ts = pow(rh1 / (t.a*t.f0), con);
    return vec3(
      (theta/t.ns + t.lon0)*180.0/PI,
      phi2z(t.e, ts)*180.0/PI,
      (p.z-t.z0)/t.k0
    );
  } else {
    return vec3(
      (theta/t.ns + t.lon0)*180.0/PI,
      -90.0,(p.z-t.z0)/t.k0);
  }
}

vec3 lcc_inverse (lcc_t t, vec2 p) {
  return lcc_inverse(t,vec3(p,0));
}

vec3 lcc_forward (lcc_t t, vec3 p) {
  float lon = p.x/180.0*PI, lat = p.y/180.0*PI;
  if (abs(2.0*abs(lat)-PI) <= 1e-10) {
    lat = sign(lat) * (PI*0.5 - 2e-10);
  }
  float con = abs(abs(lat) - PI*0.5);
  float ts, rh1;
  if (con > 1e-10) {
    rh1 = t.a * t.f0 * pow(tsfnz(t.e, lat, sin(lat)), t.ns);
  } else {
    con = lat * t.ns;
    rh1 = 0.0;
  }
  float theta = t.ns * (lon - t.lon0);
  return vec3(
    t.x0+t.k0*rh1*sin(theta),
    t.y0+t.k0*(t.rh-rh1*cos(theta)),
    t.z0+t.k0*p.z
  );
}
vec3 lcc_forward (lcc_t t, vec2 p) {
  return lcc_forward(t,vec3(p.x, p.y,0));
}

uniform float lat1; 
uniform float lat2; 
uniform float lat0;
uniform float a; 
uniform float b;
uniform float e;
uniform float ms1;
uniform float ms2;
uniform float ts0;
uniform float ts1;
uniform float ts2;
uniform float ns;
uniform float f0;
uniform float lon0;
uniform float x0;
uniform float y0;
uniform float z0;
uniform float k0;
uniform float rh;

uniform bool lamb = true;

void main ()
{
  lcc_t t;
  t.lon0 = lon0;
  t.lat0 = lat0; 
  t.lat1 = lat1; 
  t.lat2 = lat2;
  t.x0   = x0  ; 
  t.y0   = y0  ; 
  t.z0   = z0  ;
  t.a    = a   ; 
  t.b    = b   ; 
  t.e    = e   ;
  t.k0   = k0  ; 
  t.ns   = ns  ; 
  t.f0   = f0  ; 
  t.rh   = rh  ;

  float lon = atan (fragmentPos.y, fragmentPos.x);
  float lat = asin (fragmentPos.z);
  float lonmin = -PI;
  float lonmax = +PI;
  float latmin = -PI/2;
  float latmax = +PI/2;

  float x, xmin, xmax;
  float y, ymin, ymax;

  x = lon; xmin = lonmin; xmax = lonmax;
  y = lat; ymin = latmin; ymax = latmax;

  float xt0 = (x - xmin) / (xmax - xmin);
  float yt0 = (y - ymin) / (ymax - ymin);

  vec4 col0 = texture2D (texture0, vec2 (xt0, yt0));

  vec3 posl = lcc_forward (t, vec2 (rad2deg * lon, rad2deg * lat));
  float lccx = posl.x;
  float lccy = posl.y;

  float lccxmin =   40000;
  float lccxmax =  240000;
  float lccymin = 6750000;
  float lccymax = 6950000;

  x = lccx; xmin = lccxmin; xmax = lccxmax;
  y = lccy; ymin = lccymin; ymax = lccymax;

  float xl1 = (x - xmin) / (xmax - xmin);
  float yl1 = (y - ymin) / (ymax - ymin);

  bool inl = (0 < xl1) && (xl1 < 1) && (0 < yl1) && (yl1 < 1);

  if (inl && lamb){
  vec4 col1 = texture2D (texture1, vec2 (xl1, yl1));
  color.r = col1.r;
  color.g = col1.g;
  color.b = col1.b;
  color.a = 1.;
  }else{
  color.r = col0.r;
  color.g = col0.g;
  color.b = col0.b;
  color.a = 1.;
  }
}
)CODE",
R"CODE(

#version 330 core

layout (location = 0) in vec3 vertexPos;

out vec3 fragmentPos;

uniform mat4 MVP;

const float pi = 3.1415926;

void main()
{
  vec3 normedPos;

  float x = vertexPos.x;
  float y = vertexPos.y;
  float z = vertexPos.z;
  float r = 1. / sqrt (x * x + y * y + z * z); 
  normedPos.x = x * r;
  normedPos.y = y * r;
  normedPos.z = z * r;

  gl_Position =  MVP * vec4 (normedPos, 1);

  fragmentPos = normedPos;

}


)CODE");

  glUseProgram (programID);


  float lonc = -3.34247;
  float latc = 48.2865;
  float fov = 1;
  float coslonc = cos (deg2rad * lonc), sinlonc = sin (deg2rad * lonc);
  float coslatc = cos (deg2rad * latc), sinlatc = sin (deg2rad * latc);

  float r = 6;
  float xc = r * coslonc * coslatc;
  float yc = r * sinlonc * coslatc;
  float zc = r *           sinlatc;

  glm::mat4 Projection = glm::perspective (glm::radians (fov), 1.0f, 0.1f, 100.0f);
  glm::mat4 View       = glm::lookAt (glm::vec3 (xc, yc, zc),
		                      glm::vec3 (0,0,0), 
                                      glm::vec3 (0,0,1));
  glm::mat4 Model      = glm::mat4 (1.0f);

  glm::mat4 MVP = Projection * View * Model; 

  glUniformMatrix4fv (glGetUniformLocation (programID, "MVP"), 1, GL_FALSE, &MVP[0][0]);

  const float lat1deg = 44, lat2deg = 49, lat0deg = 46.5;
  const float lon0deg = 3;
  const float lat1 = lat1deg * deg2rad, 
              lat2 = lat2deg * deg2rad, 
	      lat0 = lat0deg * deg2rad;
  const float lon0 = lon0deg * deg2rad;
  const float a = 6371229.0, b = a;
  const float e = 0.0;
  const float ms1 = msfnz (e, sin (lat1), cos (lat1));
  const float ms2 = msfnz (e, sin (lat2), cos (lat2));
  const float ts0 = tsfnz (e, lat0, sin (lat0));
  const float ts1 = tsfnz (e, lat1, sin (lat1));
  const float ts2 = tsfnz (e, lat2, sin (lat2));
  float ns = abs(lat1 - lat2) > 1e-10 ? log(ms1/ms2)/log(ts1/ts2) : sin(lat1);
  if (isnan(ns)) ns = sin (lat1);
  const float f0 = ms1 / (ns * pow(ts1, ns));
  const float x0 = 700000, y0 = 6600000, z0 = 0;
  const float k0 = 1.;
  const float rh = a * f0 * pow (ts0, ns);

  glUniform1f (glGetUniformLocation (programID, "lat1"), lat1);
  glUniform1f (glGetUniformLocation (programID, "lat2"), lat2);
  glUniform1f (glGetUniformLocation (programID, "lat0"), lat0);
  glUniform1f (glGetUniformLocation (programID, "a"   ), a   );
  glUniform1f (glGetUniformLocation (programID, "b"   ), b   );
  glUniform1f (glGetUniformLocation (programID, "e"   ), e   );
  glUniform1f (glGetUniformLocation (programID, "ms1" ), ms1 );
  glUniform1f (glGetUniformLocation (programID, "ms2" ), ms2 );
  glUniform1f (glGetUniformLocation (programID, "ts0" ), ts0 );
  glUniform1f (glGetUniformLocation (programID, "ts1" ), ts1 );
  glUniform1f (glGetUniformLocation (programID, "ts2" ), ts2 );
  glUniform1f (glGetUniformLocation (programID, "ns"  ), ns  );
  glUniform1f (glGetUniformLocation (programID, "f0"  ), f0  );
  glUniform1f (glGetUniformLocation (programID, "lon0"), lon0);
  glUniform1f (glGetUniformLocation (programID, "x0"  ), x0  );
  glUniform1f (glGetUniformLocation (programID, "y0"  ), y0  );
  glUniform1f (glGetUniformLocation (programID, "z0"  ), z0  );
  glUniform1f (glGetUniformLocation (programID, "k0"  ), k0  );
  glUniform1f (glGetUniformLocation (programID, "rh"  ), rh  );

  unsigned int texture0;
  glGenTextures (1, &texture0);
  glBindTexture (GL_TEXTURE_2D, texture0); 
  glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	
  glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexImage2D (GL_TEXTURE_2D, 0, GL_RGB, w0, h0, 0, GL_RGB, GL_UNSIGNED_BYTE, rgb0);


  unsigned int texture1;
  glGenTextures (1, &texture1);
  glBindTexture (GL_TEXTURE_2D, texture1); 
  glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	
  glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexImage2D (GL_TEXTURE_2D, 0, GL_RGB, w1, h1, 0, GL_RGB, GL_UNSIGNED_BYTE, rgb1);

  glActiveTexture (GL_TEXTURE0); 
  glBindTexture (GL_TEXTURE_2D, texture0);
  glUniform1i (glGetUniformLocation (programID, "texture0"), 0);

  glActiveTexture (GL_TEXTURE1); 
  glBindTexture (GL_TEXTURE_2D, texture1);
  glUniform1i (glGetUniformLocation (programID, "texture1"), 1);


  lcc_t t;

  t.lon0 = lon0;
  t.lat0 = lat0; 
  t.lat1 = lat1; 
  t.lat2 = lat2;
  t.x0   = x0  ; 
  t.y0   = y0  ; 
  t.z0   = z0  ;
  t.a    = a   ; 
  t.b    = b   ; 
  t.e    = e   ;
  t.k0   = k0  ; 
  t.ns   = ns  ; 
  t.f0   = f0  ; 
  t.rh   = rh  ;


  vec3 posl = lcc_forward (t, vec2 (rad2deg * lon0, rad2deg * lat0));
  printf (" (%7.2f, %7.2f) (%10.2f, %10.2f)\n", rad2deg * lon0, rad2deg * lat0, posl.x, posl.y);


  for (int iy = 0; iy < 3; iy++)
    {
      for (int ix = 0; ix < 3; ix++)
        {
          float lon = lon0 + (ix - 1) * deg2rad;
          float lat = lat0 + (iy - 1) * deg2rad;
          vec3 posl = lcc_forward (t, vec2 (rad2deg * lon, rad2deg * lat));
          printf (" (%7.2f, %7.2f) (%10.2f, %10.2f) |", rad2deg * lon, rad2deg * lat, posl.x, posl.y);
        }
      printf ("\n");
    }


  while (1) 
    {   

      glUniform1i (glGetUniformLocation (programID, "lamb"), lamb);

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
