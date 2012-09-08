#include "plugin.h"

#include "spi.h"
#include "common.h"
// sim.h is used for breakpoint() but this possibly should be factored
// differently
#include "sim.h"

#include <assert.h>
#include <stdlib.h>
#include <stdint.h>

#define RESET_CYCLE_REQ 50  ///< arbitrary

struct spisd_state {
    enum {
        SPISD_INVALID = 0,
        SPISD_UNINITIALISED,
        SPISD_IDLE,             ///< also means "in the process of initialising"
        SPISD_READY,
    } state;
    uint64_t shift_in,
             shift_out;
    int out_shift_len;  ///< length of output shift register
    int bit_count;       ///< bits received in this select
    unsigned long cycle_count;  ///< monotonically increasing
    unsigned long last_reset;   ///< cycle count of idle finish

	const struct guest_ops *gops;
	void *hostcookie;
};

enum spisd_command_type {
    GO_IDLE_STATE           =  0,
    SEND_OP_COND            =  1,

    SEND_CSD                =  9,
    SEND_CID                = 10,

    STOP_TRANSMISSION       = 12,
    SEND_STATUS             = 13,

    SET_BLOCKLEN            = 16,
    READ_SINGLE_BLOCK       = 17,
    READ_MULTIPLE_BLOCK     = 18,


    WRITE_BLOCK             = 24,
    WRITE_MULTIPLE_BLOCK    = 25,

    PROGRAM_CSD             = 27,
  //SET_WRITE_PROT          = 28,
  //CLR_WRITE_PROT          = 29,
  //SEND_WRITE_PROT         = 30,

    ERASE_WR_BLK_START_ADDR = 32,
    ERASE_WR_BLK_END_ADDR   = 33,

    ERASE                   = 38,

    APP_CMD                 = 55,
    GEN_CMD                 = 56,

    READ_OCR                = 58,
    CRC_ON_OFF              = 59,
};

enum spisd_app_command_type {
    APP_SD_STATUS               = 13,

    APP_SEND_NUM_WR_BLOCKS      = 22,
    APP_SET_WR_BLK_ERASE_COUNT  = 23,

    APP_SEND_OP_COND            = 41,
    APP_SET_CLR_CARD_DETECT     = 42,

    APP_SEND_SCR                = 51,
};

#define spisd_rsp_R1_minbytes 1
#define spisd_rsp_R1_maxbytes 1
struct spisd_rsp_R1 {
    unsigned idle           :1;
    unsigned erase_reset    :1;
    unsigned illegal        :1;
    unsigned crc_error      :1;
    unsigned erase_seq_error:1;
    unsigned address_error  :1;
    unsigned parameter_error:1;

    unsigned must_be_zero   :1;
};

#define spisd_rsp_R1b_minbytes 1
#define spisd_rsp_R1b_maxbytes -1
struct spisd_rsp_R1b {
    // this is an embedded R1
    // TODO replace with an actual struct spisd_rsp_R1 ?
    unsigned idle           :1;
    unsigned erase_reset    :1;
    unsigned illegal        :1;
    unsigned crc_error      :1;
    unsigned erase_seq_error:1;
    unsigned address_error  :1;
    unsigned parameter_error:1;

    unsigned must_be_zero   :1;

    uint8_t busy[];
};

#define spisd_rsp_R2_minbytes 2
#define spisd_rsp_R2_maxbytes 2
struct spisd_rsp_R2 {
    unsigned locked         :1;
    unsigned wp_erase_skip  :1;
    unsigned error          :1;
    unsigned cc_error       :1;
    unsigned card_ecc_fail  :1;
    unsigned wp_violation   :1;
    unsigned erase_param    :1;
    unsigned out_of_range   :1;

    // this is an embedded R1
    // TODO replace with an actual struct spisd_rsp_R1 ?
    unsigned idle           :1;
    unsigned erase_reset    :1;
    unsigned illegal        :1;
    unsigned crc_error      :1;
    unsigned erase_seq_error:1;
    unsigned address_error  :1;
    unsigned parameter_error:1;

    unsigned must_be_zero   :1;
};

// TODO fill in
#define SPISD_COMMANDS(_) \
    _(GO_IDLE_STATE, R1 , spisd_go_idle_handler) \
    _(SEND_OP_COND , R1 , spisd_send_op_handler) \
    //

// TODO fill in
#define SPISD_APP_COMMANDS(_) \
    _(APP_SD_STATUS, R2 , spisd_unimp_app_handler) \
    //

