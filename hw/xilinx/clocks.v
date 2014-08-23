`timescale 1ns/1ps
module tenyr_mainclock(input clk_in, reset, output clk_core, clk_vga);

    wire clk_sys, clk_in_buf;

    BUFG BUFG_clk_in(.I(clk_in), .O(clk_in_buf));

    DCM_SP #(
        .CLK_FEEDBACK       ( "1X"                 ),
        .CLKDV_DIVIDE       ( 4.0                  ),
        .CLKFX_DIVIDE       ( 5                    ),
        .CLKFX_MULTIPLY     ( 4                    ),
        .CLKIN_DIVIDE_BY_2  ( "FALSE"              ),
        .CLKIN_PERIOD       ( 10.0                 ),
        .CLKOUT_PHASE_SHIFT ( "NONE"               ),
        .DESKEW_ADJUST      ( "SYSTEM_SYNCHRONOUS" ),
        .PHASE_SHIFT        ( 0                    ),
        .STARTUP_WAIT       ( "TRUE"               )
    ) DCM_inst(
        .CLK0     ( clk_sys    ),
        .CLKDV    ( clk_vga    ),
        .CLKFX    ( clk_core   ),
        .CLKFB    ( clk_sys    ),
        .CLKIN    ( clk_in_buf ),
        .PSCLK    ( 1'b0       ),
        .PSEN     ( 1'b0       ),
        .PSINCDEC ( 1'b0       ),
        .DSSEN    ( 1'b0       ),
        .RST      ( reset      )
    );

endmodule
