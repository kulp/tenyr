`include "common.vh"
`timescale 1ms/10us

// This clock module simply passes its input to its output.
module tenyr_mainclock(
    input clk_in, reset, output locked,
    output clk_core, clk_vga
);

    assign locked   = ~reset;
    assign clk_core = clk_in;
    assign clk_vga  = clk_in;

endmodule

