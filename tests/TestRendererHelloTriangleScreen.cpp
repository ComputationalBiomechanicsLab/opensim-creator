#include "src/Screens/RendererHelloTriangleScreen.hpp"
#include "src/Platform/App.hpp"

#include <gtest/gtest.h>

// this flag disables/enables tests that are here to aid development, such as tests
// that set up a manual testing environment.
static constexpr bool g_DisableTheseTests = true;

TEST(RendererHelloTriangleScreen, BootDirectlyIntoScreen)
{
	if (g_DisableTheseTests)
	{
		return;
	}

	osc::App app;
	app.show<osc::RendererHelloTriangleScreen>();
}