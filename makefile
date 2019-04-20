

main.x: main.cc gensphere.cc shader.cc bmp.cc
	g++ -fopenmp -g -o main.x main.cc gensphere.cc shader.cc bmp.cc -lglfw -lGLEW -lGL

clean:
	\rm -f *.o *.x
