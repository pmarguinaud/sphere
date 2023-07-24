#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "shader.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


static void checkShader (GLuint shaderID)
{
  int len;
  GLint res = GL_FALSE;
  glGetShaderiv (shaderID, GL_COMPILE_STATUS, &res);
  glGetShaderiv (shaderID, GL_INFO_LOG_LENGTH, &len);
  if (len > 0)
    {
      char mess[len+1];
      glGetShaderInfoLog (shaderID, len, NULL, &mess[0]);
      printf("%s\n", mess);
    }
}

static void checkProgram (GLuint programID)
{
  int len;
  GLint res = GL_FALSE;
  glGetProgramiv (programID, GL_LINK_STATUS, &res);
  glGetProgramiv (programID, GL_INFO_LOG_LENGTH, &len);
  if (len > 0)
    {
      char mess[len+1];
      glGetProgramInfoLog (programID, len, NULL, &mess[0]);
      printf("%s\n", mess);
    }
}


GLuint shader (const char * FragmentShaderCode, 
               const char * VertexShaderCode,
               const char * ComputeShaderCode)
{
  GLuint ProgramID = 0;

  if (FragmentShaderCode && VertexShaderCode)
    {
      int len;
      GLint res = GL_FALSE;
      GLuint VertexShaderID = glCreateShader (GL_VERTEX_SHADER);
      GLuint FragmentShaderID = glCreateShader (GL_FRAGMENT_SHADER);

      // Compile Vertex Shader
      glShaderSource (VertexShaderID, 1, &VertexShaderCode , NULL);
      glCompileShader (VertexShaderID);
      checkShader (VertexShaderID);

      // Compile Fragment Shader
      glShaderSource (FragmentShaderID, 1, &FragmentShaderCode , NULL);
      glCompileShader (FragmentShaderID);
      checkShader (FragmentShaderID);

      // Link the program
      ProgramID = glCreateProgram ();
      glAttachShader (ProgramID, VertexShaderID);
      glAttachShader (ProgramID, FragmentShaderID);
      glLinkProgram (ProgramID);
      checkProgram (ProgramID);
      
      glDetachShader (ProgramID, VertexShaderID);
      glDetachShader (ProgramID, FragmentShaderID);
      
      glDeleteShader (VertexShaderID);
      glDeleteShader (FragmentShaderID);
      
    }
  else if (ComputeShaderCode)
    {
      GLuint ComputeID;
      ComputeID = glCreateShader (GL_COMPUTE_SHADER);
      glShaderSource (ComputeID, 1, &ComputeShaderCode, NULL);
      glCompileShader (ComputeID);
      checkShader  (ComputeID);
      ProgramID = glCreateProgram();
      glAttachShader (ProgramID, ComputeID);
      glLinkProgram (ProgramID);
      checkProgram (ProgramID);
    }
  return ProgramID;
}


