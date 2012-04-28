`include "common.vh"
`timescale 1ns/10ps

module Seg7Test(input clk, output[7:0] seg, output[NDIGITS - 1:0] an); 

    parameter NDIGITS = 4;
    parameter PERIODBITS = 21;

    wire[31:0] c;
	wire[PERIODBITS - 1:0] counter;
    wire downclk, clk_valid, tick;

    clk_wiz_v3_3 clkdiv(.in(clk), .out(downclk), .CLK_VALID(clk_valid));

    Seg7 #(.BASE(0), .NDIGITS(NDIGITS))
        seg7(.clk(downclk), .enable(1'b1), .rw(1'b1), .addr(32'b0),
             .data(c), .reset_n(1'b1), .seg(seg), .an(an));

	assign tick = ~|counter;

	upcounter21 div(.clk(downclk), .q(counter), .ce(clk_valid));
	upcounter   ctr(.clk(downclk), .q(c), .ce(tick & clk_valid));

endmodule


