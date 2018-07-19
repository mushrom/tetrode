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

		SDL_Window   *window;
		SDL_Renderer *renderer;
};

// namespace tetrode
}
