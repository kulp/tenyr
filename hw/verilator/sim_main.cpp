// Verilated testbench for the tenyr CPU core.
//
// Runs tenyr code through the Verilog implementation with:
// - Serial text output via simserial.v
// - Verbosity levels (-v, -vv, -vvv, -vvvv) matching tsim
// - Disassembly and register dump using the same code as tsim (asm.c)
// - Binary loading from .texe (obj) files

#include "Vtop.h"
#include "Vtop___024root.h"
#include "verilated.h"

#include <cstdbool>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

extern "C" {

#include "tenyr_config.h"
#include "common.h"
#include "asm.h"
#include "stream.h"
#include <getopt.h>

} // extern "C"

static vluint64_t main_time = 0;

double sc_time_stamp() {
    return static_cast<double>(main_time);
}

// --- Configuration ---

// From tenyr.v
static const uint32_t RAM_BASE = 0x1000;
static const int RAM_SIZE = 8192;

static int verbose = 0;
static const char *load_file = nullptr;
static int32_t load_addr = RAM_BASE;      // --address=N, base for loading the image
static int32_t start_addr = RAM_BASE;     // --start=N, initial program counter
static vluint64_t max_periods = 0x10000000ULL;

// State machine states (Core localparams: s0=0 ... s7=7)
static const int S0 = 0;  // execute
static const int S3 = 3;  // data memory access
static const int S6 = 6;  // latch instruction

// --- RAM access ---

static inline void ram_write(Vtop &top, uint32_t addr, uint32_t word) {
    if (addr >= RAM_BASE && addr < RAM_BASE + RAM_SIZE)
        top.rootp->Tenyr__DOT__ram__DOT__store[addr - RAM_BASE] = word;
}

static inline uint32_t ram_read(Vtop &top, uint32_t addr) {
    if (addr >= RAM_BASE && addr < RAM_BASE + RAM_SIZE)
        return top.rootp->Tenyr__DOT__ram__DOT__store[addr - RAM_BASE];
    return 0xffffffff;  // default slave
}

// --- Memory access tracing (matching tsim's dispatch_op at verbose > 2) ---

/// Buffer for a data memory operation observed at S3.
/// -1: none, 0: read, 1: write
static int mem_op = -1;
static uint32_t mem_addr = 0, mem_data = 0;

/// True after the $finish instruction (0xffffffff) is detected at S0, so the
/// main loop can let it execute through S3 and print its data memory access
/// before halting (matching tsim, which prints the $finish data read before
/// the simulation ends).
static bool finish_pending = false;

/// Print a single memory access line, matching tsim's dispatch_op format
/// (verbose > 2).
static inline void trace_memaccess(int is_write, uint32_t addr, uint32_t data)
{
    if (verbose > 2)
        fprintf(stderr, "%-5s @ 0x%08x = 0x%08x\n",
                is_write ? "write" : "read", addr, data);
}

static int32_t read_sword_le(FILE *f) {
    uint32_t val = 0;
    if (fread(&val, 1, 4, f) != 4) return 0;
    return static_cast<int32_t>(val);
}

// --- .to (obj) file loader ---

static void load_tofile(Vtop &top, const char *filename) {
    FILE *f = fopen(filename, "rb");
    if (!f) { fprintf(stderr, "vsim: cannot open %s\n", filename); exit(1); }

    char magic[3];
    if (fread(magic, 1, 3, f) != 3 || memcmp(magic, "TOV", 3) != 0) {
        fprintf(stderr, "vsim: %s: bad magic (expected TOV)\n", filename);
        exit(1);
    }
    uint8_t version; fread(&version, 1, 1, f);
    read_sword_le(f);  // flags
    int32_t rec_count = read_sword_le(f);

    for (int32_t r = 0; r < rec_count; r++) {
        int32_t addr = read_sword_le(f);
        int32_t size = read_sword_le(f);
        for (int32_t i = 0; i < size; i++) {
            uint32_t word = (uint32_t)read_sword_le(f);
            uint32_t ram_addr = (uint32_t)(addr + load_addr + i);
            trace_memaccess(1, ram_addr, word);
            ram_write(top, ram_addr, word);
            // Verification read (matching tsim's load_sim)
            trace_memaccess(0, ram_addr, ram_read(top, ram_addr));
        }
    }

    // Skip symbols
    int32_t sym_count = read_sword_le(f);
    for (int32_t s = 0; s < sym_count; s++) {
        read_sword_le(f); int32_t name_len = read_sword_le(f);
        fseek(f, (name_len + 3) & ~3, SEEK_CUR);
        read_sword_le(f); read_sword_le(f);
    }
    // Skip relocations
    int32_t rlc_count = read_sword_le(f);
    for (int32_t r = 0; r < rlc_count; r++) {
        read_sword_le(f); int32_t name_len = read_sword_le(f);
        fseek(f, (name_len + 3) & ~3, SEEK_CUR);
        read_sword_le(f); read_sword_le(f); read_sword_le(f);
    }
    fclose(f);
}

