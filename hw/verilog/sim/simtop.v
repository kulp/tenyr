`include "common.vh"
`timescale 1ns/10ps

module Top();

    reg clk = 1;
    reg rhalt = 1;
    reg reset = 1;

    always #(`CLOCKPERIOD / 2) clk = ~clk;

    wire `HALTTYPE halt;
    assign halt[`HALT_EXTERNAL] = 0;

    // rhalt and reset timing should be independent of each other, and
    // do indeed appear to be so.
    initial #(1 * 4 * `CLOCKPERIOD) rhalt = 0;
    initial #(1 * 3 * `CLOCKPERIOD) reset = 0;

    Tenyr tenyr(.clk(clk), .reset(reset), .halt(halt));

`ifdef __ICARUS__
    // TODO The `ifdef guard should really be controlling for VPI availability
    reg [100:0] filename;
    reg [100:0] logfile = "Top.vcd";
    integer periods = 64;
    integer temp;
    initial #0 begin
        if ($value$plusargs("LOAD=%s", filename))
            $tenyr_load(filename);
        if ($value$plusargs("PERIODS=%d", temp))
            periods = temp;
        if ($value$plusargs("LOGFILE=%s", logfile))
            $tenyr_load(filename);
        $dumpfile(logfile);
        $dumpvars;
        #(periods * `CLOCKPERIOD) $finish;
    end
`endif

endmodule


