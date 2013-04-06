`include "common.vh"
`timescale 1ns/10ps

module VGAwrap(
	input clk_core, clk_vga, enable, rw, reset_n, input[31:0] addr, data,
    output[2:0] vgaRed, vgaGreen, output[2:1] vgaBlue, output hsync, vsync
);

	parameter VIDEO_ADDR = `VIDEO_ADDR;

    wire[11:0] ram_adA, rom_adA;
    wire[ 7:0] crx, cry, vga_ctl, ram_doA, rom_doA;

    assign vgaRed  [1:0] = {2{vgaRed  [2]}};
    assign vgaGreen[1:0] = {2{vgaGreen[2]}};
    assign vgaBlue [1  ] = {1{vgaBlue [2]}};

    mmr #(.ADDR(VIDEO_ADDR), .MMR_WIDTH(8), .DEFAULT(8'b11110111)) video_ctl(
        .clk ( clk_core ), .reset_n ( reset_n ), .enable ( 1       ),
        .rw  ( rw       ), .addr    ( addr    ), .data   ( data    ),
        .re  ( 1        ), .we      ( 0       ), .val    ( vga_ctl )
    );

    mmr #(.ADDR(VIDEO_ADDR + 1), .MMR_WIDTH(8), .DEFAULT(1)) crx_mmr(
        .clk ( clk_core ), .reset_n ( reset_n ), .enable ( 1    ),
        .rw  ( rw       ), .addr    ( addr    ), .data   ( data ),
        .re  ( 1        ), .we      ( 0       ), .val    ( crx  )
    ); // crx is 1-based ?

    mmr #(.ADDR(VIDEO_ADDR + 2), .MMR_WIDTH(8), .DEFAULT(0)) cry_mmr(
        .clk ( clk_core ), .reset_n ( reset_n ), .enable ( 1    ),
        .rw  ( rw       ), .addr    ( addr    ), .data   ( data ),
        .re  ( 1        ), .we      ( 0       ), .val    ( cry  )
    ); // cry is 0-based ?

    vga80x40 vga(
        .clk25MHz ( clk_vga   ), .reset  ( ~reset_n    ), .octl ( vga_ctl    ),
        .R        ( vgaRed[2] ), .G      ( vgaGreen[2] ), .B    ( vgaBlue[2] ),
        .hsync    ( hsync     ), .vsync  ( vsync       ),
        .TEXT_A   ( ram_adA   ), .TEXT_D ( ram_doA     ),
        .FONT_A   ( rom_adA   ), .FONT_D ( rom_doA     ),
        .ocrx     ( crx       ), .ocry   ( cry         )
    );

    ramwrap #(.BASE(VIDEO_ADDR + 'h10), .SIZE(80 * 40)) text(
        .clka  ( clk_vga ), .clkb  ( clk_core ),
        .addra ( ram_adA ), .addrb ( addr     ),
        .dina  ( 'bx     ), .dinb  ( data     ),
        .douta ( ram_doA ), .doutb ( data     ),
        .wea   ( 1'b0    ), .web   ( rw       )
    );

    fontrom font( .clka ( clk_vga ), .addra ( rom_adA ), .douta ( rom_doA ) );

endmodule
