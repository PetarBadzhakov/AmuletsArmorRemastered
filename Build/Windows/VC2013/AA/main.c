#include "direct.h"
#include <time.h>
#include <Windows.h>
#include "DITALK.H"
#ifdef _DEBUG
   #include <crtdbg.h>
#endif
#include <SDL.h>
#include "resource.h"

#define CAP_SPEED_TO_FPS       0 // 70 // 0

static int G_done = FALSE;
static SDL_Surface* screen;
static SDL_Surface* surface;
static SDL_Surface* largesurface;

static SDL_Rect srcrect = {
    0, 0,
    SCREEN_WIDTH, SCREEN_HEIGHT
};

#if (SCREEN_WIDTH > 640)
static SDL_Rect largesrcrect = {
    0, 0,
    SCREEN_WIDTH, SCREEN_HEIGHT
};
static SDL_Rect destrect = {
    0, 0,
    SCREEN_WIDTH, SCREEN_HEIGHT
};
#else
static SDL_Rect largesrcrect = {
    0, 0,
    SCREEN_WIDTH * 2, SCREEN_HEIGHT * 2
};
static SDL_Rect destrect = {
    0, 0,
    SCREEN_WIDTH * 2, SCREEN_HEIGHT * 2
};
#endif

extern T_void KeyboardUpdate(E_Boolean updateBuffers);

void SleepMS(T_word32 aMS)
{
    Sleep(aMS);
}

void WindowsUpdateMouse(void)
{
    int flags = 0;
    int x, y;
    Uint8 state;

    state = SDL_GetMouseState(&x, &y);
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
                G_done = TRUE;
                break;
            case SDL_KEYDOWN:
                if (event.key.keysym.sym == SDLK_ESCAPE) {
                    G_done = TRUE;
                } else if (event.key.keysym.sym == SDLK_LALT
                        || event.key.keysym.sym == SDLK_RALT) {
                    altPressed = TRUE;
                } else if (event.key.keysym.sym == SDLK_RETURN && altPressed) {
                    flags = screen->flags;
                    screen = SDL_SetVideoMode(0,0,0, screen->flags ^ SDL_FULLSCREEN);
                    if (!screen) screen = SDL_SetVideoMode(0,0,0, flags);
                    if (!screen) exit(1);
                }
                break;
            case SDL_KEYUP:
                if (event.key.keysym.sym == SDLK_LALT
                        || event.key.keysym.sym == SDLK_RALT) {
                    altPressed = FALSE;
                }
                break;
        }
        SDL_GetKeyState(NULL);
    }
}

#define Copy2x_4times(aDest, aSrc)                 \
    v = *((T_word32 *)aSrc);                       \
    aSrc += 4;                                     \
    aDest[0] = ((T_byte8 *)&v)[0]; aDest[1] = ((T_byte8 *)&v)[0]; \
    aDest[2] = ((T_byte8 *)&v)[1]; aDest[3] = ((T_byte8 *)&v)[1]; \
    aDest[4] = ((T_byte8 *)&v)[2]; aDest[5] = ((T_byte8 *)&v)[2]; \
    aDest[6] = ((T_byte8 *)&v)[3]; aDest[7] = ((T_byte8 *)&v)[3]; \
    aDest += 8;

#define Copy2x_20times(aDest, aSrc) \
    Copy2x_4times(aDest, aSrc)      \
    Copy2x_4times(aDest, aSrc)      \
    Copy2x_4times(aDest, aSrc)      \
    Copy2x_4times(aDest, aSrc)      \
    Copy2x_4times(aDest, aSrc)

#define Copy2x_100times(aDest, aSrc) \
    Copy2x_20times(aDest, aSrc)      \
    Copy2x_20times(aDest, aSrc)      \
    Copy2x_20times(aDest, aSrc)      \
    Copy2x_20times(aDest, aSrc)      \
    Copy2x_20times(aDest, aSrc)

#define Copy2x_320times(aDest, aSrc) \
    Copy2x_100times(aDest, aSrc)     \
    Copy2x_100times(aDest, aSrc)     \
    Copy2x_100times(aDest, aSrc)     \
    Copy2x_20times(aDest, aSrc)

