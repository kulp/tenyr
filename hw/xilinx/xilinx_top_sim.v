`timescale 1ns/10ps

`define CLOCKPERIOD 20

module Top();
    reg clk = 0; // TODO test starting with opposite polarity
    reg halt = 1;
    reg _reset = 0;
    
    always #(`CLOCKPERIOD / 2) clk = ~clk;
    
    initial #(2 * `CLOCKPERIOD/* - 2*/) halt = 0;
    initial #(1 * `CLOCKPERIOD/* - 2*/) _reset = 1;

    wor _halt = halt;

    Tenyr tenyr(.clk(clk), ._reset(_reset), ._halt(_halt));
endmodule

