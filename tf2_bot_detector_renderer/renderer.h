
#if BD_RENDERER_MODE == OPENGL
#include "sdl2opengl.h"
using TF2BDRenderer = TF2BotDetectorSDLRenderer;
#elif BD_RENDERER_MODE == DX9 && _WIN32 
#else
#error "Invalid Renderer mode!"
#endif