#define SPISD_ARRAY_ENTRY(Type,Resp,Handler) \
    [Type] = { Type, spisd_rsp_##Resp##_minbytes, spisd_rsp_##Resp##_maxbytes, Handler },

#define SPISD_APP_ARRAY_ENTRY(Type,Resp,Handler) \
    [Type] = { Type, spisd_rsp_##Resp##_minbytes, spisd_rsp_##Resp##_maxbytes, Handler },

typedef int spisd_handler(struct spisd_state *s, enum spisd_command_type type, uint32_t arg, uint8_t crc);
typedef int spisd_app_handler(struct spisd_state *s, enum spisd_command_type type, uint32_t arg, uint8_t crc);

static UNUSED int spisd_unimp_handler(struct spisd_state *s, enum spisd_command_type type, uint32_t arg, uint8_t crc)
{
    // no action
    return 0;
}

static UNUSED int spisd_unimp_app_handler(struct spisd_state *s, enum spisd_command_type type, uint32_t arg, uint8_t crc)
{
    // no action
    return 0;
}

static int spisd_go_idle_handler(struct spisd_state *s, enum spisd_command_type type, uint32_t arg, uint8_t crc)
{
    struct spisd_rsp_R1 rsp = { .idle = 0 };

    // Only the first command needs to have its CRC set properly, as long as
    // the user never enables CRC. Once the SD card enters SPI mode CRC
    // checking is off. The CRC for CMD0 is fixed at 0x4a.
    if (crc == 0x4a || s->state != SPISD_UNINITIALISED) {
        s->state = SPISD_IDLE;
        s->last_reset = s->cycle_count;
        rsp.idle = 1;
    } else {
        s->state = SPISD_UNINITIALISED;
        rsp.idle = 0;
        rsp.crc_error = 1;
    }

    // type punning workaround
    s->shift_out = *(int*)*(void*[]){ &rsp };
    s->out_shift_len = 8 * spisd_rsp_R1_minbytes;

    return 0;
}

static int spisd_send_op_handler(struct spisd_state *s, enum spisd_command_type type, uint32_t arg, uint8_t crc)
{
    int ready = -1;
    switch (s->state) {
        case SPISD_READY:
        default:
            ready = 1;
            break;

        case SPISD_IDLE:
        case SPISD_UNINITIALISED:
        case SPISD_INVALID:
            ready = 0;
            break;
    }

    assert(("ready bit determined fully by state", ready == 0 || ready == 1));

    struct spisd_rsp_R1 rsp = { .idle = !ready };

    // type punning workaround
    s->shift_out = *(int*)*(void*[]){ &rsp };
    s->out_shift_len = 8 * spisd_rsp_R1_minbytes;

    return 0;
}

static int spisd_cycle_handler(struct spisd_state *s, enum spisd_command_type type, uint32_t arg, uint8_t crc)
{
    int ready = s->cycle_count - s->last_reset > RESET_CYCLE_REQ;
    if (s->state == SPISD_IDLE && ready) {
        s->state = SPISD_READY;
    }

    return 0;
}

static const struct spisd_command {
    enum spisd_command_type type;
    int rsp_minbytes, rsp_maxbytes;
    spisd_handler *handler;
} spisd_commands[64] = {
    SPISD_COMMANDS(SPISD_ARRAY_ENTRY)
};

static const struct spisd_app_command {
    enum spisd_app_command_type type;
    int rsp_minbytes, rsp_maxbytes;
    spisd_app_handler *handler;
} spisd_app_commands[64] = {
    SPISD_APP_COMMANDS(SPISD_APP_ARRAY_ENTRY)
};

int EXPORT spisd_spi_init(void *pcookie, const struct guest_ops *gops, void *hostcookie)
{
    struct spisd_state *s = calloc(1, sizeof *s);
    *(void**)pcookie = s;
    s->out_shift_len = 0;
    s->state = SPISD_UNINITIALISED;
	s->gops = gops;
	s->hostcookie = hostcookie;

    return 0;
}

int EXPORT spisd_spi_select(void *cookie, int _ss)
{
    struct spisd_state *s = cookie;

    if (_ss)
        s->bit_count = 0;

    return 0;
}

int EXPORT spisd_spi_clock(void *cookie, int _ss, int in, int *out)
{
    struct spisd_state *s = cookie;
    s->cycle_count++;

    if (!_ss)
        return 0;

    // MSB first
    s->shift_in <<= 1;
    s->shift_in |= in;
    s->bit_count++;
    *out = !!(s->shift_out & (1ull << (s->out_shift_len - 1)));
    s->shift_out <<= 1;

    if (s->bit_count == 48) {
        if (s->shift_in & (1ull << 47))
            breakpoint("forced-zero bit in SPI command at bit position 47 is nonzero");

        if (!(s->shift_in & (1ull << 46)))
            breakpoint("forced-one bit in SPI command at bit position 46 is zero");

        if (!(s->shift_in & (1ull << 0)))
            breakpoint("forced-one bit in SPI command at bit position 0 is zero");

        enum spisd_command_type type = (s->shift_in >> 40) & 0x1f;
        uint32_t arg = (s->shift_in >> 8) & 0xffffffff;
        uint8_t crc = (s->shift_in >> 1) & 0x7f;

        const struct spisd_command *c = &spisd_commands[type];
        spisd_cycle_handler(s, type, arg, crc);
        if (c->handler)
            c->handler(s, type, arg, crc);

        s->bit_count = 0;
    }

    return 0;
}

int EXPORT spisd_spi_fini(void *cookie)
{
    struct spisd_state *s = *(void**)cookie;
    free(s);
    *(void**)cookie = NULL;

    return 0;
}

// ---

void EXPORT tenyr_plugin_init(struct guest_ops *ops)
{
    fatal_ = ops->fatal;
    debug_ = ops->debug;
}