// --- Register dump and trace (matching tsim's pre_insn + dispatch_op) ---

static void trace_instruction(Vtop &top, uint32_t insn_word, uint32_t pc) {
    // Print any buffered data memory operation from the previous instruction's
    // S3 stage (matching tsim, where dispatch_op for OP_DATA_READ/OP_WRITE is
    // called during run_instruction, which occurs after pre_insn of the
    // current instruction but before pre_insn of the next).
    if (mem_op >= 0) {
        trace_memaccess(mem_op == 1, mem_addr, mem_data);
        mem_op = -1;
    }

    // Print instruction fetch read (matching tsim's dispatch_op for OP_INSN_READ,
    // which is called before pre_insn in interp_step_sim).
    trace_memaccess(0, pc, insn_word);

    // Build an element struct for the disassembler
    struct element elem = {};
    elem.insn.u.word = static_cast<int32_t>(insn_word);
    elem.insn.reladdr = static_cast<int32_t>(pc);
    elem.insn.size = 1;

    struct stream stream = stream_make_from_file(stderr);

    if (verbose > 0)
        fprintf(stderr, "IP = 0x%08x\t", pc);

    if (verbose > 1) {
        int len = print_disassembly(&stream, &elem, ASM_AS_INSN);
        fprintf(stderr, "%*s# ", 30 - len, "");
        print_disassembly(&stream, &elem, ASM_AS_DATA);
    }

    if (verbose > 3) {
        fprintf(stderr, "\n");

        // Collect 16 registers: A-O from Verilog store, P from nextP
        int32_t regs[16];
        for (int i = 0; i < 15; i++)
            regs[i] = (int32_t)top.rootp->Tenyr__DOT__core__DOT__regs__DOT__store[i];
        regs[15] = (int32_t)(top.rootp->Tenyr__DOT__core__DOT__nextP - 1);

        print_registers(&stream, regs);
    }

    if (verbose > 0)
        fprintf(stderr, "\n");
}

// --- Command-line interface (a subset of tsim's) ---

static const char shortopts[] = "as:vhV";

static const struct option longopts[] = {
    { "address" , required_argument, NULL, 'a' },
    { "start"   , required_argument, NULL, 's' },
    { "verbose" ,       no_argument, NULL, 'v' },
    { "help"    ,       no_argument, NULL, 'h' },
    { "version" ,       no_argument, NULL, 'V' },
    { NULL, 0, NULL, 0 },
};

static const char *version(void)
{
    return "vsim version " BUILD_NAME " built " __DATE__;
}

static void usage(const char *me)
{
    printf("Usage: %s [ OPTIONS ] image-file\n"
           "Options:\n"
           "  -a, --address=N       load instructions into memory at word address N\n"
           "  -s, --start=N         start execution at word address N\n"
           "  -v, --verbose         increase verbosity of output\n"
           "\n"
           "  -h, --help            display this message\n"
           "  -V, --version         print a string describing the version\n"
           "\n", me);
}

// --- Main ---

