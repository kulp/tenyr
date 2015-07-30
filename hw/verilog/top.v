`include "common.vh"
`timescale 1ms/10us

`ifndef SIM
`ifndef __QUARTUS__
  `define VGA
`endif
`endif
`define SEG7

module Tenyr(
    input clk, reset, inout wor halt,
    output[7:0] Led, output[7:0] seg, output[3:0] an,
    output[2:0] vgaRed, vgaGreen, output[2:1] vgaBlue, output hsync, vsync
);

    parameter LOADFILE = "default.memh";
    parameter RAMABITS = 13;

    wire d_rw, d_strobe, d_cycle;
    wire i_rw, i_strobe, i_cycle;
    wire[3:0] d_sel, i_sel;
    wire valid_clk, clk_vga, clk_core;
    wire[31:0] i_addr;
    wire[31:0] d_addr, d_to_slav, i_to_slav;
    wor [31:0] d_to_mast, i_to_mast;

    wire _reset_n = ~reset;

    assign Led[7:0] = halt;

    tenyr_mainclock clocks(
        .clk_in ( clk   ), .clk_core ( clk_core ),
        .reset  ( reset ), .clk_vga  ( clk_vga  )
    );

    Core core(
        .clk    ( clk_core   ), .halt   ( halt      ), .reset_n ( _reset_n ),
        .adrD_o ( d_addr     ), .adrI_o ( i_addr    ),
        .datD_o ( d_to_slav  ), .datI_o ( i_to_slav ),
        .datD_i ( d_to_mast  ), .datI_i ( i_to_mast ),
        .wenD_o ( d_rw       ), .wenI_o ( i_rw      ),
        .selD_o ( d_sel      ), .selI_o ( i_sel     ),
        .stbD_o ( d_strobe   ), .stbI_o ( i_strobe  ),
        .ackD_i ( d_strobe   ), .ackI_i ( i_strobe  ),
        .errD_i ( 1'b0       ), .errI_i ( 1'b0      ),
        .rtyD_i ( 1'b0       ), .rtyI_i ( 1'b0      ),
        .cycD_o ( d_cycle    ), .cycI_o ( i_cycle   )  // TODO hook up _cycle
    );

// -----------------------------------------------------------------------------
// MEMORY ----------------------------------------------------------------------

    ramwrap #(.LOAD(1), .LOADFILE(LOADFILE), .INIT(0),
        .PBITS(32), .ABITS(RAMABITS), .BASE_A(`RESETVECTOR)
    ) ram(
        .clka  ( clk_core  ), .clkb  ( clk_core  ),
        .ena   ( d_strobe  ), .enb   ( i_strobe  ),
        .wea   ( d_rw      ), .web   ( i_rw      ),
        .addra ( d_addr    ), .addrb ( i_addr    ),
        .dina  ( d_to_slav ), .dinb  ( i_to_slav ),
        .douta ( d_to_mast ), .doutb ( i_to_mast )
    );

// -----------------------------------------------------------------------------
// DEVICES ---------------------------------------------------------------------

`ifdef SERIAL
    // TODO xilinx-compatible serial device ; rename to eliminate `Sim`
    SimWrap_simserial #(.BASE(12'h20), .SIZE(2)) serial(
        .clk ( clk  ), .reset_n ( _reset_n ), .enable ( d_strobe  ),
        .rw  ( d_rw ), .addr    ( d_addr   ), .data   ( d_to_slav )
    );
`endif

`ifdef SEG7
    Seg7 #(.BASE(12'h100)) seg7(
        .clk     ( clk_core ), .rw   ( d_rw      ), .seg ( seg ),
        .reset_n ( _reset_n ), .addr ( d_addr    ), .an  ( an  ),
        .strobe  ( d_strobe ), .data ( d_to_slav )
    );
`endif

`ifdef VGA
    VGAwrap vga(
        .clk_core ( clk_core ), .rw     ( d_rw      ), .vgaRed   ( vgaRed   ),
        .clk_vga  ( clk_vga  ), .addr   ( d_addr    ), .vgaGreen ( vgaGreen ),
        .en       ( 1'b1     ), .d_in   ( d_to_slav ), .vgaBlue  ( vgaBlue  ),
        .reset_n  ( _reset_n ), .strobe ( d_strobe  ), .hsync    ( hsync    ),
                                                       .vsync    ( vsync    )
    );
`endif

endmodule

