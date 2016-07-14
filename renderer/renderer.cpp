#include "renderer_application.hpp"

#include <fuse/core.hpp>
#include <fuse/core/debug_ostream.hpp>

#include <iostream>
#include <fstream>

using namespace fuse;

int CALLBACK WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow)
{

	debug_ostream debugStream;
	//std::ofstream fileStream("fuse_renderer.log");

	fuse::logger logger(&debugStream);
	//fuse::logger logger(&fileStream);

	using app = fuse::application<renderer_application>;

	if (app::init(hInstance) &&
	    app::create_window(1280, 720, FUSE_LITERAL("Fuse Renderer")) &&
	    app::create_pipeline(D3D_FEATURE_LEVEL_11_0, true))
	{
		app::main_loop();
	}

	app::shutdown();

	return 0;

}