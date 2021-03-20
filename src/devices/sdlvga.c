#define _BSD_SOURCE 1

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#include <sys/time.h>

#include "plugin.h"

#include "SDL.h"
#include "SDL_image.h"

#include "os_common.h"
#include "common.h"
#include "param.h"
#include "device.h"
#include "sim.h"

#define SDLVGA_UPDATE_HZ 60
#define SDLVGA_BASE (1L << 16)

#define RESOURCE_DIR    "rsrc"

#define ROWS 32
#define COLS 64
#define FONT_HEIGHT 15
#define FONT_WIDTH  10

#define PUMP_CYCLES 2048

library_init tenyr_plugin_init;
device_adder sdlvga_add_device;

struct sdlvga_state {
    int32_t data[ROWS][COLS];
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Texture *display;
    SDL_Texture *sprite;
    struct timeval last_update, deadline;
    enum { RUNNING, STOPPING, STOPPED } status;
    int cycles;
};

static int put_character(struct sdlvga_state *state, int row,
        int col, unsigned char ch)
{
    SDL_Rect src = {
                .x = ch * FONT_WIDTH,
                .y = 0,
                .w = FONT_WIDTH,
                .h = FONT_HEIGHT
             },
             dst = {
                 .x = col * FONT_WIDTH,
                 .y = row * FONT_HEIGHT,
                 .w = FONT_WIDTH,
                 .h = FONT_HEIGHT
             };

    SDL_RenderCopy(state->renderer, state->sprite, &src, &dst);

    return 0;
}

static int sdlvga_init(struct plugin_cookie *pcookie, struct device *device, void *cookie)
{
    struct sdlvga_state *state = *(void**)cookie;

    if (!state)
        state = *(void**)cookie = malloc(sizeof *state);

    *state = (struct sdlvga_state){ .status = RUNNING };

    if (SDL_Init(SDL_INIT_VIDEO) < 0)
        fatal(0, "Unable to init SDL: %s", SDL_GetError());

    state->window = SDL_CreateWindow("sdlvga",
                        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                        COLS * FONT_WIDTH, ROWS * FONT_HEIGHT,
                        SDL_WINDOW_SHOWN);
    if (!state->window)
        fatal(0, "Unable to create sdlvga window : %s", SDL_GetError());

    state->renderer = SDL_CreateRenderer(state->window, -1,
            SDL_RENDERER_ACCELERATED | SDL_RENDERER_TARGETTEXTURE);

    int flags = IMG_INIT_PNG;
    if (IMG_Init(flags) != flags)
        fatal(0, "sdlvga failed to initialise SDL_Image : %s", IMG_GetError());

    const char *share_path = ".";
    // If the param_get fails, we'll check the current directory
    pcookie->gops.param_get(pcookie, "paths.share", 1, (void*)&share_path);
    char *filename = build_path(share_path, RESOURCE_DIR"/font10x15/invert.font10x15.png");
    SDL_Surface *sprite = IMG_Load(filename);
    if (!sprite)
        fatal(0, "sdlvga failed to load font sprite `%s' : %s", filename, IMG_GetError());

    free(filename);

    state->display = SDL_CreateTexture(state->renderer,
            SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, COLS *
            FONT_WIDTH, ROWS * FONT_HEIGHT);

    state->sprite = SDL_CreateTextureFromSurface(state->renderer, sprite);
    SDL_FreeSurface(sprite);

    SDL_SetRenderDrawColor(state->renderer, 0, 0, 0, 255);
    SDL_RenderClear(state->renderer);
    SDL_RenderPresent(state->renderer);
    SDL_SetRenderTarget(state->renderer, state->display);

    gettimeofday(&state->last_update, NULL);
    state->deadline = state->last_update;

    return 0;
}

static int sdlvga_fini(void *cookie)
{
    struct sdlvga_state *state = cookie;

    SDL_DestroyRenderer(state->renderer);
    SDL_DestroyTexture(state->sprite);
    SDL_DestroyTexture(state->display);
    SDL_DestroyWindow(state->window);

    free(state);
    // Can't immediately call SDL_Quit() in case others are using it
    atexit(SDL_Quit);
    atexit(IMG_Quit);

    return 0;
}

static int handle_update(struct sdlvga_state *state)
{
    // do periodic updates only, not as fast as we write to the display
    // (both faster to render, and more like real life)
    struct timeval now, tick = { .tv_usec = 1000000 / SDLVGA_UPDATE_HZ };
    gettimeofday(&now, NULL);
    // TODO this could get lagged behind
    if (timercmp(&now, &state->deadline, >)) {
        SDL_SetRenderTarget(state->renderer, NULL);
        SDL_RenderCopy(state->renderer, state->display, NULL, NULL);
        SDL_RenderPresent(state->renderer);
        SDL_SetRenderTarget(state->renderer, state->display);
        state->last_update = state->deadline;
        timeradd(&state->deadline, &tick, &state->deadline);
    }

    return 0;
}

static int sdlvga_op(void *cookie, int op, int32_t addr, int32_t *data)
{
    struct sdlvga_state *state = cookie;

    // TODO handle control settings
    int32_t offset = addr - SDLVGA_BASE;
    if (offset >= 0 && offset < ROWS * COLS) {
        int row = offset / COLS;
        int col = offset % COLS;

        if (op == OP_WRITE) {
            state->data[row][col] = *data;
            put_character(state, row, col, *data & 0xff);
            handle_update(state);
        } else if (op == OP_DATA_READ) {
            *data = state->data[row][col];
        }
    }

    return 0;
}

static int sdlvga_pump(void *cookie)
{
    struct sdlvga_state *state = cookie;

    SDL_Event event;
    if (state->cycles++ % PUMP_CYCLES == 0 && state->status == RUNNING) {
        if (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_QUIT:
                    state->status = STOPPED;
                    exit(0);
                default:
                    break;
            }
        }
    }

    return 0;
}

int EXPORT tenyr_plugin_init(struct guest_ops *ops)
{
    fatal_ = ops->fatal;
    debug_ = ops->debug;

    return 0;
}

int EXPORT sdlvga_add_device(struct device *device)
{
    *device = (struct device){
        .bounds = { SDLVGA_BASE, SDLVGA_BASE + 0x1000 - 1 },
        .ops = {
            .op = sdlvga_op,
            .init = sdlvga_init,
            .fini = sdlvga_fini,
            .cycle = sdlvga_pump,
        },
    };

    return 0;
}

/* vi:set et ts=4 sw=4: */

