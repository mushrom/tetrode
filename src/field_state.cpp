#include <tetrode/field_state.hpp>
#include <stdlib.h> // rand(), TODO: implement prng

#include <stdio.h>

namespace tetrode {

field_state::field_state(unsigned board_x, unsigned board_y, uint32_t seed){
	// initialize game state
	random_seed = seed;
	size = coord_2d(board_x, board_y);
	lines_cleared = score = drop_ticks = movement_ticks = clear_ticks = 0;
	level = 1;
	updates = changes::Updated;

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
	int cleared = 0;

	for (auto& block : active.first.blocks) {
		auto& coord = active.second;
		field[coord.y + block.second.y][coord.x + block.second.x] = block.first;
	}

	if ((cleared = color_cleared_lines())) {
		clear_ticks = 30;
		lines_cleared += cleared;

		// TODO: variable goal levels
		level = 1 + (lines_cleared / 10);

		switch (cleared) {
			case 1: score += 100 * level; break;
			case 2: score += 300 * level; break;
			case 3: score += 500 * level; break;
			case 4: score += 800 * level; break;
			default: /* wut */ break;
		}
	}

	get_new_active_tetrimino();

	// clear drop counter in case there was a collision, reset hold status
	drop_ticks = 0;
	already_held = false;
	updates |= changes::Locked | changes::Updated;
}

bool field_state::collides_lower(tetrimino& tet, coord_2d& coord){
	for (auto& block : tet.blocks) {
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

bool field_state::active_collides_lower(void){
	return collides_lower(active.first, active.second);
}

coord_2d field_state::lower_collide_coord(tetrimino& tet, coord_2d& coord){
	coord_2d ret = coord;

	while (!collides_lower(tet, ret)){
		ret.y -= 1;
	}

	return ret;
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
	}

	active.second.x -= min_x;
	active.second.y -= min_y;

	active.second.x -= over_x? over_x - size.x + 1: 0;
	active.second.y -= over_y? over_y - size.y + 1: 0;
}

void field_state::rotation_normalize(void){
	bool collided = false;

	active_normalize();
	while (active_collides_lower()) {
		active.second.y += 1;
		collided = true;
	}

	active.second.y -= collided;
}

void field_state::handle_event(enum event ev){
	// clear_ticks set by place_active, to add a delay when a row is cleared
	if (clear_ticks > 0) {
		clear_ticks--;

		if (clear_ticks == 0) {
			clear_lines();
			updates |= changes::Updated;
		}

		return;
	}

	switch (ev) {
		case event::Tick:
			if (movement_ticks >= 15){
				movement_ticks = 0;
				handle_event(event::MoveDown);
			}

			if (drop_ticks && active_collides_lower()) {
				drop_ticks++;

				if (drop_ticks > 50) {
					place_active();
					drop_ticks = 0;
				}

			} else {
				drop_ticks = 0;
			}

			movement_ticks++;
			break;

		case event::MoveDown:
			// TODO: timeout for moving pieces around after collision
			if (!active_collides_lower()) {
				active.second.y -= 1;
				updates |= changes::Updated;

			} else if (drop_ticks == 0) {
				updates |= changes::WallHit;
				drop_ticks = 1;
			}

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
					next_pieces.push_front(hold);
				}

				hold = active.first;
				hold.regen_blocks();
				have_held = true;
				already_held = true;
				updates |= changes::Updated;

				get_new_active_tetrimino();
			}

			break;

		case event::MoveLeft:
			if (!active_collides_sides(movement::Left)) {
				active.second.x -= 1;
				updates |= changes::Updated;
			}
			
			else {
				updates |= changes::WallHit;
			}

			break;

		case event::MoveRight:
			if (!active_collides_sides(movement::Right)) {
				active.second.x += 1;
				updates |= changes::Updated;
			}

			else {
				updates |= changes::WallHit;
			}

			break;

		case event::RotateLeft:
			active.first.rotate(movement::Left);
			rotation_normalize();
			updates |= changes::Rotated | changes::Updated;
			break;

		case event::RotateRight:
			active.first.rotate(movement::Right);
			rotation_normalize();
			updates |= changes::Rotated | changes::Updated;
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

int field_state::clear_lines(void){
	int cleared = 0;

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
			cleared++;

			for (unsigned j = y; j < size.y - 1; j++) {
				field[j] = field[j + 1];
			}

			field[size.y - 1].resize(size.x);
		}

		else {
			y += 1;
		}
	}

	return cleared;
}

int field_state::color_cleared_lines(void){
	int cleared = 0;

	for (unsigned y = 0; y < size.y; y++) {
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
			cleared++;

			for (auto& block : field[y]) {
				block.state = block::states::Cleared;
			}
		}
	}

	return cleared;
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
