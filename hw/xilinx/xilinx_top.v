`include "common.vh"
`timescale 1ms/10us

`ifndef SIM
`define VGA
`endif
`define SEG7

module Tenyr(halt, clk, txd, rxd, seg, an, vgaRed, vgaGreen, vgaBlue, hsync, vsync, Led);
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
    reg reset_n = 1;
    wire clk_vga;
    `ifdef OLDCLOCK
    wire clk_core0, clk_core90, clk_core180, clk_core270;
    tenyr_mainclock clocks(.reset(/*~reset_n*/1'b0), .locked(phases_valid),
                           .in(clk),
                           .clk_core0(clk_core0), .clk_core0_CE(phases_valid),
                           .clk_core90(clk_core90), .clk_core90_CE(phases_valid),
                           .clk_core180(clk_core180), .clk_core180_CE(phases_valid),
                           .clk_core270(clk_core270), .clk_core270_CE(phases_valid),
                           .clk_vga(clk_vga), .clk_vga_CE(phases_valid));
    `else
    tenyr_mainclock clocks(.reset(/*~reset_n*/1'b0), .locked(phases_valid),
                           .in(clk),
                           .clk_core0(clk_core_base), .clk_core0_CE(phases_valid),
                           .clk_vga(clk_vga), .clk_vga_CE(phases_valid));
    reg clk_core0   = 1,
        clk_core90  = 0,
        clk_core180 = 0,
        clk_core270 = 0;
    `endif
    wire clk_datamem = clk_core180;
    wire clk_insnmem = clk_core0;

    always @(negedge clk_core_base) begin
        {clk_core270,clk_core180,clk_core90,clk_core0} = {clk_core180,clk_core90,clk_core0,clk_core270};
    end

    assign halt[`HALT_TENYR] = ~phases_valid;
    assign Led[2:0] = halt;

    // TODO pull out constant or pull out RAM
    wire ram_inrange = operand_addr < 1024;
    // active on posedge clock
    GenedBlockMem ram(.clka(~clk_datamem),
                      .ena(ram_inrange), .wea(operand_rw), .addra(operand_addr),
                      .dina(in_data), .douta(out_data),
                      .clkb(~clk_insnmem),
                      /*.enb(1'b1),*/ .web(1'b0), .addrb(insn_addr),
                      .dinb(32'bx), .doutb(insn_data));

`ifdef SEG7
    Seg7 #(.BASE(12'h100))
             seg7(.clk(clk_datamem), .reset_n(reset_n), .enable(1'b1), // XXX use halt ?
                  .rw(operand_rw), .addr(operand_addr),
                  .data(operand_data), .seg(seg), .an(an));
`endif

    Core core(.clk0(clk_core0), .clk90(clk_core90), .clk180(clk_core180), .clk270(clk_core270),
              .en(phases_valid),
              .reset_n(reset_n), .rw(operand_rw),
              .norm_addr(operand_addr), .norm_data(operand_data),
              .insn_addr(insn_addr)   , .insn_data(insn_data), .halt(halt));

`ifdef VGA

    wire[7:0] crx; // 1-based ?
    wire[7:0] cry; // 0-based ?
    wire[7:0] vga_ctl;

    mmr #(.ADDR(`VIDEO_ADDR), .MMR_WIDTH(8), .DEFAULT(8'b11110111))
        video_ctl(.clk(clk_datamem), .reset_n(reset_n), .enable(1),
                  .rw(operand_rw), .addr(operand_addr), .data(operand_data),
                  .re(1), .we(0), .val(vga_ctl));

    mmr #(.ADDR(`VIDEO_ADDR + 1), .MMR_WIDTH(8), .DEFAULT(1))
        crx_mmr(.clk(clk_datamem), .reset_n(reset_n), .enable(1),
                .rw(operand_rw), .addr(operand_addr), .data(operand_data),
                .re(1), .we(0), .val(crx));

    mmr #(.ADDR(`VIDEO_ADDR + 2), .MMR_WIDTH(8), .DEFAULT(0))
        cry_mmr(.clk(clk_datamem), .reset_n(reset_n), .enable(1),
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
        .douta (ram_doA),
        .wea   (1'b0),
`ifdef DISABLE_TEXTRAM
        .clkb  ('b0),
        .dinb  ('bz),
        .addrb ('bz),
        .web   ('b0),
        .doutb (nonce_doutb)
`else
        .clkb  (clk_datamem),
        .dinb  (operand_data),
        .addrb (operand_addr),
        .web   (operand_rw),
        .doutb (operand_data)
`endif
    );

    fontrom font(
        .clka  (clk_vga),
        .addra (rom_adA),
        .douta (rom_doA)
    );

`endif

endmodule


