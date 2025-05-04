#include "direct.h"
#include <time.h>
#include <Windows.h>
#include "DITALK.H"
#ifdef _DEBUG
#  include <crtdbg.h>
#endif
#include <SDL.h>
#include "resource.h"

#define CAP_SPEED_TO_FPS 0

static int              G_done       = FALSE;
static SDL_Surface*     screen;
static SDL_Surface*     surface;
static SDL_Surface*     largesurface;
static SDL_Rect         srcrect     = { 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT };
static SDL_Rect         largesrcrect = { 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT };
static SDL_Rect         destrect     = { 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT };

extern T_void KeyboardUpdate(E_Boolean updateBuffers);

void SleepMS(T_word32 aMS)
{
    Sleep(aMS);
}

void WindowsUpdateMouse(void)
{
    int flags = 0, x, y;
    Uint8 state = SDL_GetMouseState(&x, &y);
    DirectMouseSet(x, y);
    if (state & SDL_BUTTON_LMASK) flags |= MOUSE_BUTTON_LEFT;
    if (state & SDL_BUTTON_RMASK) flags |= MOUSE_BUTTON_RIGHT;
    if (state & SDL_BUTTON_MMASK) flags |= MOUSE_BUTTON_MIDDLE;
    DirectMouseSetButton(flags);
}

void WindowsUpdateEvents(void)
{
    int flags;
    SDL_Event event;
    static int altPressed = FALSE;

    while (SDL_PollEvent(&event)) {
        switch (event.type) {
          case SDL_QUIT:
            G_done = TRUE; break;
          case SDL_KEYDOWN:
            if (event.key.keysym.sym == SDLK_ESCAPE)
              G_done = TRUE;
            else if (event.key.keysym.sym == SDLK_LALT ||
                     event.key.keysym.sym == SDLK_RALT)
              altPressed = TRUE;
            else if (event.key.keysym.sym == SDLK_RETURN && altPressed) {
                flags = screen->flags;
                screen = SDL_SetVideoMode(
                    SCREEN_WIDTH, SCREEN_HEIGHT, 32,
                    flags ^ SDL_FULLSCREEN);
                if (!screen)
                    screen = SDL_SetVideoMode(
                        SCREEN_WIDTH, SCREEN_HEIGHT, 32, flags);
                if (!screen) exit(1);
            }
            break;
          case SDL_KEYUP:
            if (event.key.keysym.sym == SDLK_LALT ||
                event.key.keysym.sym == SDLK_RALT)
              altPressed = FALSE;
            break;
          default:
            break;
        }
        SDL_GetKeyState(NULL);
    }
}

void WindowsUpdate(char *p_screen, unsigned char *palette)
{
    SDL_Color   colors[256];
    int         i, y;
    unsigned char *src = (unsigned char *)surface->pixels;
    unsigned char *dst = (unsigned char *)largesurface->pixels;

    T_word32    tick       = clock();
    static T_word32 lastTick = 0xFFFFEEEE;
    static int     fps      = 0;
    static T_word32 lastFPS = 0xFFFFEEEE;
    static double  movingAverage = 0;

#if CAP_SPEED_TO_FPS
    {
      int delay = (1000 / CAP_SPEED_TO_FPS) - (tick - lastTick);
      if (delay > 0) Sleep(delay);
    }
#endif
    lastTick = tick;

    /* palette */
    for (i = 0; i < 256; i++) {
        colors[i].r = ((palette[0] & 0x3F) << 2);
        colors[i].g = ((palette[1] & 0x3F) << 2);
        colors[i].b = ((palette[2] & 0x3F) << 2);
        palette += 3;
    }
    SDL_SetColors(largesurface, colors, 0, 256);

    /* copy scanlines 1:1 */
    for (y = 0; y < SCREEN_HEIGHT; y++) {
        memcpy(dst, src, SCREEN_WIDTH);
        src += SCREEN_WIDTH;
        dst += SCREEN_WIDTH;
    }

    SDL_BlitSurface(largesurface, &largesrcrect, screen, &destrect);
    SDL_UpdateRect(screen, 0, 0, 0, 0);

    fps++;
    if ((tick - lastFPS) >= 1000) {
        if (movingAverage < 1.0) movingAverage = fps;
        movingAverage = fps * 0.05 + movingAverage * 0.95;
        lastFPS += 1000;
        fps = 0;
    }

    WindowsUpdateEvents();
    WindowsUpdateMouse();
    KeyboardUpdate(TRUE);
}

extern T_void game_main(T_word16 argc, char *argv[]);

int SDL_main(int argc, char *argv[])
{
    char *pixels;
    int   x, y;
    SDL_Color black = {0,0,0,0}, white = {255,255,255,0};

    if (SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO) < 0) {
        printf("Could not initialize SDL: %s\n", SDL_GetError());
        return 1;
    }
    atexit(SDL_Quit);

    screen = SDL_SetVideoMode(
        SCREEN_WIDTH, SCREEN_HEIGHT, 32,
        SDL_HWSURFACE|SDL_DOUBLEBUF
#ifdef NDEBUG
      | SDL_FULLSCREEN
#endif
    );
    if (!screen) {
        printf("Could not set video mode: %s\n", SDL_GetError());
        return 1;
    }
    SDL_WM_SetCaption("Amulets & Armor", "Amulets & Armor");
    SDL_ShowCursor(SDL_DISABLE);

    surface = SDL_CreateRGBSurface(
        SDL_SWSURFACE,
        SCREEN_WIDTH, SCREEN_HEIGHT,
        8, 0,0,0,0
    );
    if (!surface) {
        printf("Could not create surface: %s\n", SDL_GetError());
        return 1;
    }

    largesurface = SDL_CreateRGBSurface(
        SDL_SWSURFACE,
        SCREEN_WIDTH, SCREEN_HEIGHT,
        8, 0,0,0,0
    );
    if (!largesurface) {
        printf("Could not create largesurface: %s\n", SDL_GetError());
        return 1;
    }

    SDL_SetColors(surface, &black, 0, 1);
    SDL_SetColors(surface, &white, 255, 1);

    pixels = (char*)surface->pixels;
    GRAPHICS_ACTUAL_SCREEN = (void*)pixels;
    for (y = 0; y < SCREEN_HEIGHT; y++) {
        for (x = 0; x < SCREEN_WIDTH; x++, pixels++) {
            *pixels = (x==0 || x==SCREEN_WIDTH-1 || y==0 || y==SCREEN_HEIGHT-1)
                      ? 255 : 0;
        }
    }

#ifndef NDEBUG
    {
      int tmp = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);
      _CrtSetDbgFlag(tmp | _CRTDBG_LEAK_CHECK_DF);
    }
#endif

    game_main(argc, argv);
    return 0;
}