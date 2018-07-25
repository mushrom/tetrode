SDL2_FLAGS_y=`sdl2-config --cflags --libs` -lSDL2_ttf
CXXFLAGS=-std=c++11 -Wall -O2 -march=native -I./include $(SDL2_FLAGS_y)

BASE_SRC=src/field_state.cpp src/frontend.cpp
BASE_OBJ=$(BASE_SRC:.cpp=.o)

SDL2_SRC=src/sdl2_frontend.cpp
SDL2_OBJ=$(SDL2_SRC:.cpp=.o)

tetrode-sdl: $(BASE_OBJ) $(SDL2_OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $(BASE_OBJ) $(SDL2_OBJ)

.PHONY: clean
clean:
	rm -f tetrode-sdl $(BASE_OBJ) $(SDL2_OBJ)
