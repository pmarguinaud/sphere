#define EGL_EGLEXT_PROTOTYPES
#include <EGL/egl.h>
#include <EGL/eglext.h>

#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <GL/glext.h>


#include "bmp.h"
#include "gensphere.h"
#include "shader.h"
#include <stdio.h>
#include <stdlib.h>
#include <iostream>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <sys/time.h>
#include <assert.h>
#include <fcntl.h>
#include <gbm.h>
#include <unistd.h>
#include <string.h>


static void pre ()
{
  const char * m = nullptr;
  EGLint e = eglGetError (); 
  switch (e)
    {
      case EGL_SUCCESS:
        return;
      case EGL_NOT_INITIALIZED:
        m = "EGL is not initialized, or could not be initialized, for the specified EGL display connection.";
        break;
      case EGL_BAD_ACCESS:
        m = "EGL cannot access a requested resource (for example a context is bound in another thread).";
        break;
      case EGL_BAD_ALLOC:
        m = "EGL failed to allocate resources for the requested operation.";
        break;
      case EGL_BAD_ATTRIBUTE:
        m = "An unrecognized attribute or attribute value was passed in the attribute list.";
        break;
      case EGL_BAD_CONTEXT:
        m = "An EGLContext argument does not name a valid EGL rendering context.";
        break;
      case EGL_BAD_CONFIG:
        m = "An EGLConfig argument does not name a valid EGL frame buffer configuration.";
        break;
      case EGL_BAD_CURRENT_SURFACE:
        m = "The current surface of the calling thread is a window, pixel buffer or pixmap that is no longer valid.";
        break;
      case EGL_BAD_DISPLAY:
        m = "An EGLDisplay argument does not name a valid EGL display connection.";
        break;
      case EGL_BAD_SURFACE:
        m = "An EGLSurface argument does not name a valid surface (window, pixel buffer or pixmap) configured for GL rendering.";
        break;
      case EGL_BAD_MATCH:
        m = "Arguments are inconsistent (for example, a valid context requires buffers not supplied by a valid surface).";
        break;
      case EGL_BAD_PARAMETER:
        m = "One or more argument values are invalid.";
        break;
      case EGL_BAD_NATIVE_PIXMAP:
        m = "A NativePixmapType argument does not refer to a valid native pixmap.";
        break;
      case EGL_BAD_NATIVE_WINDOW:
        m = "A NativeWindowType argument does not refer to a valid native window.";
        break;
      case EGL_CONTEXT_LOST:
        m = "A power management event has occurred. The application must destroy all contexts and reinitialise OpenGL ES state and objects to continue rendering. ";
        break;
    }
  if (m != nullptr)
    printf ("%s\n", m);

  exit (1);
}

static void screenshot_ppm
(const char *filename, unsigned int width,
 unsigned int height, unsigned char *pixels)
{
  const size_t format_nchannels = 3;
  FILE * fp = fopen (filename, "w");
  fprintf (fp, "P3\n%d %d\n%d\n", width, height, 255);
  for (int i = 0; i < height; i++) 
    {
      for (int j = 0; j < width; j++) 
        {
          int cur = format_nchannels * ((height - i - 1) * width + j);
          fprintf (fp, "%3d %3d %3d ", (pixels)[cur], (pixels)[cur + 1], (pixels)[cur + 2]);
        }
      fprintf(fp, "\n");
    }
  fclose (fp);
}