void WindowsUpdate(char *p_screen, unsigned char *palette)
{
    SDL_Color colors[256];
    int i, y;
    unsigned char *src = (unsigned char *)surface->pixels;
    unsigned char *dst = (unsigned char *)largesurface->pixels;
    unsigned char *line;
    static int    lastFPS      = 0;
    static int    fps          = 0;
    T_word32      tick         = clock();
    static T_word32 lastTick    = 0xFFFFEEEE;
    static double movingAverage = 0;
#if (SCREEN_WIDTH <= 640)
    T_word32 v;
#endif
    T_word32 frac;

#if CAP_SPEED_TO_FPS
    if ((tick - lastTick) < (1000 / CAP_SPEED_TO_FPS)) {
        Sleep((1000 / CAP_SPEED_TO_FPS) - (tick - lastTick));
    } else
#endif
    {
        lastTick = tick;

        for (i = 0; i < 256; i++) {
            colors[i].r = ((palette[0] & 0x3F) << 2);
            colors[i].g = ((palette[1] & 0x3F) << 2);
            colors[i].b = ((palette[2] & 0x3F) << 2);
            colors[i].unused = 0;
            palette += 3;
        }
        SDL_SetColors(surface,      colors, 0, 256);
        SDL_SetColors(largesurface, colors, 0, 256);

#if (SCREEN_WIDTH > 640)
        line = src;
        for (y = 0, frac = 0; y < SCREEN_HEIGHT; y++, line += SCREEN_WIDTH) {
            while (frac < SCREEN_HEIGHT) {
                memcpy(dst, line, SCREEN_WIDTH);
                dst += SCREEN_WIDTH;
                frac += SCREEN_HEIGHT;
            }
            frac -= SCREEN_HEIGHT;
        }
#else
        line = src;
        for (y = 0, frac = 0; y < SCREEN_HEIGHT; y++, line += SCREEN_WIDTH) {
            while (frac < (SCREEN_HEIGHT * 2)) {
                Copy2x_320times(dst, line);
                frac += SCREEN_HEIGHT;
            }
            frac -= SCREEN_HEIGHT * 2;
        }
#endif

        if (SDL_BlitSurface(largesurface, &largesrcrect, screen, &destrect)) {
            printf("Failed blit: %s\n", SDL_GetError());
        }
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
#if CAP_SPEED_TO_FPS
        Sleep(1);
#endif
    }
}

extern T_void game_main(T_word16 argc, char *argv[]);

int SDL_main(int argc, char *argv[])
{
    char *pixels;
    int x, y;
    SDL_Color black = {0,   0,   0,   0};
    SDL_Color white = {255, 255, 255, 0};

    if (SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO) < 0) {
        printf("Could not initialize SDL: %s\n", SDL_GetError());
        return 1;
    }
    atexit(SDL_Quit);

#if (SCREEN_WIDTH > 640)
    screen = SDL_SetVideoMode(
        SCREEN_WIDTH, SCREEN_HEIGHT, 32,
        SDL_HWSURFACE|SDL_DOUBLEBUF
#ifdef NDEBUG
        | SDL_FULLSCREEN
#endif
    );
#else
    screen = SDL_SetVideoMode(
        SCREEN_WIDTH*2, SCREEN_HEIGHT*2, 32,
        SDL_HWSURFACE|SDL_DOUBLEBUF
#ifdef NDEBUG
        | SDL_FULLSCREEN
#endif
    );
#endif

    SDL_WM_SetCaption("Amulets & Armor", "Amulets & Armor");
    SDL_ShowCursor(SDL_DISABLE);

    if (!screen) {
        printf("Could not set video mode: %s\n", SDL_GetError());
        return 1;
    }

    surface = SDL_CreateRGBSurface(
        SDL_SWSURFACE, SCREEN_WIDTH, SCREEN_HEIGHT, 8,
        0, 0, 0, 0
    );
    if (!surface) {
        printf("Could not create overlay: %s\n", SDL_GetError());
        return 1;
    }

#if (SCREEN_WIDTH > 640)
    largesurface = SDL_CreateRGBSurface(
        SDL_SWSURFACE,
        SCREEN_WIDTH, SCREEN_HEIGHT, 8,
        0, 0, 0, 0
    );
#else
    largesurface = SDL_CreateRGBSurface(
        SDL_SWSURFACE,
        SCREEN_WIDTH*2, SCREEN_HEIGHT*2, 8,
        0, 0, 0, 0
    );
#endif
    if (!largesurface) {
        printf("Could not create overlay: %s\n", SDL_GetError());
        return 1;
    }

    SDL_SetColors(surface, &black, 0, 1);
    SDL_SetColors(surface, &white, 255, 1);

    pixels = (char*)surface->pixels;
    GRAPHICS_ACTUAL_SCREEN = pixels;
    for (y = 0; y < SCREEN_HEIGHT; y++) {
        for (x = 0; x < SCREEN_WIDTH; x++, pixels++) {
            if (x == 0 || x == SCREEN_WIDTH-1 ||
                y == 0 || y == SCREEN_HEIGHT-1)
                *pixels = 255;
            else
                *pixels = 0;
        }
    }

#ifndef NDEBUG
    {
        int tmpFlag = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);
        tmpFlag |= _CRTDBG_LEAK_CHECK_DF;
        _CrtSetDbgFlag(tmpFlag);
    }
#endif

    game_main(argc, argv);
    return 0;
}