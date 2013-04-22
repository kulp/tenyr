`include "common.vh"
`timescale 1ms/10us

`ifndef SIM
`ifndef __QUARTUS__
  `define VGA
`endif
`endif
`define SEG7
`undef SERIAL

module Tenyr(
    input clk, reset, inout `HALTTYPE halt,
    output[7:0] Led, output[7:0] seg, output[3:0] an,
    output[2:0] vgaRed, vgaGreen, output[2:1] vgaBlue, output hsync, vsync
);

    parameter LOADFILE = "default.memh";

    wire oper_rw, oper_strobe;
    wire valid_clk, clk_vga, clk_core;
    wire[31:0] insn_addr, oper_addr, insn_data, out_data;
    wire[31:0] oper_data = (!oper_rw && oper_strobe) ? out_data : 32'bz;

    reg[3:0] startup = 0; // delay startup for a few clocks
    wire _reset_n = startup[3] & ~reset;
    always @(posedge clk_core) startup <= {startup,valid_clk};

    assign halt[`HALT_TENYR] = ~startup[3];
    assign Led[7:0] = halt;

    tenyr_mainclock clocks(
        .in     ( clk       ), .clk_core0    ( clk_core  ),
        .reset  ( 1'b0      ), .clk_core0_CE ( valid_clk ),
        .locked ( valid_clk ), .clk_vga      ( clk_vga   ),
                               .clk_vga_CE   ( valid_clk )
    );

    Core core(
        .clk    ( clk_core    ), .reset_n ( _reset_n  ), .mem_rw ( oper_rw   ),
        .strobe ( oper_strobe ), .i_addr  ( insn_addr ), .d_addr ( oper_addr ),
        .halt   ( halt        ), .i_data  ( insn_data ), .d_data ( oper_data )
    );

// -----------------------------------------------------------------------------
// MEMORY ----------------------------------------------------------------------

    ramwrap #(.LOAD(1), .LOADFILE(LOADFILE), .INIT(0), .SIZE(16384))
    ram(
        .clka  ( clk_core    ), .clkb  ( clk_core   ),
        .ena   ( oper_strobe ), .enb   ( startup[2] ),
        .wea   ( oper_rw     ), .web   ( 1'b0       ),
        .addra ( oper_addr   ), .addrb ( insn_addr  ),
        .dina  ( oper_data   ), .dinb  ( 32'bz      ),
        .douta ( out_data    ), .doutb ( insn_data  )
    );

// -----------------------------------------------------------------------------
// DEVICES ---------------------------------------------------------------------

`ifdef SERIAL
    // TODO xilinx-compatible serial device ; rename to eliminate `Sim`
    SimWrap_simserial #(.BASE(12'h20), .SIZE(2)) serial(
        .clk ( clk     ), .reset_n ( reset_n   ), .enable ( oper_strobe ),
        .rw  ( oper_rw ), .addr    ( oper_addr ), .data   ( oper_data   )
    );
`endif

`ifdef SEG7
    Seg7 #(.BASE(12'h100)) seg7(
        .clk     ( clk_core    ), .rw   ( oper_rw   ), .seg ( seg ),
        .reset_n ( _reset_n    ), .addr ( oper_addr ), .an  ( an  ),
        .enable  ( oper_strobe ), .data ( oper_data )
    );
`endif

`ifdef VGA
    VGAwrap vga(
        .clk_core ( clk_core ), .rw     ( oper_rw     ), .vgaRed   ( vgaRed   ),
        .clk_vga  ( clk_vga  ), .addr   ( oper_addr   ), .vgaGreen ( vgaGreen ),
        .en       ( 1        ), .data   ( oper_data   ), .vgaBlue  ( vgaBlue  ),
        .reset_n  ( _reset_n ), .strobe ( oper_strobe ), .hsync    ( hsync    ),
                                                         .vsync    ( vsync    )
    );
`endif

endmodule

