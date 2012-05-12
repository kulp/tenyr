`include "common.vh"
`timescale 1ns/10ps

module Seg7Test(input clk, output[7:0] seg, output[NDIGITS - 1:0] an); 

    parameter NDIGITS = 4;
    parameter BASE = 0;

    reg[15:0] counter;
    wire[15:0] c = counter;
    wire downclk, tick;

    JCounter #(.STAGES(2), .SHIFTS(10))
        downclocker(.clk(clk), .ce(1'b1), .tick(downclk));

    Seg7 #(.BASE(BASE), .NDIGITS(NDIGITS))
        seg7(.clk(downclk), .enable(1'b1), .rw(1'b1), .addr(BASE),
             .data({16'b0,c}), .reset_n(1'b1), .seg(seg), .an(an));

    JCounter #(.STAGES(5), .SHIFTS(10))
        div(.clk(downclk), .ce(1'b1), .tick(tick));

    always @(posedge tick)
        counter = counter + 1;

endmodule


