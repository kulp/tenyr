`timescale 1ns/1ps
module tenyr_mainclock(input clk_in, reset, output clk_core, clk_vga);

    wire clk_sys, clk_in_buf, locked_core, locked_vga, clk_core_un, clk_vga_un;

    BUFG   BUFG_clk_in  (.I(clk_in),      .O(clk_in_buf));
    BUFGCE BUFG_clk_core(.I(clk_core_un), .O(clk_core  ), .CE(locked_core));
    BUFGCE BUFG_clk_vga (.I(clk_vga_un ), .O(clk_vga   ), .CE(locked_vga ));

    DCM_CLKGEN #(.CLKFX_DIVIDE(5), .CLKFX_MULTIPLY(4)) clk_gen_core(
        .CLKIN  ( clk_in_buf  ),
        .CLKFX  ( clk_core_un ),
        .RST    ( reset       ),
        .LOCKED ( locked_core )
    );

    DCM_CLKGEN #(.CLKFX_MULTIPLY(4), .CLKFX_DIVIDE(4), .CLKFXDV_DIVIDE(4)) clk_gen_vga(
        .CLKIN  ( clk_in_buf ),
        .CLKFXDV( clk_vga_un ),
        .RST    ( reset      ),
        .LOCKED ( locked_vga )
    );

endmodule
