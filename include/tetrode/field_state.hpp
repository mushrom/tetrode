#pragma once
#include <vector>
#include <list>
#include <utility> // std::pair
#include <stdint.h>

namespace tetrode {

enum event {
	NullEvent,

	Tick,
	RotateLeft,
	RotateRight,
	MoveLeft,
	MoveRight,
	MoveDown,
	Drop,
	Hold,
	Pause,

	Quit,
};

enum movement {
	Left, Right,
};

class block {
	public:
		enum states {
			Empty,
			Reserved,
			Ghost,
			Cleared,

			Garbage,
			Cyan,
			Yellow,
			Purple,
			Green,
			Red,
			Blue,
			Orange,
		};

		block(enum states new_state = states::Empty){
			state = new_state;
		}

		enum states state;
};

class coord_2d {
	public:
		coord_2d(int nx = 0, int ny = 0){
			x = nx;
			y = ny;
		};

		bool operator == (const coord_2d& other){
			return (x = other.x) && (y == other.y);
		}

		int x, y;
};

class tetrimino {
	public:
		enum shape {
			I, O, T, S, Z, J, L,
		};

		tetrimino(enum shape new_shape=shape::I){
			shape = new_shape;
			regen_blocks();
		}

		void rotate(enum movement dir);

		std::list<std::pair<block, coord_2d>> blocks;
		unsigned rotations : 2; // only 4 possible states, so we only need two bits
		enum shape shape;

		void regen_blocks(void);
};

class field_state {
	public:
		field_state(unsigned board_x=10, unsigned board_y=40, uint32_t seed=0);
		void handle_event(enum event ev);
		coord_2d lower_collide_coord(tetrimino& tet, coord_2d& coord);

		coord_2d size;
		std::pair<tetrimino, coord_2d> active;

		tetrimino hold;
		bool have_held = false;
		bool already_held = false;

		std::list<tetrimino> next_pieces;
		std::vector<std::vector<block>> field;

		// TODO: implement prng
		uint32_t random_seed;

		unsigned movement_ticks;
		unsigned clear_ticks;
		unsigned drop_ticks;

		unsigned level;
		unsigned score;
		unsigned lines_cleared;

	private:
		void generate_next_pieces(void);
		void place_active(void);
		void get_new_active_tetrimino(void);
		int  clear_lines(void);
		int  color_cleared_lines(void);

		bool collides_lower(tetrimino& tet, coord_2d& coord);
		bool active_collides_lower(void);
		bool active_collides_sides(enum movement dir);
		void active_normalize(void);
		void rotation_normalize(void);
};

// namespace tetrode
}
