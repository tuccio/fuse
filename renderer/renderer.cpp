#include "renderer_application.hpp"

#include <fuse/debug_ostream.hpp>

#include <iostream>

int CALLBACK WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow)
{

	AllocConsole();

	freopen("CONOUT$", "w", stdout);
	freopen("CONOUT$", "w", stderr);

	debug_ostream debugStream;

	fuse::logger logger(&debugStream);

	using app = fuse::application<renderer_application>;

	if (app::init(hInstance) &&
	    app::create_window(1280, 720, "Sample") &&
	    app::create_pipeline(D3D_FEATURE_LEVEL_11_0, true))
	{
		app::main_loop();
	}

	app::shutdown();

	return 0;

}