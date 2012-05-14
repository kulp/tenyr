`timescale 1ns/10ps

`define CLOCKPERIOD 250
`define PERIOD      8
`define NDIGITS     4

module Top();
    reg clk = 0;
    always #(`CLOCKPERIOD / 2) clk = ~clk;

    Seg7Test #(.PERIODBITS(`PERIOD), .NDIGITS(`NDIGITS)) test(.clk(clk));

    initial #0 begin
        $dumpfile("Seg7Test.vcd");
        $dumpvars;
        #('h1FF * (1 << `PERIOD) * `CLOCKPERIOD) $finish;
    end
endmodule

