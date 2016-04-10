#define _BSD_SOURCE 1
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#include <sys/time.h>

#include "plugin.h"

#include "SDL.h"
#include "SDL_image.h"

#include "common.h"
#include "device.h"
#include "sim.h"

#define SDLLED_UPDATE_HZ 30
#define SDLLED_BASE (0x100)

#define DIGIT_COUNT     4
#define DIGIT_WIDTH     38
#define DOT_WIDTH       6
#define DIGIT_HEIGHT    64
#define RESOURCE_DIR    "rsrc"

#define PUMP_CYCLES 2048

struct sdlled_state {
    uint32_t data[2];
    SDL_Window *window;
    SDL_Renderer *renderer;
    enum { RUNNING, STOPPING, STOPPED } status;
    SDL_Texture *digits[16];
    SDL_Texture *dots[2];
    struct timeval last_update, deadline;
    int cycles;
};

static int handle_update(struct sdlled_state *state);

static int put_digit(struct sdlled_state *state, unsigned index, unsigned digit)
{
    SDL_Rect src = { .w = DIGIT_WIDTH, .h = DIGIT_HEIGHT },
             dst = {
                 .x = index * (DIGIT_WIDTH + DOT_WIDTH),
                 .w = DIGIT_WIDTH,
                 .h = DIGIT_HEIGHT
             };

    if (digit > 15)
        return -1;

    SDL_Texture *num = state->digits[digit];
    if (!num) {
        char filename[1024];
        snprintf(filename, sizeof filename, RESOURCE_DIR "/%d/%c.png",
                DIGIT_HEIGHT, "0123456789ABCDEF"[digit]);

        SDL_Surface *surf = IMG_Load(filename);
        num = state->digits[digit] = SDL_CreateTextureFromSurface(state->renderer, surf);
        SDL_FreeSurface(surf);
    }

    SDL_RenderCopy(state->renderer, num, &src, &dst);

    return 0;
}

static int put_dot(struct sdlled_state *state, unsigned index, unsigned on)
{
    SDL_Rect src = { .w = DOT_WIDTH, .h = DIGIT_HEIGHT },
             dst = {
                 .x = (index + 1) * DIGIT_WIDTH + index * DOT_WIDTH,
                 .w = DOT_WIDTH,
                 .h = DIGIT_HEIGHT
             };

    SDL_Texture *dot = state->dots[on];
    if (!dot) {
        char filename[1024];
        snprintf(filename, sizeof filename, RESOURCE_DIR "/%d/dot_%s.png",
                DIGIT_HEIGHT, on ? "on" : "off");

        SDL_Surface *surf = IMG_Load(filename);
        dot = state->dots[on] = SDL_CreateTextureFromSurface(state->renderer, surf);
        SDL_FreeSurface(surf);
    }

    SDL_RenderCopy(state->renderer, dot, &src, &dst);

    return 0;
}

static void decode_led(uint32_t data, int digits[4])
{
    for (unsigned i = 0; i < 4; i++)
        digits[i] = (data >> (i * 4)) & 0xf;
}

static void decode_dots(uint32_t data, int dots[4])
{
    for (unsigned i = 0; i < 4; i++)
        dots[i] = (data >> i) & 1;
}

static int sdlled_init(struct plugin_cookie *pcookie, void *cookie)
{
    struct sdlled_state *state = *(void**)cookie;

    if (!state)
        state = *(void**)cookie = malloc(sizeof *state);

    *state = (struct sdlled_state){ .status = RUNNING };

    if (SDL_Init(SDL_INIT_VIDEO) < 0)
        fatal(0, "Unable to init SDL: %s", SDL_GetError());

    state->window = SDL_CreateWindow("sdlled",
            0, 0, DIGIT_COUNT * (DIGIT_WIDTH + DOT_WIDTH), DIGIT_HEIGHT, SDL_WINDOW_SHOWN);

    if (!state->window)
        fatal(0, "Unable to set up LED surface : %s", SDL_GetError());

    state->renderer = SDL_CreateRenderer(state->window, -1, 0);

    int flags = IMG_INIT_PNG;
    if (IMG_Init(flags) != flags)
        fatal(0, "sdlled failed to initialise SDL_Image");

    gettimeofday(&state->last_update, NULL);
    state->deadline = state->last_update;
    handle_update(state);

    return 0;
}

static int sdlled_fini(void *cookie)
{
    struct sdlled_state *state = cookie;

    for (unsigned i = 0; i < 16; i++)
        SDL_DestroyTexture(state->digits[i]);

    SDL_DestroyTexture(state->dots[0]);
    SDL_DestroyTexture(state->dots[1]);

    SDL_DestroyRenderer(state->renderer);
    SDL_DestroyWindow(state->window);
    free(state);
    // Can't immediately call SDL_Quit() in case others are using it
    atexit(SDL_Quit);
    atexit(IMG_Quit);

    return 0;
}

static int handle_update(struct sdlled_state *state)
{
    int digits[4];
    int dots[4];

    decode_led(state->data[0], digits);
    decode_dots(state->data[1], dots);

    debug(5, "sdlled : %x%c%x%c%x%c%x%c\n",
             digits[3], dots[3] ? '.' : ' ',
             digits[2], dots[2] ? '.' : ' ',
             digits[1], dots[1] ? '.' : ' ',
             digits[0], dots[0] ? '.' : ' ');

    for (unsigned i = 0; i < 4; i++) {
        put_digit(state, i, digits[3 - i]);
        put_dot(state, i, dots[3 - i]);
    }

    // do periodic updates only, not as fast as we write to the display
    // (both faster to render, and more like real life)
    struct timeval now, tick = { .tv_usec = 1000000 / SDLLED_UPDATE_HZ };
    gettimeofday(&now, NULL);
    // TODO this could get lagged behind
    if (timercmp(&now, &state->deadline, >)) {
        SDL_RenderPresent(state->renderer);
        state->last_update = state->deadline;
        timeradd(&state->deadline, &tick, &state->deadline);
    }

    return 0;
}

static int sdlled_op(void *cookie, int op, uint32_t addr, uint32_t *data)
{
    struct sdlled_state *state = cookie;

    if (op == OP_WRITE) {
        state->data[addr - SDLLED_BASE] = *data;
        handle_update(state);
    } else if (op == OP_DATA_READ) {
        int32_t off = addr - SDLLED_BASE;
        switch (off) {
            case 0: *data = state->data[off] & 0x0000ffff; break;
            case 1: *data = state->data[off] & 0x0000000f; break;
            default: return 1;
        }
    }

    return 0;
}

static int sdlled_pump(void *cookie)
{
    struct sdlled_state *state = cookie;

    SDL_Event event;
    if (state->cycles++ % PUMP_CYCLES == 0 && state->status == RUNNING) {
        if (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_QUIT:
                    state->status = STOPPED;
                    debug(0, "sdlled requested quit");
                    exit(0);
                default:
                    break;
            }
        }
    }

    return 0;
}

void EXPORT tenyr_plugin_init(struct guest_ops *ops)
{
    fatal_ = ops->fatal;
    debug_ = ops->debug;
}

int EXPORT sdlled_add_device(struct device **device)
{
    **device = (struct device){
        .bounds = { SDLLED_BASE, SDLLED_BASE + 1 },
        .ops = {
            .op = sdlled_op,
            .init = sdlled_init,
            .fini = sdlled_fini,
            .cycle = sdlled_pump,
        },
    };

    return 0;
}

/* vi: set ts=4 sw=4 et: */
