`include "common.vh"
`timescale 1ms/10us

`ifndef SIM
`define VGA
`endif
`define SEG7

module Tenyr(
    input clk, reset, inout `HALTTYPE halt,
    output[7:0] Led, output[7:0] seg, output[3:0] an,
    output[2:0] vgaRed, vgaGreen, output[2:1] vgaBlue, output hsync, vsync
);

    wire oper_rw;
    wire valid_clk, clk_vga, clk_core;
    wire _reset_n = valid_clk & ~reset;
    wire[31:0] insn_addr, oper_addr, insn_data, out_data;
    wire[31:0] oper_data = !oper_rw ? out_data : 32'bz;

    assign halt[`HALT_TENYR] = ~valid_clk;
    assign Led[7:0] = halt;

    tenyr_mainclock clocks(
        .in     ( clk       ), .clk_core0    ( clk_core  ),
        .reset  ( 1'b0      ), .clk_core0_CE ( valid_clk ),
        .locked ( valid_clk ), .clk_vga      ( clk_vga   ),
                               .clk_vga_CE   ( valid_clk )
    );

    Core core(
        .clk  ( clk_core  ), .reset_n ( _reset_n  ), .mem_rw ( oper_rw   ),
        .en   ( valid_clk ), .i_addr  ( insn_addr ), .d_addr ( oper_addr ),
        .halt ( halt      ), .i_data  ( insn_data ), .d_data ( oper_data )
    );

// -----------------------------------------------------------------------------
// MEMORY ----------------------------------------------------------------------

    // TODO pull out constant or pull out RAM
    wire ram_inrange = oper_addr < 1024;
    GenedBlockMem ram(
        .clka  ( clk_core    ),  .clkb  ( clk_core  ),
        .ena   ( ram_inrange ),/*.enb   ( 1'b1      ),*/
        .wea   ( oper_rw     ),  .web   ( 1'b0      ),
        .addra ( oper_addr   ),  .addrb ( insn_addr ),
        .dina  ( oper_data   ),  .dinb  ( 'bx       ),
        .douta ( out_data    ),  .doutb ( insn_data )
    );

// -----------------------------------------------------------------------------
// DEVICES ---------------------------------------------------------------------

`ifdef SEG7
    Seg7 #(.BASE(12'h100)) seg7(
        .clk     ( clk_core ), .rw   ( oper_rw   ), .seg ( seg ),
        .reset_n ( _reset_n ), .addr ( oper_addr ), .an  ( an  ),
        .enable  ( 1'b1     ), .data ( oper_data )
    );
`endif

`ifdef VGA
    VGAwrap vga(
        .clk_core ( clk_core ), .rw   ( oper_rw   ), .vgaRed   ( vgaRed   ),
        .clk_vga  ( clk_vga  ), .addr ( oper_addr ), .vgaGreen ( vgaGreen ),
        .enable   ( 1        ), .data ( oper_data ), .vgaBlue  ( vgaBlue  ),
        .reset_n  ( _reset_n ),                      .hsync    ( hsync    ),
                                                     .vsync    ( vsync    )
    );
`endif

endmodule

