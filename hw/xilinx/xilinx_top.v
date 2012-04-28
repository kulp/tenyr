`include "common.vh"
`timescale 1ms/10us

`define VGA
`undef  SERIAL

// For now, we expose the serial wires txd and rxd as ports to keep ISE from
// optimising away the whole design. Once I figure out what I am doing, these
// should be replaced by whatever actual ports we end up with.
module Tenyr(halt,
`ifdef ISIM
        reset_n,
`endif
        clk, txd, rxd, seg, an, R, G, B, hsync, vsync);
    wire[31:0] insn_addr, operand_addr;
    wire[31:0] insn_data, in_data, out_data, operand_data;
    wire operand_rw;

    input clk /* synthesis .ispad = 1 */;

    `HALTTYPE halt;
    //wor ohalt;
    input rxd;
    output txd;

    output R, G, B;
    output hsync, vsync;

    output[7:0] seg;
    output[3:0] an;

    assign in_data      =  operand_rw ? operand_data : 32'bx;
    assign operand_data = !operand_rw ?     out_data : 32'bz;

    wire phases_valid;
    reg reset_n = 1;
    //input reset_n;
    wire clk_core0, clk_core90, clk_core180, clk_core270;
    /*
`ifdef ISIM
    assign clk_core0 = clk;
    assign #3 clk_core90 = clk;
    assign phases_valid = 1;
`else
*/
    // reset is active-high for tenyr_clocks
/*
    wire clk_core;
    tenyr_clocks clkdiv(.reset(~reset_n), .in(clk), .clk_vga(clk_vga), .clk_core(clk_core));
    */

    /*
    clock_phasing phaser(.reset(~reset_n), .valid(phases_valid),
                         .in(clk_core), .out0(clk_core0), .out90(clk_core90),
                         .out180(clk_core180), .out270(clk_core270));
    */
    wire clk_vga, clk_vga2;
    tenyr_mainclock clocks(.reset(/*~reset_n*/1'b0), .locked(phases_valid),
                           .in(clk), .clk_core0(clk_core0), .clk_core90(clk_core90),
                           .clk_vga(clk_vga), .clk_vga2(clk_vga2));

    wire vga_locked = phases_valid;
    //vga_clocker vga_clock(.CLK_IN1(clk), .CLK_OUT1(clk_vga), .LOCKED(vga_locked));

//`endif

    //assign ohalt = ~phases_valid;
    assign halt[`HALT_TENYR] = ~phases_valid;

    // active on posedge clock
    GenedBlockMem ram(.clka(clk_core0), .wea(operand_rw), .addra(operand_addr),
                      .dina(in_data), .douta(out_data),
                      .clkb(clk_core0), .web(1'b0), .addrb(insn_addr),
                      .dinb(32'bx), .doutb(insn_data));

`ifdef SERIAL
    Serial serial(.clk(clk_core), .reset_n(reset_n), .enable(1'b1), // XXX use halt ?
                  .rw(operand_rw), .addr(operand_addr),
                  .data(operand_data), .txd(txd), .rxd(rxd));
`endif

    Seg7 #(.BASE(12'h100))
             seg7(.clk(clk_core0), .reset_n(reset_n), .enable(1'b1), // XXX use halt ?
                  .rw(operand_rw), .addr(operand_addr),
                  .data(operand_data), .seg(seg), .an(an));

    Core core(.clk(clk_core0), .clkL(clk_core90), .en(phases_valid),
              .reset_n(reset_n), .rw(operand_rw),
              .norm_addr(operand_addr), .norm_data(operand_data),
              .insn_addr(insn_addr)   , .insn_data(insn_data), .halt(halt));

`ifndef SIM
`ifdef VGA

//`define USETEST

`ifdef USETEST
    // reset is active-high in vga80x40_test
    vga80x40_test vga(.reset(~reset_n), .clk50MHz(clk_vga2), .R(R), .G(G), .B(B),
                      .hsync(hsync), .vsync(vsync));
`else

    reg[7:0] crx_oreg = 8'd40;
    reg[7:0] cry_oreg = 8'd20;
    reg[7:0] ctl_oreg = 8'b11110010;
    reg crx_oreg_ce   = 1'b1;
    reg cry_oreg_ce   = 1'b1;
    reg ctl_oreg_ce   = 1'b1;

    wire[ 7:0] ram_diA;
    wire[ 7:0] ram_doA;
    wire[11:0] ram_adA;
    wire ram_weA;

    wire[ 7:0] ram_diB;
    wire[ 7:0] ram_doB;
    wire[11:0] ram_adB;
    wire ram_weB;

    wire[11:0] rom_adB;
    wire[ 7:0] rom_doB;

    vga80x40 vga(
        .reset       (~reset_n),
        .clk25MHz    (clk_vga),
        .R           (R),
        .G           (G),
        .B           (B),
        .hsync       (hsync),
        .vsync       (vsync),
        .TEXT_A      (ram_adB),
        .TEXT_D      (ram_doB),
        .FONT_A      (rom_adB),
        .FONT_D      (rom_doB),
        .ocrx        (crx_oreg),
        .ocry        (cry_oreg),
        .octl        (ctl_oreg)
    );

    textram text(
        .clka  (clk_vga),
        .dina  (ram_diA),
        .addra (ram_adA),
        .wea   (1'b0),
        .douta (ram_doA),
        .clkb  (clk_vga),
        .dinb  (ram_diB),
        .addrb (ram_adB),
        .web   (1'b0),
        .doutb (ram_doB)
    );

    fontrom font(
        .clka  (clk_vga),
        .addra (rom_adB),
        .douta (rom_doB)
    );

`endif

`endif
`endif

endmodule


