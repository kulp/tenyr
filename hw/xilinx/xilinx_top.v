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
    wire[31:0] insn_addr, oper_addr, insn_data, out_data;
    wire[31:0] oper_data = !oper_rw ? out_data : 32'bz;

    wire `HALTTYPE ihalt;
    assign ihalt[`HALT_TENYR   ] = ~valid_clk;
    assign ihalt[`HALT_EXTERNAL] = halt;
`ifdef SIM
    assign ihalt[`HALT_SIM     ] = 0;
`endif
    assign Led[7:0] = {'b0,ihalt};

    tenyr_mainclock clocks(
        .reset     ( 1'b0     ), .locked       ( valid_clk ), .in ( clk ),
        .clk_core0 ( clk_core ), .clk_core0_CE ( valid_clk ),
        .clk_vga   ( clk_vga  ), .clk_vga_CE   ( valid_clk )
    );

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
    VGAwrap vga(
        .clk_core ( clk_core ), .clk_vga  ( clk_vga   ),
        .enable   ( 1        ), .reset_n  ( _reset_n  ),
        .rw       ( oper_rw  ), .addr     ( oper_addr ), .data    ( oper_data ),
        .vgaRed   ( vgaRed   ), .vgaGreen ( vgaGreen  ), .vgaBlue ( vgaBlue   ),
        .hsync    ( hsync    ), .vsync    ( vsync     )
    );
`endif

endmodule