int main(int argc, char **argv) {
    Verilated::commandArgs(argc, argv);

    int ch;
    while ((ch = getopt_long(argc, argv, shortopts, longopts, NULL)) != -1) {
        switch (ch) {
            case 'a': load_addr  = (int32_t)strtol(optarg, NULL, 0); break;
            case 's': start_addr = (int32_t)strtol(optarg, NULL, 0); break;
            case 'v': verbose++; break;
            case 'h': usage(argv[0]);   return 0;
            case 'V': puts(version());  return 0;
            default:  usage(argv[0]);   return 1;
        }
    }

    // The image file is a positional argument, matching tsim.
    if (optind >= argc) {
        usage(argv[0]);
        return 1;
    }
    if (argc - optind > 1) {
        fprintf(stderr, "vsim: too many arguments\n");
        usage(argv[0]);
        return 1;
    }
    load_file = argv[optind];

    Vtop top;
    top.reset = 1;
    top.clk = 0;

    load_tofile(top, load_file);

    top.eval();
    int prev_state = top.rootp->Tenyr__DOT__core__DOT__state;

    while (true) {
        top.clk = !top.clk;
        if (main_time < 10) top.rootp->Tenyr__DOT__core__DOT__nextP = start_addr;
        if (main_time > 10) {
            top.reset = 0;
            // The Verilog reset vector is hardwired to 0x1000 (see RESETVECTOR
            // in common.vh), which clobbers any --start address once the reset
            // block de-asserts. Override adr_o at the first post-reset cycle so
            // --start=N redirects the first instruction fetch.
            if (main_time == 11)
                top.rootp->Tenyr__DOT__core__DOT__adr_o = (uint32_t)start_addr;
        }
        top.eval();

        int curr_state = top.rootp->Tenyr__DOT__core__DOT__state;

        // Buffer data memory operations at S3.  d_stb is high during S5
        // (instruction fetch) and S3 (data access), so checking state==S3
        // distinguishes data from instruction traffic.
        if (curr_state == S3 && top.rootp->Tenyr__DOT__d_stb) {
            mem_op   = top.rootp->Tenyr__DOT__d_wen ? 1 : 0;
            mem_addr = top.rootp->Tenyr__DOT__d_adr;
            mem_data = top.rootp->Tenyr__DOT__d_wen
                ? top.rootp->Tenyr__DOT__d_to_slav
                : top.rootp->Tenyr__DOT__d_to_mast;
        }

        // Trace at the S6->S0 transition (instruction boundary)
        if (curr_state == S0 && prev_state == S6) {
            uint32_t insn = top.rootp->Tenyr__DOT__core__DOT__insn;
            // nextP is already incremented (PC+1 after fetch); tsim shows
            // the pre-fetch PC, so subtract 1 for both IP and register dump.
            uint32_t pc = top.rootp->Tenyr__DOT__core__DOT__nextP - 1;

            // Skip the trace for the instruction at 0xffffffff that the
            // processor fetches after $finish.  tsim's pre_fetch catches
            // regs[15] == halt_addr before fetching it, so no trace is
            // emitted.  In the Verilog, the fetch has already happened, so
            // we suppress the trace here.
            if (!(insn == 0xffffffffU && finish_pending)) {
                trace_instruction(top, insn, pc);
            }
        }

        // Halt on error/retry signal (checked at every cycle, matching the
        // original behaviour where halt = err_i | rty_i).
        if (top.reset == 0 && top.rootp->Tenyr__DOT__core__DOT__halt)
            break;

        // Halt on $finish instruction (0xffffffff).  Only check at the S0
        // boundary (state just entered S0) so the $finish instruction can
        // execute through S3 and its data memory access can be traced.
        // Uses prev_state BEFORE it is updated below, so the check fires
        // only on the S6->S0 transition -- not on the S0->S0 half-cycle.
        if (top.reset == 0 && curr_state == S0 && prev_state != S0) {
            uint32_t insn = top.rootp->Tenyr__DOT__core__DOT__insn;
            if (insn == 0xffffffffU) {
                if (!finish_pending) {
                    // First sighting of $finish: let it execute so we can
                    // trace its data memory access at S3.
                    finish_pending = true;
                } else {
                    // Second sighting (the instruction at 0xffffffff that
                    // $finish branched to): print any buffered data from
                    // $finish's S3, then halt.
                    if (mem_op >= 0) {
                        trace_memaccess(mem_op == 1, mem_addr, mem_data);
                        mem_op = -1;
                    }
                    break;
                }
            }
        }

        prev_state = curr_state;

        if (main_time >= max_periods) break;
        main_time++;
    }

    top.final();
    return 0;
}
