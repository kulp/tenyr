`timescale 1ns/10ps
`include "common.vh"

module Top();

    reg clk = 0; // TODO test starting with opposite polarity
    reg rhalt = 1;
    reg reset_n = 0;
    
    always #(`CLOCKPERIOD / 2) clk = ~clk;
    
    initial #(40 * `CLOCKPERIOD/* - 2*/) rhalt = 0;
    initial #(50 * `CLOCKPERIOD/* - 2*/) reset_n = 1;

    Tenyr tenyr(.clk(clk), .reset(~reset_n), .halt(rhalt));

endmodule

