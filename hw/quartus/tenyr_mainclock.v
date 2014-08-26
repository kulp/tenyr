// For a 50MHz input
// Produces a 50MHz clk_core and a 25MHz clk_vga
module tenyr_mainclock(
    input clk_in, reset, output locked,
    output clk_core, output reg clk_vga
);
    assign locked = 1;
    assign clk_core = clk_in;

    always @(posedge clk_in)
        clk_vga <= ~clk_vga;

endmodule

