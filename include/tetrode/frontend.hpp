#pragma once
#include <tetrode/field_state.hpp>

namespace tetrode {

class frontend {
	public:
		void handle_event(enum event ev);
		virtual int run(void){ return -1; };
		field_state field = field_state();
};

// namespace tetrode
}
