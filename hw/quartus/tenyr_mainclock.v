// For a 50MHz input
// Produces a 50MHz clk_core0 and a 25MHz clk_vga
// review clk_core0 for potentially bad gating
module tenyr_mainclock(
    input in, reset, output locked,
    output clk_core0, input clk_core0_CE, output reg clk_vga, input clk_vga_CE
);
    assign locked = 1;

    always @(posedge in) begin
        if (clk_vga_CE)
            clk_core0 <= in;
        if (clk_vga_CE)
            clk_vga <= ~clk_vga;
    end

endmodule

