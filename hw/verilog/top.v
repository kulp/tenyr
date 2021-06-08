`include "common.vh"
`timescale 1ms/10us

module Tenyr(
    input clk, reset, inout halt,
    output[7:0] seg, output[3:0] an, inout[23:0] gpio,
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
    wire[31:0] d_to_mast, i_to_mast;

    assign i_ack = i_stb;

    tenyr_mainclock clocks(
        .clk_in ( clk   ), .clk_core ( clk_core ),
        .reset  ( reset ), .clk_vga  ( clk_vga  )
    );

    Core core(
        .clk   ( clk_core  ),
        .halt  ( halt      ),
        .reset ( reset     ),
        .adr_o ( d_adr     ),
        .dat_o ( d_to_slav ),
        .dat_i ( d_to_mast ),
        .wen_o ( d_wen     ),
        .sel_o ( d_sel     ),
        .stb_o ( d_stb     ),
        .ack_i ( d_ack     ),
        .err_i ( 1'b0      ),
        .rty_i ( 1'b0      ),
        .cyc_o ( d_cyc     )
    );

// -----------------------------------------------------------------------------
// MEMORY ----------------------------------------------------------------------

    wire r_wen, r_stb, r_cyc, r_ack;
    wire[3:0] r_sel;
    wire[31:0] r_adr, r_ddn, r_dup;

    TwoPortRAM #(.LOADH(1), .LOADFILE(LOADFILE), .INIT(0),
        .PBITS(32), .ABITS(RAMABITS), .OFFSET(`RESETVECTOR)
    ) ram(
        .clka  ( clk_core ), .clkb ( 1'b0 ),
        .ena   ( r_stb    ), .enb  ( 1'b0 ),
        .acka  ( r_ack    ),
        .wea   ( r_wen    ),
        .addra ( r_adr    ),
        .dina  ( r_ddn    ),
        .douta ( r_dup    )
    );

// -----------------------------------------------------------------------------
// DEVICES ---------------------------------------------------------------------

    wire s_wen, s_stb, s_cyc;
    wire[3:0] s_sel;
    wire[31:0] s_adr, s_ddn, s_dup;
    wire s_stbcyc = s_stb & s_cyc;

`ifdef SERIAL
    // TODO write a hardware-compatible serial device ; rename to eliminate `Sim`
    SimWrap_simserial #(.BASE(12'h20), .SIZE(2)) serial(
        .clk ( clk_core ), .reset ( reset ), .enable ( s_stbcyc ),
        .rw  ( s_wen    ), .addr  ( s_adr ), .data   ( s_ddn    )
    );
`endif

    wire g_wen, g_stb, g_cyc, g_ack;
    wire[3:0] g_sel;
    wire[31:0] g_adr, g_ddn, g_dup;
    wire g_stbcyc = g_stb & g_cyc;

    Seg7 seg7(
        .clk    ( clk_core ), .rw   ( g_wen ), .seg   ( seg   ),
        .reset  ( reset    ), .addr ( g_adr ), .an    ( an    ),
        .strobe ( g_stbcyc ), .d_in ( g_ddn ), .d_out ( g_dup ), .ack( g_ack )
    );

    wire o_wen, o_stb, o_cyc;
    wire[3:0] o_sel;
    wire[31:0] o_adr, o_ddn, o_dup;
    wire o_stbcyc = o_stb & o_cyc;

    Gpio #(.COUNT(24)) gio(
        .clk    ( clk_core ), .rw   ( o_wen ), .gpio   ( gpio  ),
        .reset  ( reset    ), .addr ( o_adr ), .data_i ( o_ddn ),
        .strobe ( o_stbcyc ),                  .data_o ( o_dup )
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

    wire x_wen, x_stb, x_cyc;
    wire[3:0] x_sel;
    wire[31:0] x_adr, x_ddn, x_dup;
    assign x_dup = 32'hffffffff;

    wb_mux #(
        .num_slaves(6),
        .MATCH_ADDR({
    //  GPIO    7-seg   VGA display  serial port  memory       default
        32'h200,32'h100,`VIDEO_ADDR ,32'h00000020,`RESETVECTOR,-32'sd1 }),
        .MATCH_MASK({
        -32'd4 ,-32'd2 ,32'hffff0000,32'hfffffffe,32'hffffd000,-32'sd1 })
    ) mux (
        .wb_clk_i  ( clk_core   ),
        .wb_rst_i  ( reset      ),
        .wbm_adr_i ( d_adr      ),
        .wbm_dat_i ( d_to_slav  ),
        .wbm_dat_o ( d_to_mast  ),
        .wbm_we_i  ( d_wen      ),
        .wbm_sel_i ( d_sel      ),
        .wbm_stb_i ( d_stb      ),
        .wbm_ack_o ( d_ack      ),
        .wbm_err_o ( /* TODO */ ),
        .wbm_rty_o ( /* TODO */ ),
        .wbm_cyc_i ( d_cyc      ),

        //            gpio   7-seg  VGA    serial mem    def.
        .wbs_adr_o ({ o_adr, g_adr, v_adr, s_adr, r_adr, x_adr }),
        .wbs_dat_o ({ o_ddn, g_ddn, v_ddn, s_ddn, r_ddn, x_ddn }),
        .wbs_dat_i ({ o_dup, g_dup, v_dup, s_dup, r_dup, x_dup }),
        .wbs_we_o  ({ o_wen, g_wen, v_wen, s_wen, r_wen, x_wen }),
        .wbs_sel_o ({ o_sel, g_sel, v_sel, s_sel, r_sel, x_sel }),
        .wbs_stb_o ({ o_stb, g_stb, v_stb, s_stb, r_stb, x_stb }),
        .wbs_ack_i ({ o_stb, g_ack, v_stb, s_stb, r_ack, x_stb }),
        .wbs_err_i ({  1'b0,  1'b0,  1'b0,  1'b0,  1'b0,  1'b0 }),
        .wbs_rty_i ({  1'b0,  1'b0,  1'b0,  1'b0,  1'b0,  1'b0 }),
        .wbs_cyc_o ({ o_cyc, g_cyc, v_cyc, s_cyc, r_cyc, x_cyc }),

        // unused ports
        .wbm_cti_i ( 3'bz ),
        .wbm_bte_i ( 2'bz )
    );

endmodule

