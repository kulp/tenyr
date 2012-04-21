`include "common.vh"
`timescale 1ms/10us

// For now, we expose the serial wires txd and rxd as ports to keep ISE from
// optimising away the whole design. Once I figure out what I am doing, these
// should be replaced by whatever actual ports we end up with.
module Tenyr(_halt/*, _reset*/, clk, txd, rxd, seg, an, R, G, B, hsync, vsync);
    wire[31:0] insn_addr, operand_addr;
    wire[31:0] insn_data, in_data, out_data, operand_data;
    wire operand_rw;

    input clk;
    input rxd;
    output txd;

    output R, G, B;
    output hsync, vsync;

    output[7:0] seg;
    output[3:0] an;

    assign in_data      =  operand_rw ? operand_data : 32'bx;
    assign operand_data = !operand_rw ?     out_data : 32'bz;

    reg _reset = 1;
    inout wor _halt;
`ifdef ISIM
    wire clk_core = clk;
`else
    // reset is active-high for tenyr_clocks
    tenyr_clocks clkdiv(.reset(~_reset), .in(clk), .clk_vga(clk_vga), .clk_core(clk_core));
`endif

    // active on posedge clock
    GenedBlockMem ram(.clka(clk_core), .wea(operand_rw), .addra(operand_addr),
                      .dina(in_data), .douta(out_data),
                      .clkb(clk_core), .web(1'b0), .addrb(insn_addr),
                      .dinb(32'bx), .doutb(insn_data));

/*
    Serial serial(.clk(clk_core), ._reset(_reset), .enable(1'b1), // XXX use halt ?
                  .rw(operand_rw), .addr(operand_addr),
                  .data(operand_data), .txd(txd), .rxd(rxd));
*/

    Seg7 #(.BASE(12'h100))
             seg7(.clk(clk_core), ._reset(_reset), .enable(1'b1), // XXX use halt ?
                  .rw(operand_rw), .addr(operand_addr),
                  .data(operand_data), .seg(seg), .an(an));

    Core core(.clk(clk_core), ._reset(_reset), .rw(operand_rw),
              .norm_addr(operand_addr), .norm_data(operand_data),
              .insn_addr(insn_addr)   , .insn_data(insn_data), .halt(_halt));

`ifndef ISIM
    // reset is active-high in vga80x40_test
    vga80x40_test vga(.reset(~_reset), .clk50MHz(clk_vga), .R(R), .G(G), .B(B),
                      .hsync(hsync), .vsync(vsync));
`endif

endmodule


