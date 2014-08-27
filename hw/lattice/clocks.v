`timescale 1ns/1ps
module tenyr_mainclock(input clk_in, reset, output clk_core, clk_vga);

    assign clk_core = clk_in;
    assign clk_vga = clk_in;

endmodule
