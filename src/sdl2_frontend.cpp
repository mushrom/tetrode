#include <tetrode/sdl2_frontend.hpp>
#include <tetrode/frontend.hpp>
#include <tetrode/field_state.hpp>

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_mixer.h>

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
	{ block::states::Cleared,  {0xf0, 0xdd, 0xf0} },

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
	menus.push_front(main_menu());

	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0){
		throw "SDL_Init()";
	}

	if (TTF_Init() == -1) {
		throw "TTF_Init()";
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

	font = TTF_OpenFont("assets/fonts/LiberationSans-Regular.ttf", BLOCK_FILL_SIZE);

	if (!font) {
		throw "TTF_OpenFont()";
	}

	int mix_flags = MIX_INIT_OGG;
	if (Mix_Init(mix_flags) != mix_flags) {
		throw "Mix_Init()";
	}

	if (Mix_OpenAudio(48000, MIX_DEFAULT_FORMAT, 2, 1024) == -1) {
		throw "Mix_OpenAudio()";
	}

	sfx.locked   = Mix_LoadWAV("assets/sfx/locked.ogg");
	sfx.rotation = Mix_LoadWAV("assets/sfx/rotation.ogg");
	sfx.tspin    = Mix_LoadWAV("assets/sfx/tspin.ogg");
	sfx.wallhit  = Mix_LoadWAV("assets/sfx/wallhit.ogg");

	if (!sfx.locked || !sfx.rotation || !sfx.tspin || !sfx.wallhit) {
		throw "Mix_LoadWAV()";
	}

	Mix_AllocateChannels(32);
}

