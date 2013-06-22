`include "common.vh"
`timescale 1ns/10ps

// port B is address-offset, and padded to 32 bits
module ramwrap(
    input clka, ena, wea, clkb, enb, web,
    input[ABITS-1:0] addra, input[DBITS-1:0] dina, output[DBITS-1:0] douta,
    input[ABITS-1:0] addrb, input[DBITS-1:0] dinb, output[DBITS-1:0] doutb
);

    parameter INIT     = 1;
    parameter ZERO     = 0;
    parameter LOAD     = 0;
    parameter LOADFILE = "default.memh";
    parameter ABITS    = 32;
    parameter DBITS    = 32;
    parameter SIZE     = 1024;
    parameter BASE_A   = 0;
    parameter BASE_B   = BASE_A;

    wire[DBITS-1:0] douta_internal, doutb_internal;

    assign douta = (a_inrange && !wea) ? douta_internal : 32'bz;
    assign doutb = (b_inrange && !web) ? doutb_internal : 32'bz;

    reg a_inrange, b_inrange;
    always @(posedge clka)
        a_inrange <= addra >= BASE_A && addra < SIZE + BASE_A;
    always @(posedge clkb)
        b_inrange <= addrb >= BASE_B && addrb < SIZE + BASE_B;

    BlockRAM #(
        .INIT(INIT), .ZERO(ZERO), .LOAD(LOAD), .LOADFILE(LOADFILE),
        .ABITS(ABITS), .DBITS(DBITS), .SIZE(SIZE)
    ) wrapped(
        .clka  ( clka            ), .clkb  ( clkb            ),
        .ena   ( ena & a_inrange ), .enb   ( enb & b_inrange ),
        .wea   ( wea & a_inrange ), .web   ( web & b_inrange ),
        .addra ( addra - BASE_A  ), .addrb ( addrb - BASE_B  ),
        .dina  ( dina            ), .dinb  ( dinb            ),
        .douta ( douta_internal  ), .doutb ( doutb_internal  )
    );

endmodule

