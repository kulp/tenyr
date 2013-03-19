`include "common.vh"
`timescale 1ms/10us

`ifndef SIM
`define VGA
`endif
`define SEG7

module Tenyr(
    input clk, halt, reset,
    output[7:0] Led, output[7:0] seg, output[3:0] an,
    output[2:0] vgaRed, vgaGreen, output[2:1] vgaBlue, output hsync, vsync
);

    wire oper_rw;
    wire valid_clk, clk_vga, clk_core;
    wire _reset_n = valid_clk & ~reset;
    wire[31:0] insn_addr, oper_addr;
    wire[31:0] insn_data, out_data;
    wire[31:0] oper_data = !oper_rw ? out_data : 32'bz;

    assign vgaRed  [1:0] = {2{vgaRed  [2]}};
    assign vgaGreen[1:0] = {2{vgaGreen[2]}};
    assign vgaBlue [1  ] = {1{vgaBlue [2]}};

    tenyr_mainclock clocks(
        .reset     ( 1'b0     ), .locked       ( valid_clk ), .in ( clk ),
        .clk_core0 ( clk_core ), .clk_core0_CE ( valid_clk ),
        .clk_vga   ( clk_vga  ), .clk_vga_CE   ( valid_clk )
    );

    wire `HALTTYPE ihalt;
    assign ihalt[`HALT_TENYR   ] = ~valid_clk;
    assign ihalt[`HALT_EXTERNAL] = halt;
`ifdef SIM
    assign ihalt[`HALT_SIM     ] = 0;
`endif
    assign Led[7:0] = {'b0,ihalt};

    Core core(
        .clk    ( clk_core ), .en     ( valid_clk ), .reset_n ( _reset_n  ),
        .halt   ( ihalt    ), .i_addr ( insn_addr ), .i_data  ( insn_data ),
        .mem_rw ( oper_rw  ), .d_addr ( oper_addr ), .d_data  ( oper_data )
    );

// -----------------------------------------------------------------------------
// MEMORY ----------------------------------------------------------------------

    // TODO pull out constant or pull out RAM
    wire ram_inrange = oper_addr < 1024;
    GenedBlockMem ram(
        .clka  ( clk_core    ),  .clkb  ( clk_core  ),
        .ena   ( ram_inrange ),/*.enb   ( 1'b1      ),*/
        .addra ( oper_addr   ),  .addrb ( insn_addr ),
        .dina  ( oper_data   ),  .dinb  ( 'bx       ),
        .douta ( out_data    ),  .doutb ( insn_data ),
        .wea   ( oper_rw     ),  .web   ( 1'b0      )
    );

// -----------------------------------------------------------------------------
// DEVICES ---------------------------------------------------------------------

`ifdef SEG7
    Seg7 #(.BASE(12'h100)) seg7(
        .clk ( clk_core ), .reset_n ( _reset_n  ), .enable ( 1'b1      ),
        .rw  ( oper_rw  ), .addr    ( oper_addr ), .data   ( oper_data ),
        .seg ( seg      ), .an      ( an        )
    );
`endif

`ifdef VGA
    wire[11:0] ram_adA, rom_adA;
    wire[ 7:0] crx, cry, vga_ctl, ram_doA, rom_doA;

    mmr #(.ADDR(`VIDEO_ADDR), .MMR_WIDTH(8), .DEFAULT(8'b11110111)) video_ctl(
        .clk ( clk_core ), .reset_n ( _reset_n  ), .enable ( 1         ),
        .rw  ( oper_rw  ), .addr    ( oper_addr ), .data   ( oper_data ),
        .re  ( 1        ), .we      ( 0         ), .val    ( vga_ctl   )
    );

    mmr #(.ADDR(`VIDEO_ADDR + 1), .MMR_WIDTH(8), .DEFAULT(1)) crx_mmr(
        .clk ( clk_core ), .reset_n ( _reset_n  ), .enable ( 1         ),
        .rw  ( oper_rw  ), .addr    ( oper_addr ), .data   ( oper_data ),
        .re  ( 1        ), .we      ( 0         ), .val    ( crx       )
    ); // crx is 1-based ?

    mmr #(.ADDR(`VIDEO_ADDR + 2), .MMR_WIDTH(8), .DEFAULT(0)) cry_mmr(
        .clk ( clk_core ), .reset_n ( _reset_n  ), .enable ( 1         ),
        .rw  ( oper_rw  ), .addr    ( oper_addr ), .data   ( oper_data ),
        .re  ( 1        ), .we      ( 0         ), .val    ( cry       )
    ); // cry is 0-based ?

    vga80x40 vga(
        .clk25MHz ( clk_vga   ), .reset  ( ~_reset_n   ), .octl ( vga_ctl    ),
        .R        ( vgaRed[2] ), .G      ( vgaGreen[2] ), .B    ( vgaBlue[2] ),
        .hsync    ( hsync     ), .vsync  ( vsync       ),
        .TEXT_A   ( ram_adA   ), .TEXT_D ( ram_doA     ),
        .FONT_A   ( rom_adA   ), .FONT_D ( rom_doA     ),
        .ocrx     ( crx       ), .ocry   ( cry         )
    );

    ramwrap #(.BASE(`VIDEO_ADDR + 'h10), .SIZE(80 * 40)) text(
        .clka  ( clk_vga ), .clkb  ( clk_core  ),
        .addra ( ram_adA ), .addrb ( oper_addr ),
        .dina  ( 'bx     ), .dinb  ( oper_data ),
        .douta ( ram_doA ), .doutb ( oper_data ),
        .wea   ( 1'b0    ), .web   ( oper_rw   )
    );

    fontrom font( .clka ( clk_vga ), .addra ( rom_adA ), .douta ( rom_doA ) );
`endif

endmodule

