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

    wire d_rw, d_strobe, d_cycle, d_ack;
    wire i_rw, i_strobe, i_cycle, i_ack;
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
        .ackD_i ( d_ack      ), .ackI_i ( i_ack     ),
        .errD_i ( 1'b0       ), .errI_i ( 1'b0      ),
        .rtyD_i ( 1'b0       ), .rtyI_i ( 1'b0      ),
        .cycD_o ( d_cycle    ), .cycI_o ( i_cycle   )  // TODO hook up _cycle
    );

// -----------------------------------------------------------------------------
// MEMORY ----------------------------------------------------------------------

    wire r_rw, r_strobe, r_cycle;
    wire[3:0] r_sel;
    wire[31:0] r_addr, r_to_slav, r_to_mast;

    ramwrap #(.LOAD(1), .LOADFILE(LOADFILE), .INIT(0),
        .PBITS(32), .ABITS(RAMABITS), .BASE_A(`RESETVECTOR)
    ) ram(
        .clka  ( clk_core  ), .clkb  ( clk_core  ),
        .ena   ( r_strobe  ), .enb   ( i_strobe  ),
        .wea   ( r_rw      ), .web   ( i_rw      ),
        .addra ( r_addr    ), .addrb ( i_addr    ),
        .dina  ( r_to_slav ), .dinb  ( i_to_slav ),
        .douta ( r_to_mast ), .doutb ( i_to_mast )
    );

// -----------------------------------------------------------------------------
// DEVICES ---------------------------------------------------------------------

`ifdef SERIAL
    wire s_rw, s_strobe, s_cycle;
    wire[3:0] s_sel;
    wire[31:0] s_addr, s_to_slav, s_to_mast;

    // TODO xilinx-compatible serial device ; rename to eliminate `Sim`
    SimWrap_simserial #(.BASE(12'h20), .SIZE(2)) serial(
        .clk ( clk  ), .reset_n ( _reset_n ), .enable ( s_strobe  ),
        .rw  ( s_rw ), .addr    ( s_addr   ), .data   ( s_to_slav )
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

    wb_mux #(
        .NUM_SLAVES(2),
        .MATCH_ADDR( { `RESETVECTOR, 32'h00000020 } ),
        .MATCH_MASK( { 32'hffffd000, 32'hfffffffe } )
    ) mux (
        .wb_clk_i  ( clk_core   ),
        .wb_rst_i  ( _reset_n   ),
        .wbm_adr_i ( d_addr     ), .wbs_adr_o ( { r_addr   , s_addr    } ),
        .wbm_dat_i ( d_to_slav  ), .wbs_dat_i ( { r_to_mast, s_to_mast } ),
        .wbm_dat_o ( d_to_mast  ), .wbs_dat_o ( { r_to_slav, s_to_slav } ),
        .wbm_we_i  ( d_rw       ), .wbs_we_o  ( { r_rw     , s_rw      } ),
        .wbm_sel_i ( d_sel      ), .wbs_sel_o ( { r_sel    , s_sel     } ),
        .wbm_stb_i ( d_strobe   ), .wbs_stb_o ( { r_strobe , s_strobe  } ),
        .wbm_ack_o ( d_ack      ), .wbs_ack_i ( { r_strobe , s_strobe  } ),
        .wbm_err_o ( /* TODO */ ), .wbs_err_i ( { 1'b0     , 1'b0      } ),
        .wbm_rty_o ( /* TODO */ ), .wbs_rty_i ( { 1'b0     , 1'b0      } ),
        .wbm_cyc_i ( d_cycle    ), .wbs_cyc_o ( { r_cycle  , s_cycle   } ),
        .wbm_cti_i ( 3'bz       ),
        .wbm_bte_i ( 2'bz       )
    );

endmodule