int main (int argc, char * argv[])
{
  const int width = 512, height = 512;

  // Initialize EGL

  bool verbose = false;


  int fd = open ("/dev/dri/renderD128", O_RDWR);
  assert (fd > 0);


  struct gbm_device * gbm = gbm_create_device (fd);
  assert (gbm != nullptr);

  EGLDisplay display = eglGetPlatformDisplay(EGL_PLATFORM_GBM_MESA, gbm, nullptr);

  if (verbose)
    std::cerr << "display: " << std::hex << display << std::endl;
  assert (display != nullptr);


  EGLint major, minor;
  EGLBoolean result = eglInitialize (display, &major, &minor); pre ();
  if (result == EGL_FALSE)
    std::cerr << "eglInitialize failed" << std::endl;


  if (verbose)
    {
      std::cout << "Initialized EGL context version " << major << "." << minor << std::endl;
      std::cerr << "EGL_VERSION: " << eglQueryString(display, EGL_VERSION) << std::endl;
      std::cerr << "EGL_CLIENT_APIS: " << eglQueryString(display, EGL_CLIENT_APIS) << std::endl;
      std::cerr << "EGL_VENDOR: " << eglQueryString(display, EGL_VENDOR) << std::endl;
      std::cerr << "display EGL_EXTENSIONS: " << eglQueryString(display, EGL_EXTENSIONS) << std::endl;
    }

  const EGLint cfgAttr[] = 
  {
    EGL_RENDERABLE_TYPE, EGL_OPENGL_BIT,
    EGL_RED_SIZE, 8, EGL_GREEN_SIZE, 8,
    EGL_BLUE_SIZE, 8, EGL_DEPTH_SIZE, 8, EGL_NONE
  };

  const int MAX_NUM_CONFIG = 50;
  EGLint numConfig;
  EGLConfig config[MAX_NUM_CONFIG];
  memset (config, sizeof (config), 0);

  eglChooseConfig (display, cfgAttr, config, MAX_NUM_CONFIG, &numConfig); pre ();

  if (verbose)
    std::cout << "numConfig:" << numConfig << std::endl;

  const EGLint surfAttr[] = 
  {
    EGL_WIDTH, width,
    EGL_HEIGHT, height,
    EGL_NONE,
  };

  EGLSurface eglSurf = eglCreatePbufferSurface (display, config[0], surfAttr);

  eglBindAPI (EGL_OPENGL_API); pre ();

  const EGLint ctxAttr[] = 
  {
    EGL_CONTEXT_MAJOR_VERSION, 4,
    EGL_CONTEXT_MINOR_VERSION, 3,
    EGL_NONE
  };

  EGLContext context = eglCreateContext (display, config[0], EGL_NO_CONTEXT, ctxAttr); pre ();
  eglMakeCurrent (display, eglSurf, eglSurf, context); pre ();



  GLuint fbo;
  GLuint rbo_color;
  GLuint rbo_depth;
  GLuint err;


  glGenFramebuffers(1, &fbo);
  glBindFramebuffer(GL_FRAMEBUFFER, fbo);



  glGenRenderbuffers(1, &rbo_color);
  glBindRenderbuffer(GL_RENDERBUFFER, rbo_color);
  /* Storage must be one of: */
  /* GL_RGBA4, GL_RGB565, GL_RGB5_A1, GL_DEPTH_COMPONENT16, GL_STENCIL_INDEX8. */
  glRenderbufferStorage(GL_RENDERBUFFER, GL_RGB565, width, height);
  glFramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                            GL_RENDERBUFFER, rbo_color);


  glGenRenderbuffers(1, &rbo_depth);
  glBindRenderbuffer(GL_RENDERBUFFER, rbo_depth);
  glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, width, height);
  glFramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                            GL_RENDERBUFFER, rbo_depth);



  glReadBuffer (GL_COLOR_ATTACHMENT0);
  glPixelStorei (GL_UNPACK_ALIGNMENT, 1);
  glViewport(0, 0, width, height);

  glClearColor (0.0f, 0.0f, 0.0f, 0.0f);
  glEnable (GL_DEPTH_TEST);
  glEnable (GL_BLEND);
  glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glCullFace (GL_BACK);
  glFrontFace (GL_CCW);
  glEnable (GL_CULL_FACE);
  glDepthFunc (GL_LESS); 


  int Nj = atoi (argv[1]);
  int np; 
  float * xyz;
  unsigned int nt;
  unsigned int * ind;
  int w, h;
  unsigned char * rgb = nullptr;

  bmp ("Whole_world_-_land_and_oceans_8000.bmp", &rgb, &w, &h);

  gensphere1 (Nj, &np, &xyz, &nt, &ind);



  GLuint VertexArrayID;
  GLuint vertexbuffer, colorbuffer, elementbuffer;

  glGenVertexArrays (1, &VertexArrayID);
  glBindVertexArray (VertexArrayID);

  glGenBuffers (1, &vertexbuffer);
  glBindBuffer (GL_ARRAY_BUFFER, vertexbuffer);
  glBufferData (GL_ARRAY_BUFFER, 3 * np * sizeof (float), xyz, GL_STATIC_DRAW);
  glEnableVertexAttribArray (0); 
  glVertexAttribPointer (0, 3, GL_FLOAT, GL_FALSE, 0, nullptr); 
  
  glGenBuffers (1, &elementbuffer);
  glBindBuffer (GL_ELEMENT_ARRAY_BUFFER, elementbuffer);
  glBufferData (GL_ELEMENT_ARRAY_BUFFER, 3 * nt * sizeof (unsigned int), ind , GL_STATIC_DRAW);

  GLuint programID = shader 
