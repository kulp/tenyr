`include "common.vh"
`timescale 1ns/10ps

// port B is address-offset, and padded to 32 bits
module ramwrap(clka, dina, addra, wea, douta, clkb, dinb, addrb, web, doutb);

    parameter BASE = 0;
    parameter SIZE = 1024;

    input clka;
    input wea;
    input[7:0] dina;
    output[7:0] douta;
    input[11:0] addra;

    input clkb;
    input web;
    input[31:0] dinb;
    output[31:0] doutb;
    input[31:0] addrb;

    wire[31:0] doutb_internal;

    wire in_range = (addrb >= BASE && addrb < BASE + SIZE);
    assign doutb = in_range && !web ? doutb_internal : 32'bz;

    textram wrapped(
        .clka  (clka),
        .dina  (dina),
        .addra (addra),
        .wea   (wea),
        .douta (douta),
        .clkb  (clkb),
        .dinb  (dinb),
        // XXX when this subtractor was introduced, maximum speed dropped from
        // 58MHz to 48MHz
        .addrb (addrb - BASE),
        .web   (web & in_range),
        .doutb (doutb_internal)
    );

endmodule

