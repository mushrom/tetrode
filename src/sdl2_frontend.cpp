#include <tetrode/sdl2_frontend.hpp>
#include <tetrode/frontend.hpp>
#include <tetrode/field_state.hpp>
#include <SDL2/SDL.h>

#include <map>
#include <tuple>
#include <stdio.h>

namespace tetrode {

#define BLOCK_FILL_SIZE 21
#define BLOCK_FULL_SIZE 24

std::map<block::states, std::tuple<unsigned, unsigned, unsigned>> block_colors = {
	{ block::states::Empty,    {0x11, 0x11, 0x11} },
	{ block::states::Reserved, {0x22, 0x11, 0x11} },
	{ block::states::Ghost,    {0x88, 0xaa, 0xdd} },

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

void sdl2_frontend::draw_tetrimino(tetrimino& tet, coord_2d coord){
	SDL_Rect rect;
	rect.w = rect.h = BLOCK_FILL_SIZE;

	for (const auto &block : tet.blocks) {
		auto color = block_colors[block.first.state];

		//int x = (block.second.x + field.active.second.x);
		//int y = (block.second.y + field.active.second.y);
		int x = (block.second.x + coord.x);
		int y = (block.second.y + coord.y);

		rect.x = 48 + x       * BLOCK_FULL_SIZE;
		rect.y = 48 + ((field.size.y / 2) - y) * BLOCK_FULL_SIZE;

		SDL_SetRenderDrawColor(renderer,
				std::get<0>(color),
				std::get<1>(color),
				std::get<2>(color), 0);
		SDL_RenderFillRect(renderer, &rect);
	}
}

void sdl2_frontend::redraw(void){
	SDL_RenderClear(renderer);
	SDL_SetRenderDrawColor(renderer, 0x8, 0x8, 0x8, 0);
	SDL_RenderFillRect(renderer, NULL);

	SDL_Rect rect;
	rect.w = rect.h = BLOCK_FILL_SIZE;

	for (int y = field.size.y / 2; y >= 0; y--) {
		for (int x = 0; x < field.size.x; x++) {
			auto color = block_colors[field.field[y][x].state];

			rect.x = 48 + x       * BLOCK_FULL_SIZE;
			rect.y = 48 + ((field.size.y / 2) - y) * BLOCK_FULL_SIZE;

			SDL_SetRenderDrawColor(renderer,
			                       std::get<0>(color),
			                       std::get<1>(color),
			                       std::get<2>(color), 0);
			SDL_RenderFillRect(renderer, &rect);
		}
	}

	coord_2d ghost_coord = field.lower_collide_coord(field.active.first, field.active.second);
	tetrimino ghost_block = field.active.first;

	for (auto& block : ghost_block.blocks){
		block.first.state = block::states::Ghost;
	}

	draw_tetrimino( ghost_block, ghost_coord );
	draw_tetrimino( field.active.first, field.active.second );
	draw_tetrimino( field.next_pieces.front(),
	                coord_2d(field.size.x + 2, 2 ));

	if (field.have_held) {
		draw_tetrimino( field.hold,
		                coord_2d(field.size.x + 2, 8 ));
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