sdl2_frontend::~sdl2_frontend(){
	// Close fonts
	TTF_CloseFont(font);
	font = NULL;
	TTF_Quit();

	// Close audio
	for (auto& x : {sfx.locked, sfx.rotation, sfx.tspin, sfx.wallhit}) {
		Mix_FreeChunk(x);
	}

	Mix_CloseAudio();
	Mix_Quit();

	SDL_Quit();
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
				case SDLK_LEFT:
				case SDLK_h:      return event::MoveLeft;
				case SDLK_RIGHT:
				case SDLK_l:      return event::MoveRight;
				case SDLK_LCTRL:
				case SDLK_RCTRL:
				case SDLK_z:
				case SDLK_j:      return event::RotateLeft;
				case SDLK_x:
				case SDLK_UP:
				case SDLK_k:      return event::RotateRight;
				case SDLK_SPACE:  return event::Drop;
				case SDLK_DOWN:   return event::MoveDown;
				case SDLK_F1:
				case SDLK_ESCAPE: return event::Pause;
				case SDLK_c:
				case SDLK_RSHIFT:
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

void sdl2_frontend::draw_text(std::string& str, coord_2d coord) {
	SDL_Color color = {0xff, 0xff, 0xff};
	SDL_Surface *text_surface;

	if (!(text_surface = TTF_RenderText_Blended(font, str.c_str(), color))){
		throw "TTF_RenderText_Blended()";

	} else {
		SDL_Texture *text = SDL_CreateTextureFromSurface(renderer, text_surface);
		SDL_Rect rect;

		rect.x = BLOCK_FULL_SIZE + coord.x * BLOCK_FULL_SIZE;
		rect.y = BLOCK_FULL_SIZE + coord.y * BLOCK_FULL_SIZE;
		rect.w = text_surface->w;
		rect.h = text_surface->h;;

		SDL_RenderCopy(renderer, text, NULL, &rect);

		// don't forget to free everything lol
		SDL_DestroyTexture(text);
		SDL_FreeSurface(text_surface);
	}
}

void sdl2_frontend::draw_field(field_state& n_field){
	SDL_Rect rect;
	rect.w = rect.h = BLOCK_FILL_SIZE;

	for (int y = n_field.size.y / 2; y >= 0; y--) {
		for (int x = 0; x < n_field.size.x; x++) {
			auto color = block_colors[n_field.field[y][x].state];

			rect.x = 48 + x       * BLOCK_FULL_SIZE;
			rect.y = 48 + ((n_field.size.y / 2) - y) * BLOCK_FULL_SIZE;

			SDL_SetRenderDrawColor(renderer,
			                       std::get<0>(color),
			                       std::get<1>(color),
			                       std::get<2>(color), 0);
			SDL_RenderFillRect(renderer, &rect);
		}
	}

	coord_2d ghost_coord = n_field.lower_collide_coord(n_field.active.first, n_field.active.second);
	tetrimino ghost_block = n_field.active.first;

	for (auto& block : ghost_block.blocks){
		block.first.state = block::states::Ghost;
	}

	draw_tetrimino( ghost_block, ghost_coord );
	draw_tetrimino( n_field.active.first, n_field.active.second );
	draw_tetrimino( n_field.next_pieces.front(),
	                coord_2d(n_field.size.x + 2, 2 ));

	if (n_field.have_held) {
		draw_tetrimino( n_field.hold,
		                coord_2d(n_field.size.x + 2, 8 ));
	}

	std::string score_str = "score: " + std::to_string(n_field.score);
	std::string level_str = "level: " + std::to_string(n_field.level);
	std::string lines_str = "cleared: " + std::to_string(n_field.lines_cleared);

	draw_text(level_str, coord_2d(n_field.size.x + 2, 2));
	draw_text(score_str, coord_2d(n_field.size.x + 2, 3));
	draw_text(lines_str, coord_2d(n_field.size.x + 2, 4));
}

void sdl2_frontend::draw_menus(void){
	SDL_DisplayMode dm;
	SDL_GetCurrentDisplayMode(0, &dm);

	for (auto& x : menus) {
		SDL_Rect rect;
		rect.h = dm.h;
		rect.w = 150;

		rect.x = 0;
		rect.y = 0;

		SDL_SetRenderDrawColor(renderer, 0x20, 0x20, 0x20, 0);
		SDL_RenderFillRect(renderer, &rect);

		unsigned i = 0;
		for (auto& entry : x.entries) {
			if (*x.selected == entry) {
				std::string asdf = ">" + entry->text;
				draw_text(asdf, coord_2d(0, i));

			} else {
				draw_text(entry->text, coord_2d(0, i));
			}

			i += 1;
		}
	}
}

void sdl2_frontend::clear(void){
	SDL_RenderClear(renderer);
	SDL_SetRenderDrawColor(renderer, 0x8, 0x8, 0x8, 0);
	SDL_RenderFillRect(renderer, NULL);
}

void sdl2_frontend::present(void){
	SDL_RenderPresent(renderer);
}

void sdl2_frontend::redraw(void){
	clear();
	draw_field(field);

	if (!menus.empty()){
		draw_menus();
	}

	present();
}

void sdl2_frontend::play_sfx(void){
	if (field.updates & changes::Locked) {
		Mix_PlayChannel(-1, sfx.locked, 0);
	}

	else if (field.updates & changes::Tspin) {
		Mix_PlayChannel(-1, sfx.tspin, 0);
	}

	else if (field.updates & changes::Rotated) {
		Mix_PlayChannel(-1, sfx.rotation, 0);
	}

	else if (field.updates & changes::WallHit) {
		Mix_PlayChannel(-1, sfx.wallhit, 0);
	}
}

int sdl2_frontend::run(void){
	event ev;
	redraw();

	while ((ev = get_event()) != event::Quit) {
		if (ev == event::Pause) {
			paused = !paused;
		}

		if (!menus.empty() && ev != event::NullEvent) {
			menus.back().handle_event(this, ev);
			redraw();
		}

		else if (!paused) {
			field.handle_event(event::Tick);
			field.handle_event(ev);

			play_sfx();

			if (field.updates & changes::Updated) {
				redraw();
			}
		}

		field.updates = 0;
		SDL_Delay(10);
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
