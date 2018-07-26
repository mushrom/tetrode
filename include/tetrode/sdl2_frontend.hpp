#pragma once
#include <tetrode/frontend.hpp>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_mixer.h>

#include <string>

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
		void draw_text(std::string& text, coord_2d coord);

		SDL_Window   *window;
		SDL_Renderer *renderer;
		TTF_Font     *font;

		struct {
			Mix_Chunk *rotation;
			Mix_Chunk *locked;
			Mix_Chunk *wallhit;
			Mix_Chunk *tspin;
		} sfx;
};

// namespace tetrode
}
