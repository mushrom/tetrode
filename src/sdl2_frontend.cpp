#include <tetrode/sdl2_frontend.hpp>
#include <tetrode/frontend.hpp>
#include <tetrode/field_state.hpp>
#include <SDL2/SDL.h>

#include <map>
#include <tuple>
#include <stdio.h>

namespace tetrode {

std::map<block::states, std::tuple<unsigned, unsigned, unsigned>> block_colors = {
	{ block::states::Empty,    {0x11, 0x11, 0x11} },
	{ block::states::Reserved, {0x22, 0x11, 0x11} },
	{ block::states::Garbage,  {0x22, 0x22, 0x22} },
	{ block::states::Cyan,     {0x44, 0x88, 0xaa} },
	{ block::states::Yellow,   {0xaa, 0xaa, 0x44} },
	{ block::states::Purple,   {0xaa, 0x44, 0xaa} },
	{ block::states::Green,    {0x44, 0xaa, 0x44} },
	{ block::states::Red,      {0xaa, 0x44, 0x44} },
	{ block::states::Blue,     {0x44, 0x44, 0xaa} },
	{ block::states::Orange,   {0xaa, 0x66, 0x44} },
};

sdl2_frontend::sdl2_frontend() {
	if (SDL_Init(SDL_INIT_VIDEO) < 0){
		throw "SDL_Init()";
	}

	window = SDL_CreateWindow("Tetrode SDL",
	                          SDL_WINDOWPOS_UNDEFINED,
	                          SDL_WINDOWPOS_UNDEFINED,
	                          480,
	                          640,
	                          SDL_WINDOW_SHOWN);

	if (!window) {
		throw "SDL_CreateWindow()";
	}

	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

	if (!renderer) {
		throw "SDL_CreateRenderer()";
	}
}

sdl2_frontend::~sdl2_frontend(){

}

event sdl2_frontend::get_event(void){
	SDL_Event e;

	if (SDL_PollEvent(&e) != 0) {
		puts("Got event!");
		if (e.type == SDL_QUIT){
			return event::Quit;
		}

		else if (e.type == SDL_KEYDOWN) {
			switch (e.key.keysym.sym) {
				case SDLK_q:      return event::Quit;
				case SDLK_h:      return event::MoveLeft;
				case SDLK_l:      return event::MoveRight;
				case SDLK_j:      return event::RotateLeft;
				case SDLK_k:      return event::RotateRight;
				case SDLK_SPACE:  return event::Drop;
				case SDLK_ESCAPE: return event::Pause;
				case SDLK_LSHIFT: return event::Hold;
				default:          break;
			}
		}
	}

	return event::NullEvent;
}

void sdl2_frontend::redraw(void){
	SDL_RenderClear(renderer);
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
	SDL_RenderFillRect(renderer, NULL);

	SDL_Rect rect;
	rect.w = rect.h = 21;

	for (int y = field.size.y / 2; y >= 0; y--) {
		for (int x = 0; x < field.size.x; x++) {
			auto color = block_colors[field.field[y][x].state];

			rect.x = 1 + x       * 24;
			rect.y = 1 + ((field.size.y / 2) - y) * 24;

			SDL_SetRenderDrawColor(renderer,
			                       std::get<0>(color),
			                       std::get<1>(color),
			                       std::get<2>(color), 0);
			SDL_RenderFillRect(renderer, &rect);
		}
	}

	for (const auto &block : field.active.first.blocks) {
		auto color = block_colors[block.first.state];

		int x = (block.second.x + field.active.second.x);
		int y = (block.second.y + field.active.second.y);

		rect.x = 1 + x       * 24;
		rect.y = 1 + ((field.size.y / 2) - y) * 24;

		SDL_SetRenderDrawColor(renderer,
				std::get<0>(color),
				std::get<1>(color),
				std::get<2>(color), 0);
		SDL_RenderFillRect(renderer, &rect);
	}

	for (const auto &block : field.next_pieces.front().blocks) {
		auto color = block_colors[block.first.state];

		int x = block.second.x;
		int y = block.second.y;

		rect.x = (field.size.x * 24) + 48 + x * 24;
		rect.y = (2 - y) * 24;

		SDL_SetRenderDrawColor(renderer,
				std::get<0>(color),
				std::get<1>(color),
				std::get<2>(color), 0);
		SDL_RenderFillRect(renderer, &rect);
	}

	if (field.have_held) {
		for (const auto &block : field.hold.blocks) {
			auto color = block_colors[block.first.state];

			int x = block.second.x;
			int y = block.second.y;

			rect.x = (field.size.x * 24) + 48 + x * 24;
			rect.y = (8 - y) * 24;

			SDL_SetRenderDrawColor(renderer,
					std::get<0>(color),
					std::get<1>(color),
					std::get<2>(color), 0);
			SDL_RenderFillRect(renderer, &rect);
		}
	}

	SDL_RenderPresent(renderer);
}

int sdl2_frontend::run(void){
	event ev;
	unsigned ticks = 0;

	while ((ev = get_event()) != event::Quit) {
		if (ticks >= 15) {
			field.handle_event(event::MoveDown);
			ticks = 0;
		}

		field.handle_event(ev);
		redraw();
		SDL_Delay(10);
		ticks++;
	}

	return 0;
}

// namespace tetrode
}

int main(int argc, char *argv[]){
	tetrode::sdl2_frontend foo;
	foo.run();
	return 0;
}
