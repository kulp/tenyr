`include "common.vh"
`timescale 1ns/10ps

module VGAwrap(
    input clk_core, clk_vga, en, rw, reset_n,
    input strobe, input[31:0] addr, input[31:0] d_in, // TODO allow reads
    output[2:0] vgaRed, vgaGreen, output[2:1] vgaBlue, output hsync, vsync
);

    parameter[31:0] VIDEO_ADDR = `VIDEO_ADDR;
    localparam[31:0] ROWS = 40, COLS = 80;

    wire[11:0] ram_adA, rom_adA;
    wire[ 7:0] crx, cry, vga_ctl, ram_doA, rom_doA;

    assign vgaRed  [1:0] = {2{vgaRed  [2]}};
    assign vgaGreen[1:0] = {2{vgaGreen[2]}};
    assign vgaBlue [1  ] = {1{vgaBlue [2]}};

    mmr #(.ADDR(VIDEO_ADDR + 'h1000 - 1), .DBITS(8), .DEFAULT(8'b11110111)) video_ctl(
        .clk ( clk_core ), .reset_n ( reset_n ), .strobe ( strobe  ),
        .rw  ( rw       ), .addr    ( addr    ), .d_in   ( d_in    ),
        .re  ( 1'b1     ), .we      ( 1'b0    ), .val    ( vga_ctl )
    );

    mmr #(.ADDR(VIDEO_ADDR + 'h1000 - 2), .DBITS(7), .DEFAULT(1)) crx_mmr(
        .clk ( clk_core ), .reset_n ( reset_n ), .strobe ( strobe ),
        .rw  ( rw       ), .addr    ( addr    ), .d_in   ( d_in   ),
        .re  ( 1'b1     ), .we      ( 1'b0    ), .val    ( crx    )
    ); // crx is 1-based ?

    mmr #(.ADDR(VIDEO_ADDR + 'h1000 - 3), .DBITS(6), .DEFAULT(0)) cry_mmr(
        .clk ( clk_core ), .reset_n ( reset_n ), .strobe ( strobe ),
        .rw  ( rw       ), .addr    ( addr    ), .d_in   ( d_in   ),
        .re  ( 1'b1     ), .we      ( 1'b0    ), .val    ( cry    )
    ); // cry is 0-based ?

    vga80x40 vga(
        .clk25MHz ( clk_vga   ), .reset  ( ~reset_n    ), .octl ( vga_ctl    ),
        .R        ( vgaRed[2] ), .G      ( vgaGreen[2] ), .B    ( vgaBlue[2] ),
        .hsync    ( hsync     ), .vsync  ( vsync       ),
        .TEXT_A   ( ram_adA   ), .TEXT_D ( ram_doA     ),
        .FONT_A   ( rom_adA   ), .FONT_D ( rom_doA     ),
        .ocrx     ( crx       ), .ocry   ( cry         )
    );

    ramwrap #(
        .BASE_B(VIDEO_ADDR), .SIZE(ROWS * COLS),
        .ABITS(12), .DBITS(8), .INIT(1), .ZERO('h20)
    ) text(
        .clka  ( clk_vga ), .clkb  ( clk_core ),
        .ena   ( 1'b1    ), .enb   ( 1'b1     ),
        .addra ( ram_adA ), .addrb ( addr     ),
        .dina  ( 8'bx    ), .dinb  ( d_in     ),
        .douta ( ram_doA ), //.doutb ( d_out    ),
        .wea   ( 1'b0    ), .web   ( rw       )
    );

    BlockRAM #(.LOAD(1), .LOADFILE("lat0-12.memh"),
               .SIZE(256 * 12), .DBITS(8))
    font(
        .clka  ( clk_vga ), .ena   ( 1'b1    ), .wea  ( 1'b0 ),
        .addra ( rom_adA ), .douta ( rom_doA ), .clkb ( 1'b0 )
    );

endmodule
