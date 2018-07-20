#include <tetrode/field_state.hpp>
#include <stdlib.h> // rand(), TODO: implement prng

#include <stdio.h>

namespace tetrode {

field_state::field_state(unsigned board_x, unsigned board_y, uint32_t seed){
	// initialize game state
	random_seed = seed;
	size = coord_2d(board_x, board_y);
	get_new_active_tetrimino();

	// initialize playing field vector
	field.resize(board_y);
	for (std::vector<block> &row : field) {
		row.resize(board_x);
	}
}

void field_state::get_new_active_tetrimino(void){
	// make sure there's enough pieces in the queue for the preview
	// and popping a new block
	if (next_pieces.size() < 2) {
		generate_next_pieces();
	}

	tetrimino piece = next_pieces.front();
	next_pieces.pop_front();
	active = { piece, coord_2d(size.x / 2 - 1, size.y / 2 + 1) };
}

void field_state::place_active(void){
	for (auto& block : active.first.blocks) {
		auto& coord = active.second;
		field[coord.y + block.second.y][coord.x + block.second.x] = block.first;
	}

	clear_lines();
	get_new_active_tetrimino();
	already_held = false;
}

bool field_state::active_collides_lower(void){
	for (auto& block : active.first.blocks) {
		auto& coord = active.second;

		int y = block.second.y + coord.y;
		int x = block.second.x + coord.x;

		if (y <= 0){
			return true;
		}

		if (field[y - 1][x].state != block::states::Empty){
			return true;
		}
	}

	return false;
}

bool field_state::active_collides_sides(enum movement dir){
	for (auto& block : active.first.blocks) {
		auto& coord = active.second;

		unsigned y = block.second.y + coord.y;
		unsigned x = block.second.x + coord.x;

		if (dir == movement::Left){
			if (x == 0) {
				return true;
			}

			if (field[y][x - 1].state != block::states::Empty){
				return true;
			}
		}

		else {
			if (x + 1 >= size.x) {
				return true;
			}

			if (field[y][x + 1].state != block::states::Empty){
				return true;
			}
		}
	}

	return false;
}

void field_state::active_normalize(void){
	int min_x = 0;
	int min_y = 0;

	int over_x = 0;
	int over_y = 0;

	for (auto& block : active.first.blocks) {
		auto& coord = active.second;

		int y = block.second.y + coord.y;
		int x = block.second.x + coord.x;

		if (y < min_y){
			min_y = y;
		}

		if (x < min_x){
			min_x = x;
		}

		if (y >= size.y && y > over_y) {
			over_y = y;
		}

		if (x >= size.x && x > over_x) {
			over_x = x;
		}

		printf("coord: %d, %d, %d, %d\n", x, y, size.x, size.y);
		printf("coord: over_x: %d\n", over_x);

	}

	active.second.x -= min_x;
	active.second.y -= min_y;

	active.second.x -= over_x? over_x - size.x + 1: 0;
	active.second.y -= over_y? over_y - size.y + 1: 0;

	printf("active: %d, %d\n", active.second.x, active.second.y);
}

void field_state::handle_event(enum event ev){
	switch (ev) {
		case event::MoveDown:
			// TODO: timeout for moving pieces around after collision
			if (!active_collides_lower()) {
				active.second.y -= 1;

			}
			/*else {
				place_active();
			}*/

			break;

		case event::Drop:
			while (!active_collides_lower()) {
				active.second.y -= 1;
			}

			place_active();
			break;

		case event::Hold:
			if (!already_held) {
				if (have_held) {
					auto temp = active.first;
					active.first = hold;
					hold = temp;
					active.second = coord_2d(size.x / 2 - 1, size.y / 2 + 1);
				}

				else {
					hold = active.first;
					have_held = true;
					get_new_active_tetrimino();
				}

				hold.regen_blocks();
				already_held = true;
			}

			break;

		case event::MoveLeft:
			if (!active_collides_sides(movement::Left)) {
				active.second.x -= 1;
			}

			break;

		case event::MoveRight:
			if (!active_collides_sides(movement::Right)) {
				active.second.x += 1;
			}
			break;

		case event::RotateLeft:
			active.first.rotate(movement::Left);
			active_normalize();

			while (active_collides_lower()) {
				active.second.y += 1;
			}

			break;

		case event::RotateRight:
			active.first.rotate(movement::Right);
			active_normalize();

			while (active_collides_lower()) {
				active.second.y += 1;
			}

			break;
	}
}

void field_state::generate_next_pieces(void){
	std::list<tetrimino> pieces;

	// 7-bag random generator, generate a list of all 7 tetriminos
	for (unsigned i = tetrimino::shape::I; i <= tetrimino::shape::L; i++) {
		tetrimino new_piece(static_cast<enum tetrimino::shape>(i));
		pieces.push_back(new_piece);
	}

	// then insert them into the piece queue in a random order
	while (!pieces.empty()) {
		unsigned index = rand() % pieces.size();
		std::list<tetrimino>::iterator it = std::next(pieces.begin(), index);

		next_pieces.push_back(*it);
		pieces.erase(it, std::next(it, 1));
	}
}

void field_state::clear_lines(void){
	//for (unsigned y = 0; y < size.y; y++) {
	for (unsigned y = 0; y < size.y;) {
		bool full = true;

		for (auto& block : field[y]) {
			if (block.state == block::states::Empty
			    || block.state == block::states::Reserved)
			{
				full = false;
				break;
			}
		}

		if (full) {
			for (unsigned j = y; j < size.y - 1; j++) {
				field[j] = field[j + 1];
			}

			field[size.y - 1].resize(size.x);
		}

		else {
			y += 1;
		}
	}
}

void tetrimino::rotate(enum movement dir){
	if (shape == shape::O) {
		// no rotations for the O tetrimino
		return;
	}

	for (auto& block : blocks) {
		auto orig = block.second;

		if (dir == movement::Left) {
			block.second.x = -orig.y;
			block.second.y = orig.x;
		}

		else {
			block.second.x = orig.y;
			block.second.y = -orig.x;
		}
	}
}

void tetrimino::regen_blocks(void){
	// TODO: maybe find a more concise way to do this
	switch (shape) {
		case shape::I:
			blocks = {
				{ block(block::states::Cyan), coord_2d(-1, 0) },
				{ block(block::states::Cyan), coord_2d( 0, 0) },
				{ block(block::states::Cyan), coord_2d( 1, 0) },
				{ block(block::states::Cyan), coord_2d( 2, 0) },
			};
			break;

		case shape::O:
			blocks = {
				{ block(block::states::Yellow), coord_2d(0, 0) },
				{ block(block::states::Yellow), coord_2d(0, 1) },
				{ block(block::states::Yellow), coord_2d(1, 0) },
				{ block(block::states::Yellow), coord_2d(1, 1) },
			};
			break;

		case shape::T:
			blocks = {
				{ block(block::states::Purple), coord_2d( 0, 1) },
				{ block(block::states::Purple), coord_2d(-1, 0) },
				{ block(block::states::Purple), coord_2d( 0, 0) },
				{ block(block::states::Purple), coord_2d( 1, 0) },
			};
			break;

		case shape::S:
			blocks = {
				{ block(block::states::Green), coord_2d(-1, 0) },
				{ block(block::states::Green), coord_2d( 0, 0) },
				{ block(block::states::Green), coord_2d( 0, 1) },
				{ block(block::states::Green), coord_2d( 1, 1) },
			};
			break;

		case shape::Z:
			blocks = {
				{ block(block::states::Red), coord_2d( 0, 0) },
				{ block(block::states::Red), coord_2d( 1, 0) },
				{ block(block::states::Red), coord_2d(-1, 1) },
				{ block(block::states::Red), coord_2d( 0, 1) },
			};
			break;

		case shape::J:
			blocks = {
				{ block(block::states::Blue), coord_2d(-1, 0) },
				{ block(block::states::Blue), coord_2d( 0, 0) },
				{ block(block::states::Blue), coord_2d( 1, 0) },
				{ block(block::states::Blue), coord_2d(-1, 1) },
			};
			break;

		case shape::L:
			blocks = {
				{ block(block::states::Orange), coord_2d(-1, 0) },
				{ block(block::states::Orange), coord_2d( 0, 0) },
				{ block(block::states::Orange), coord_2d( 1, 0) },
				{ block(block::states::Orange), coord_2d( 1, 1) },
			};
			break;

		default: break;
	}
}

// namespace tetrode
}
