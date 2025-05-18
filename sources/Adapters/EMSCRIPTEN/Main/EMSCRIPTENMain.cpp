#include "Adapters/EMSCRIPTEN/System/EMSCRIPTENSystem.h"
#include "Adapters/SDL2/GUI/SDLGUIWindowImp.h"
#include "Application/Application.h"
#include <string.h>
#include <emscripten.h>

static void mainloop(void)  {
	EMSCSystem::MainLoop();
}
/*
 * generic entrypoint for linux based targets
 */
int main(int argc, char *argv[]) {
    EMSCSystem::Boot(argc, argv);

    SDLCreateWindowParams params;
    params.title = "littlegptracker";
    params.cacheFonts_ = true;

    printf("foo");

    Application::GetInstance()->Init(params);

	emscripten_set_main_loop(mainloop, 0, 1);
    return 0;
}

void _assert() {};
