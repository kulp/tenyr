#include "Vtop.h"

// If "verilator --trace" is used, include the tracing class
#if VM_TRACE
# include <verilated_vcd_c.h>
#endif

static const vluint64_t MAX_TIME = 1000000u;

// Current simulation time (64-bit unsigned)
vluint64_t main_time = 0;
// Called by $time in Verilog
double sc_time_stamp()
{
    return static_cast< double >( main_time );  // Note does conversion to real, to match SystemC
}

static inline bool finished(const Vtop &top)
{
    return main_time >= MAX_TIME
        || Verilated::gotFinish()
        || (top.reset == 0 && top.Tenyr__DOT__core__DOT__nextP == 0);
}

int main(int argc, char* argv[])
{
    Verilated::commandArgs(argc, argv);

    Vtop top;

#if VM_TRACE
    // If verilator was invoked with --trace argument,
    // and if at run time passed the +trace argument, turn on tracing
    VerilatedVcdC* tfp = NULL;
    const char* flag = Verilated::commandArgsPlusMatch("trace");
    if (flag && 0==strcmp(flag, "+trace")) {
        Verilated::traceEverOn(true);  // Verilator must compute traced signals
        VL_PRINTF("Enabling waves into logs/vlt_dump.vcd...\n");
        tfp = new VerilatedVcdC;
        top.trace(tfp, 99);  // Trace 99 levels of hierarchy
        Verilated::mkdir("logs");
        tfp->open("logs/vlt_dump.vcd");  // Open the dump file
    }
#endif

    top.reset = 1;
    top.clk = 0;

    while (!finished(top)) {
        top.clk = !top.clk;
        if (main_time < 10)
            top.Tenyr__DOT__core__DOT__nextP = 0x1000;
        if (main_time > 10)
            top.reset = 0;
        top.eval();
        printf("P = %08x\n", top.Tenyr__DOT__core__DOT__nextP);

#if VM_TRACE
        // Dump trace data for this cycle
        if (tfp) tfp->dump(main_time);
#endif

        main_time++;
    }

    top.final();
}
