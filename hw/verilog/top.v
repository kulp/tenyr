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

    wire d_wen, d_stb, d_cyc, d_ack;
    wire i_wen, i_stb, i_cyc, i_ack;
    wire[3:0] d_sel, i_sel;
    wire valid_clk, clk_vga, clk_core;
    wire[31:0] i_adr;
    wire[31:0] d_adr, d_to_slav, i_to_slav;
    wor [31:0] d_to_mast, i_to_mast;

    wire _reset_n = ~reset;

    assign Led[7:0] = halt;

    tenyr_mainclock clocks(
        .clk_in ( clk   ), .clk_core ( clk_core ),
        .reset  ( reset ), .clk_vga  ( clk_vga  )
    );

    Core core(
        .clk    ( clk_core  ), .halt   ( halt      ), .reset_n ( _reset_n ),
        .adrD_o ( d_adr     ), .adrI_o ( i_adr     ),
        .datD_o ( d_to_slav ), .datI_o ( i_to_slav ),
        .datD_i ( d_to_mast ), .datI_i ( i_to_mast ),
        .wenD_o ( d_wen     ), .wenI_o ( i_wen     ),
        .selD_o ( d_sel     ), .selI_o ( i_sel     ),
        .stbD_o ( d_stb     ), .stbI_o ( i_stb     ),
        .ackD_i ( d_ack     ), .ackI_i ( i_ack     ),
        .errD_i ( 1'b0      ), .errI_i ( 1'b0      ),
        .rtyD_i ( 1'b0      ), .rtyI_i ( 1'b0      ),
        .cycD_o ( d_cyc     ), .cycI_o ( i_cyc     )
    );

// -----------------------------------------------------------------------------
// MEMORY ----------------------------------------------------------------------

    wire r_wen, r_stb, r_cyc;
    wire[3:0] r_sel;
    wire[31:0] r_adr, r_ddn, r_dup;

    BlockRAM #(.LOAD(1), .LOADFILE(LOADFILE), .INIT(0),
        .PBITS(32), .ABITS(RAMABITS), .OFFSET(`RESETVECTOR)
    ) ram(
        .clka  ( clk_core ), .clkb  ( clk_core  ),
        .ena   ( r_stb    ), .enb   ( i_stb     ),
        .wea   ( r_wen    ), .web   ( i_wen     ),
        .addra ( r_adr    ), .addrb ( i_adr     ),
        .dina  ( r_ddn    ), .dinb  ( i_to_slav ),
        .douta ( r_dup    ), .doutb ( i_to_mast )
    );

// -----------------------------------------------------------------------------
// DEVICES ---------------------------------------------------------------------

`ifdef SERIAL
    wire s_wen, s_stb, s_cyc;
    wire[3:0] s_sel;
    wire[31:0] s_adr, s_ddn, s_dup;

    // TODO xilinx-compatible serial device ; rename to eliminate `Sim`
    SimWrap_simserial #(.BASE(12'h20), .SIZE(2)) serial(
        .clk ( clk   ), .reset_n ( _reset_n ), .enable ( s_stb ),
        .rw  ( s_wen ), .addr    ( s_adr    ), .data   ( s_ddn )
    );
`endif

`ifdef SEG7
    Seg7 #(.BASE(12'h100)) seg7(
        .clk     ( clk_core ), .rw   ( d_wen     ), .seg ( seg ),
        .reset_n ( _reset_n ), .addr ( d_adr     ), .an  ( an  ),
        .strobe  ( d_stb    ), .data ( d_to_slav )
    );
`endif

`ifdef VGA
    VGAwrap vga(
        .clk_core ( clk_core ), .rw     ( d_wen     ), .vgaRed   ( vgaRed   ),
        .clk_vga  ( clk_vga  ), .addr   ( d_adr     ), .vgaGreen ( vgaGreen ),
        .en       ( 1'b1     ), .d_in   ( d_to_slav ), .vgaBlue  ( vgaBlue  ),
        .reset_n  ( _reset_n ), .strobe ( d_stb     ), .hsync    ( hsync    ),
                                                       .vsync    ( vsync    )
    );
`endif

    wb_mux #(
        .NUM_SLAVES(2),
        .MATCH_ADDR( { `RESETVECTOR, 32'h00000020 } ),
        .MATCH_MASK( { 32'hffffd000, 32'hfffffffe } )
    ) mux (
        .wb_clk_i  ( clk_core   ),
        .wb_rst_i  ( _reset_n   ),
        .wbm_adr_i ( d_adr      ), .wbs_adr_o ( { r_adr, s_adr } ),
        .wbm_dat_i ( d_to_slav  ), .wbs_dat_i ( { r_dup, s_dup } ),
        .wbm_dat_o ( d_to_mast  ), .wbs_dat_o ( { r_ddn, s_ddn } ),
        .wbm_we_i  ( d_wen      ), .wbs_we_o  ( { r_wen, s_wen } ),
        .wbm_sel_i ( d_sel      ), .wbs_sel_o ( { r_sel, s_sel } ),
        .wbm_stb_i ( d_stb      ), .wbs_stb_o ( { r_stb, s_stb } ),
        .wbm_ack_o ( d_ack      ), .wbs_ack_i ( { r_stb, s_stb } ),
        .wbm_err_o ( /* TODO */ ), .wbs_err_i ( { 1'b0 , 1'b0  } ),
        .wbm_rty_o ( /* TODO */ ), .wbs_rty_i ( { 1'b0 , 1'b0  } ),
        .wbm_cyc_i ( d_cyc      ), .wbs_cyc_o ( { r_cyc, s_cyc } ),
        .wbm_cti_i ( 3'bz       ),
        .wbm_bte_i ( 2'bz       )
    );

endmodule

