#include "Event.h"

#include <memory>

using namespace osc;

std::unique_ptr<Event> osc::parse_into_event(const SDL_Event& e)
{
	return std::make_unique<RawEvent>(e);
}