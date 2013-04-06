`include "common.vh"
`timescale 1ns/10ps

module Top();

    reg clk = 1;
    reg rhalt = 1;
    reg reset_n = 0;

    always #(`CLOCKPERIOD / 2) clk = ~clk;

    wire `HALTTYPE halt;
    assign halt[`HALT_TENYR] = rhalt;
    assign halt[`HALT_EXTERNAL] = 0;

    // rhalt and reset_n timing should be independent of each other, and
    // do indeed appear to be so.
    initial #(4 * `CLOCKPERIOD) rhalt = 0;
    initial #(3 * `CLOCKPERIOD) reset_n = 1;

    Tenyr tenyr(.clk(clk), .reset_n(reset_n), .halt(halt));

    reg [100:0] filename;
    integer periods = 64;
    integer temp;
    initial #0 begin
        $dumpfile("Top.vcd");
        if ($value$plusargs("LOAD=%s", filename))
            $tenyr_load(filename);
        if ($value$plusargs("PERIODS=%d", temp))
            periods = temp;
        $dumpvars;
        #(periods * `CLOCKPERIOD) $finish;
    end

endmodule


