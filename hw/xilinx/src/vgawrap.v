`include "common.vh"
`timescale 1ns/10ps

module VGAwrap(
    input clk_core, clk_vga, en, rw, reset,
    input strobe, input[31:0] addr, input[31:0] d_in, output wor[31:0] d_out,
    output[2:0] vgaRed, vgaGreen, output[2:1] vgaBlue, output hsync, vsync
);

    parameter[31:0] VIDEO_ADDR = `VIDEO_ADDR;
    localparam[31:0] ROWS = 32, COLS = 64;
    localparam[31:0] FONT_ROWS = 15, FONT_COLS = 10;

    wire[11:0] ram_adA, rom_adA;
    wire[ 9:0] ram_doA, rom_doA;

    assign vgaRed  [1:0] = {2{vgaRed  [2]}};
    assign vgaGreen[1:0] = {2{vgaGreen[2]}};
    assign vgaBlue [1  ] = {1{vgaBlue [2]}};

    vga64x32 vga(
        .clk      ( clk_vga   ), .reset  ( reset       ),
        .R        ( vgaRed[2] ), .G      ( vgaGreen[2] ), .B ( vgaBlue[2] ),
        .hsync    ( hsync     ), .vsync  ( vsync       ),
        .TEXT_A   ( ram_adA   ), .TEXT_D ( ram_doA     ),
        .FONT_A   ( rom_adA   ), .FONT_D ( rom_doA     )
    );

    BlockRAM #(
        .SIZE(ROWS * COLS), .ABITS($clog2(ROWS * COLS) + 1), .DBITS(8), .INIT(1), .ZERO('h20)
    ) text(
        .clka  ( clk_vga ), .clkb  ( clk_core ),
        .ena   ( 1'b1    ), .enb   ( strobe   ),
        .addra ( ram_adA ), .addrb ( addr     ),
        .dina  ( 8'bx    ), .dinb  ( d_in     ),
        .douta ( ram_doA ), .doutb ( d_out    ),
        .wea   ( 1'b0    ), .web   ( rw       )
    );

    BlockRAM #(.LOADB(1), .LOADFILE("../../rsrc/font10x15/font10x15.memb"),
               .SIZE(256 * FONT_ROWS), .DBITS(FONT_COLS))
    font(
        .clka  ( clk_vga ), .ena   ( 1'b1    ), .wea  ( 1'b0 ),
        .addra ( rom_adA ), .douta ( rom_doA ), .clkb ( 1'b0 )
    );

endmodule
