

main.x: main.cc gensphere.cc shader.cc bmp.cc
	g++ -std=c++11 -fopenmp -g -o main.x main.cc gensphere.cc shader.cc bmp.cc -lEGL -lGL -lgbm

clean:
	\rm -f *.o *.x
