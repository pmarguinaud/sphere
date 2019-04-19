

main.x: main.cc gensphere.cc shader.cc bmp.cc
	g++ -g -o main.x main.cc gensphere.cc shader.cc bmp.cc -lglfw -lGLEW -lGL
