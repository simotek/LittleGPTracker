#include "Adapters/EMSCRIPTEN/System/EMSCRIPTENSystem.h"
#include "Adapters/SDL2/GUI/SDLGUIWindowImp.h"
#include "Application/Application.h"
#include <string.h>
/*
 * generic entrypoint for linux based targets
 */
int main(int argc, char *argv[]) {
    EMSCSystem::Boot(argc, argv);

    SDLCreateWindowParams params;
    params.title = "littlegptracker";
    params.cacheFonts_ = true;

    Application::GetInstance()->Init(params);

    return EMSCSystem::MainLoop();
}

void _assert() {};
