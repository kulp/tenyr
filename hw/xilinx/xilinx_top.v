`include "common.vh"
`timescale 1ms/10us

`define VGA
`undef  SERIAL

module Tenyr(halt,
`ifdef ISIM
        reset_n,
`endif
        clk, txd, rxd, seg, an, vgaRed, vgaGreen, vgaBlue, hsync, vsync, Led);
    wire[31:0] insn_addr, operand_addr;
    wire[31:0] insn_data, in_data, out_data, operand_data;
    wire operand_rw;

    input clk /* synthesis .ispad = 1 */;

    `HALTTYPE halt;
    input rxd;
    output txd;

    output[2:0] vgaRed;
    output[2:0] vgaGreen;
    output[2:1] vgaBlue;
    assign vgaRed[1:0] = {2{vgaRed[2]}};
    assign vgaGreen[1:0] = {2{vgaGreen[2]}};
    assign vgaBlue[1] = {1{vgaBlue[2]}};

    output hsync, vsync;

    output[7:0] seg;
    output[3:0] an;

    output[7:0] Led;
    assign Led[7:3] = 5'b00000;

    assign in_data      =  operand_rw ? operand_data : 32'bx;
    assign operand_data = !operand_rw ?     out_data : 32'bz;

    wire phases_valid;
`ifdef ISIM
    input reset_n;
`else
    reg reset_n = 1;
`endif
    wire clk_core0, clk_core90, clk_core180, clk_core270;
	assign clk_core180 = ~clk_core0;
    wire clk_vga;
    tenyr_mainclock clocks(.reset(/*~reset_n*/1'b0), .locked(phases_valid),
                           .in(clk),
                           .clk_core0(clk_core0), .clk_core0_CE(phases_valid),
                           .clk_core90(clk_core90), .clk_core90_CE(phases_valid),
                           .clk_vga(clk_vga), .clk_vga_CE(phases_valid));

    assign halt[`HALT_TENYR] = ~phases_valid;
    assign Led[2:0] = halt;

    // active on posedge clock
    GenedBlockMem ram(.clka(~clk_core180), .wea(operand_rw), .addra(operand_addr),
                      .dina(in_data), .douta(out_data),
                      .clkb(~clk_core0), .web(1'b0), .addrb(insn_addr),
                      .dinb(32'bx), .doutb(insn_data));

`ifdef SERIAL
    Serial serial(.clk(clk_core0), .reset_n(reset_n), .enable(1'b1), // XXX use halt ?
                  .rw(operand_rw), .addr(operand_addr),
                  .data(operand_data), .txd(txd), .rxd(rxd));
    Serial serial2(.clk(clk_core0), .reset_n(reset_n), .enable(1'b1), // XXX use halt ?
                  .rw(operand_rw), .addr(operand_addr),
                  .data(operand_data), .rxd(txd));
`endif

    Seg7 #(.BASE(12'h100))
             seg7(.clk(clk_core90), .reset_n(reset_n), .enable(1'b1), // XXX use halt ?
                  .rw(operand_rw), .addr(operand_addr),
                  .data(operand_data), .seg(seg), .an(an));

    Core core(.clk(clk_core0), .clkL(clk_core90), .en(phases_valid),
              .reset_n(reset_n), .rw(operand_rw),
              .norm_addr(operand_addr), .norm_data(operand_data),
              .insn_addr(insn_addr)   , .insn_data(insn_data), .halt(halt));

`ifndef SIM
`ifdef VGA

    wire[7:0] crx; // 1-based ?
    wire[7:0] cry; // 0-based ?
    wire[7:0] vga_ctl;

    mmr #(.ADDR(`VIDEO_ADDR), .MMR_WIDTH(8), .DEFAULT(8'b11110111))
        video_ctl(.clk(clk_core0), .reset_n(reset_n), .enable(1),
                  .rw(operand_rw), .addr(operand_addr), .data(operand_data),
                  .re(1), .we(0), .val(vga_ctl));

    mmr #(.ADDR(`VIDEO_ADDR + 1), .MMR_WIDTH(8), .DEFAULT(1))
        crx_mmr(.clk(clk_core0), .reset_n(reset_n), .enable(1),
                .rw(operand_rw), .addr(operand_addr), .data(operand_data),
                .re(1), .we(0), .val(crx));

    mmr #(.ADDR(`VIDEO_ADDR + 2), .MMR_WIDTH(8), .DEFAULT(0))
        cry_mmr(.clk(clk_core0), .reset_n(reset_n), .enable(1),
                .rw(operand_rw), .addr(operand_addr), .data(operand_data),
                .re(1), .we(0), .val(cry));

    wire[ 7:0] ram_doA;
    wire[11:0] ram_adA;

    wire[11:0] rom_adA;
    wire[ 7:0] rom_doA;

    vga80x40 vga(
        .reset       (~reset_n),
        .clk25MHz    (clk_vga),
        .R           (vgaRed[2]),
        .G           (vgaGreen[2]),
        .B           (vgaBlue[2]),
        .hsync       (hsync),
        .vsync       (vsync),
        .TEXT_A      (ram_adA),
        .TEXT_D      (ram_doA),
        .FONT_A      (rom_adA),
        .FONT_D      (rom_doA),
        .ocrx        (crx),
        .ocry        (cry),
        .octl        (vga_ctl)
    );

    ramwrap #(.BASE(`VIDEO_ADDR + 'h10), .SIZE(80 * 40)) text(
        .clka  (clk_vga),
        .dina  ('bz),
        .addra (ram_adA),
        .wea   (1'b0),
        .douta (ram_doA),
        .clkb  (clk_core0),
        .dinb  (operand_data),
        .addrb (operand_addr),
        .web   (operand_rw),
        .doutb (operand_data)
    );

    fontrom font(
        .clka  (clk_vga),
        .addra (rom_adA),
        .douta (rom_doA)
    );

`endif
`endif

endmodule


