`include "common.vh"
`timescale 1ms/10us

module `SIMCLK(
	input in, reset, output locked,
	output clk_core0, input clk_core0_CE,
	output clk_vga, input clk_vga_CE
);

	assign locked    = ~reset;
	assign clk_core0 = in & clk_core0_CE;
	assign clk_vga   = in & clk_vga_CE;

endmodule

