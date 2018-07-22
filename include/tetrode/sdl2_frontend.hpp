#pragma once
#include <tetrode/frontend.hpp>
#include <SDL2/SDL.h>

namespace tetrode {

class sdl2_frontend : public frontend {
	public:
		sdl2_frontend();
		~sdl2_frontend();

		virtual int run(void);

	private:
		void redraw(void);
		event get_event(void);
		void draw_tetrimino(tetrimino& tet, coord_2d coord);

		SDL_Window   *window;
		SDL_Renderer *renderer;
};

// namespace tetrode
}
