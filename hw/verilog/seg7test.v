`include "common.vh"
`timescale 1ns/10ps

module Seg7Test(input clk, output[7:0] seg, output[NDIGITS - 1:0] an); 

    parameter NDIGITS = 4;
    parameter BASE = 0;

    reg[15:0] counter;
    wire[15:0] c = counter;

    reg downclk = 0;
    reg[6:0] downctr = 0;
    always @(posedge clk) begin
        downctr <= downctr + 1;
        if (downctr == 0)
            downclk <= ~downclk;
    end

    reg tick = 0;
    reg[15:0] tickctr = 0;
    always @(posedge clk) begin
        tickctr <= tickctr + 1;
        if (tickctr == 0)
            tick <= ~tick;
    end

    Seg7 #(.BASE(BASE), .NDIGITS(NDIGITS))
        seg7(.clk(downclk), .enable(1'b1), .rw(1'b1), .addr(BASE),
             .data({16'b0,c}), .reset_n(1'b1), .seg(seg), .an(an));

    always @(posedge tick)
        counter = counter + 1;

endmodule


