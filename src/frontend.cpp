#include <tetrode/frontend.hpp>
#include <list>
#include <string>

namespace tetrode {

void frontend::handle_event(enum event ev){
	switch (ev) {
		default: break;
	}
}

void menu::handle_event(frontend *front, event ev){
	// XXX: maps the standard key mappings to menu movements through events,
	//      probably want to change this at some point
	switch (ev) {
		case event::RotateLeft:
		case event::MoveDown:
			if (selected != entries.end()){
				selected++;
			}
			break;

		case event::RotateRight:
			if (selected != entries.begin()){
				selected--;
			}
			break;

		case event::Drop:
			(*selected)->action(front);
			break;

		default: break;
	}
}

// namespace tetrode
}