(
R"CODE(
#version 330 core

in vec3 fragmentPos;

out vec4 color;

uniform sampler2D texture;

void main()
{
  float lon = (atan (fragmentPos.y, fragmentPos.x) / 3.1415926 + 1.0) * 0.5;
  float lat = asin (fragmentPos.z) / 3.1415926 + 0.5;
  color = texture2D (texture, vec2 (lon, lat));
  color.a = 1.0;
}
)CODE",
R"CODE(
#version 330 core

layout(location = 0) in vec3 vertexPos;

out vec3 fragmentPos;

uniform mat4 MVP;

void main()
{
  gl_Position =  MVP * vec4 (vertexPos, 1);
  fragmentPos = vertexPos;
}
)CODE");

  glUseProgram (programID);

  glm::mat4 Projection = glm::perspective (glm::radians (20.0f), 1.0f / 1.0f, 0.1f, 100.0f);
  glm::mat4 View       = glm::lookAt (glm::vec3 (6.0f,0.0f,0.0f), glm::vec3 (0,0,0), glm::vec3 (0,0,1));
  glm::mat4 Model      = glm::mat4 (1.0f);

  glm::mat4 MVP = Projection * View * Model; 

  glUniformMatrix4fv (glGetUniformLocation (programID, "MVP"), 1, GL_FALSE, &MVP[0][0]);


  unsigned int texture;
  glGenTextures (1, &texture);
  glBindTexture (GL_TEXTURE_2D, texture); 
  glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	// set texture wrapping to GL_REPEAT (default wrapping method)
  glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexImage2D (GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, rgb);

  glUniform1i (glGetUniformLocation (programID, "texture"), 0);

  glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  glBindVertexArray (VertexArrayID);
  glDrawElements (GL_TRIANGLES, 3 * nt, GL_UNSIGNED_INT, nullptr);

  glFlush ();



  unsigned char * pixels = (unsigned char *)malloc(3 * sizeof (unsigned char) * width * height);
  memset (pixels, 0, 3 * sizeof (unsigned char) * width * height);
  glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, pixels);

  int c[3] = {0, 0, 0};
  for (int i = 0; i < width * height; i++)
    for (int j = 0; j < 3; j++)
      if (pixels[3*i+j] != 0)
        c[j]++;

  for (int j = 0; j < 3; j++)
    printf (" c[%d] = %d\n", j, c[j]);

  screenshot_ppm ("toto.ppm", width, height, pixels);
  free (pixels);


  glDeleteFramebuffers(1, &fbo);
  glDeleteRenderbuffers(1, &rbo_color);
  glDeleteRenderbuffers(1, &rbo_depth);

  eglTerminate(display);

  return 0;
}
