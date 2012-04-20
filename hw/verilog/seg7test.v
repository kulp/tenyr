`include "common.vh"
`timescale 1ns/10ps

module Seg7Test(input clk, output[7:0] seg, output[NDIGITS - 1:0] an); 

    parameter NDIGITS = 4;
    parameter PERIODBITS = 21;

    integer counter = 0;
    reg[15:0] out = 0;
    wire[31:0] c = out;

`ifndef ICARUS
    wire downclk;
    clk_wiz_v3_3 clkdiv(.CLK_IN1(clk), .CLK_OUT1(downclk));
`else
    wire downclk = clk;
`endif

    Seg7 #(.BASE(0), .NDIGITS(NDIGITS))
        seg7(.clk(downclk), .enable(1'b1), .rw(1'b1), .addr(32'b0),
             .data(c), ._reset(1'b1), .seg(seg), .an(an));

    always @(negedge downclk) begin
        counter = (counter + 1) & {PERIODBITS{1'b1}};
        if (counter == 0)
            out = out + 1;
    end

endmodule


