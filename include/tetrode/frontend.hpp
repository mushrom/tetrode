#pragma once

#include <tetrode/field_state.hpp>
#include <list>
#include <string>
#include <cstdio>

namespace tetrode {

class frontend;
class menu;

class menu {
	public:
		class entry {
			public:
				std::string text;
				virtual void action(frontend *front){ /* do stuff here */ };
		};

		void handle_event(frontend *front, event ev);
		std::list<entry*> entries;
		// XXX: this must be set by the subclass, otherwise things will be borked!
		std::list<entry*>::iterator selected;
};

class frontend {
	public:
		void handle_event(enum event ev);
		virtual int run(void){ return -1; };
		field_state field = field_state();

		std::list<menu> menus;
		bool paused = true;
};

class main_menu : public menu {
	public:
		class main_entry : public menu::entry {
			public:
				main_entry(std::string s){ text = s; };
				virtual void action(frontend *front){
					puts("Got here");

					front->menus.pop_back();
					front->paused = false;
				}
		};

		main_menu() {
			for (auto& x : {"Marathon", "Practice", "Multiplayer", "Settings"}) {
				entries.push_back(new main_entry(x));
			}

			selected = entries.begin();
		}

		/*entries = {
			new main_entry("testing"),
			new main_entry("this"),
			new main_entry("thing"),
		};
		*/
};

// namespace tetrode
}
