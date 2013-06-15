`include "common.vh"
`timescale 1ns/10ps

module Top();

    parameter LOADFILE = "default.memh";

    reg clk = 1;
    reg rhalt = 1;
    reg reset = 1;
    reg [31:0] irqs = 0;

    always #(`CLOCKPERIOD / 2) clk <= ~clk;

    wire `HALTTYPE halt;
    assign halt[`HALT_EXTERNAL] = 0;

    // rhalt and reset timing should be independent of each other, and
    // do indeed appear to be so.
    initial #(1 * 4 * `CLOCKPERIOD) rhalt = 0;
    initial #(1 * 3 * `CLOCKPERIOD) reset = 0;

    Tenyr #(.LOADFILE(LOADFILE))
        tenyr(.clk(clk), .reset(reset), .halt(halt), .irqs(irqs));

`ifdef __ICARUS__
    // TODO The `ifdef guard should really be controlling for VPI availability
    reg [800:0] filename;
    reg [800:0] logfile = "Top.vcd";
    integer irq_times[1023:0];
    integer irq_signals[1023:0];
    integer irq_ptr = 0;
    integer irq_size = 1024;
    integer periods = 64;
    integer clk_count = 0;
    integer temp;
    initial #0 begin
        for (temp = 0; temp < 1024; temp = temp + 1)
            irq_times[temp] = 0;

        if ($value$plusargs("LOAD=%s", filename))
            $tenyr_load(filename); // replace with $readmemh ?
        if ($value$plusargs("PERIODS=%d", temp))
            periods = temp;
        if ($value$plusargs("LOGFILE=%s", filename))
            logfile = filename;
        if ($value$plusargs("VECTORS=%s", filename))
            $readmemh(filename, tenyr.eib.vecs.wrapped.store);
        if ($value$plusargs("INTERRUPT_TIMES=%s", filename)) begin
            $readmemh(filename, irq_times);

            if ($value$plusargs("INTERRUPT_FIRST=%d", temp))
                irq_ptr = temp;
            if ($value$plusargs("INTERRUPT_LAST=%d", temp))
                irq_size = temp;
            for (temp = irq_ptr; temp < irq_size; temp = temp + 1)
                irq_signals[temp] = 0;

            if ($value$plusargs("INTERRUPT_SIGNALS=%s", filename))
                $readmemh(filename, irq_signals);
        end
        $dumpfile(logfile);
        $dumpvars;
        #(periods * `CLOCKPERIOD) $finish;
    end

    task do_irq(input integer which, clocks);
        begin
            irqs[which] = 1;
            irqs[which] = #(clocks * `CLOCKPERIOD) 0;
        end
    endtask

    always #`CLOCKPERIOD clk_count = clk_count + 1;

    always #`CLOCKPERIOD begin
        if (irq_ptr < irq_size && irq_times[irq_ptr] == clk_count) begin
            do_irq(irq_signals[irq_ptr], 2);
            irq_ptr = irq_ptr + 1;
        end
    end
`endif

endmodule

