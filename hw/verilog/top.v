`include "common.vh"
`timescale 1ms/10us

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

    assign Led[7:0] = halt;
    assign i_ack = i_stb;

    tenyr_mainclock clocks(
        .clk_in ( clk   ), .clk_core ( clk_core ),
        .reset  ( reset ), .clk_vga  ( clk_vga  )
    );

    Core core(
        .clk    ( clk_core  ), .halt   ( halt      ), .reset ( reset ),
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

    wire s_wen, s_stb, s_cyc;
    wire[3:0] s_sel;
    wire[31:0] s_adr, s_ddn, s_dup;

`ifdef SERIAL
    // TODO xilinx-compatible serial device ; rename to eliminate `Sim`
    SimWrap_simserial #(.BASE(12'h20), .SIZE(2)) serial(
        .clk ( clk   ), .reset ( reset ), .enable ( s_stb ),
        .rw  ( s_wen ), .addr  ( s_adr ), .data   ( s_ddn )
    );
`endif

    wire g_wen, g_stb, g_cyc;
    wire[3:0] g_sel;
    wire[31:0] g_adr, g_ddn, g_dup;
    wire g_stbcyc = g_stb & g_cyc;

    Seg7 seg7(
        .clk    ( clk_core ), .rw   ( g_wen ), .seg   ( seg   ),
        .reset  ( reset    ), .addr ( g_adr ), .an    ( an    ),
        .strobe ( g_stbcyc ), .d_in ( g_ddn ), .d_out ( g_dup )
    );

    wire v_wen, v_stb, v_cyc;
    wire[3:0] v_sel;
    wire[31:0] v_adr, v_ddn, v_dup;
    wire v_stbcyc = v_stb & v_cyc;

`ifdef VGA
    VGAwrap vga(
        .clk_core ( clk_core ), .rw     ( v_wen ), .vgaRed   ( vgaRed   ),
        .clk_vga  ( clk_vga  ), .addr   ( v_adr ), .vgaGreen ( vgaGreen ),
        .en       ( 1'b1     ), .d_in   ( v_ddn ), .vgaBlue  ( vgaBlue  ),
        .reset    ( reset    ), .d_out  ( v_dup ), .hsync    ( hsync    ),
        .strobe   ( v_stbcyc ),                    .vsync    ( vsync    )
    );
`endif

    wb_mux #(
        .NUM_SLAVES(4),
        //            7-seg disp.   VGA display   serial port   main memory
        .MATCH_ADDR({ 32'h00000100, `VIDEO_ADDR , 32'h00000020, `RESETVECTOR }),
        .MATCH_MASK({ 32'hfffffffe, 32'hffff0000, 32'hfffffffe, 32'hffffd000 })
    ) mux (
        .wb_clk_i  ( clk_core   ),
        .wb_rst_i  ( reset      ), //            7-seg  VGA    serial mem
        .wbm_adr_i ( d_adr      ), .wbs_adr_o ({ g_adr, v_adr, s_adr, r_adr }),
        .wbm_dat_i ( d_to_slav  ), .wbs_dat_o ({ g_ddn, v_ddn, s_ddn, r_ddn }),
        .wbm_dat_o ( d_to_mast  ), .wbs_dat_i ({ g_dup, v_dup, s_dup, r_dup }),
        .wbm_we_i  ( d_wen      ), .wbs_we_o  ({ g_wen, v_wen, s_wen, r_wen }),
        .wbm_sel_i ( d_sel      ), .wbs_sel_o ({ g_sel, v_sel, s_sel, r_sel }),
        .wbm_stb_i ( d_stb      ), .wbs_stb_o ({ g_stb, v_stb, s_stb, r_stb }),
        .wbm_ack_o ( d_ack      ), .wbs_ack_i ({ g_stb, v_stb, s_stb, r_stb }),
        .wbm_err_o ( /* TODO */ ), .wbs_err_i ({ 1'b0 , 1'b0 , 1'b0 , 1'b0  }),
        .wbm_rty_o ( /* TODO */ ), .wbs_rty_i ({ 1'b0 , 1'b0 , 1'b0 , 1'b0  }),
        .wbm_cyc_i ( d_cyc      ), .wbs_cyc_o ({ g_cyc, v_cyc, s_cyc, r_cyc }),
        .wbm_cti_i ( 3'bz       ),
        .wbm_bte_i ( 2'bz       )
    );

endmodule

