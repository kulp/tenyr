`include "common.vh"
`timescale 1ns/10ps

// port B is address-offset, and padded to 32 bits
module ramwrap(
    input clka, ena, wea, clkb, enb, web,
    input[PBITS-1:0] addra, input[DBITS-1:0] dina, output[DBITS-1:0] douta,
    input[PBITS-1:0] addrb, input[DBITS-1:0] dinb, output[DBITS-1:0] doutb
);

    parameter INIT     = 1;
    parameter ZERO     = 0;
    parameter LOAD     = 0;
    parameter LOADFILE = "default.memh";
    parameter PBITS    = 32; // port address bits
    parameter ABITS    = 10; // internal address bits
    parameter DBITS    = 32;
    parameter SIZE     = 1 << ABITS;
    // BASEs are assumed to have ABITS of zeros
    parameter [PBITS-1:0] BASE_A = 0;
    parameter [PBITS-1:0] BASE_B = BASE_A;

    wire[DBITS-1:0] douta_internal, doutb_internal;

    assign douta = (a_inrange && !wea) ? douta_internal : 32'bz;
    assign doutb = (b_inrange && !web) ? doutb_internal : 32'bz;

`define IN_RANGE(Base,Addr) (Addr[PBITS-1:ABITS] == Base[PBITS-1:ABITS])

    reg a_inrange, b_inrange;
    always @(posedge clka) a_inrange <= `IN_RANGE(BASE_A,addra);
    always @(posedge clkb) b_inrange <= `IN_RANGE(BASE_B,addrb);

    BlockRAM #(
        .INIT(INIT), .ZERO(ZERO), .LOAD(LOAD), .LOADFILE(LOADFILE), .SIZE(SIZE),
        .PBITS(ABITS), .ABITS(ABITS), .DBITS(DBITS), .OFFSET(BASE_A)
    ) wrapped(
        .clka  ( clka             ), .clkb  ( clkb             ),
        .ena   ( ena & a_inrange  ), .enb   ( enb & b_inrange  ),
        .wea   ( wea              ), .web   ( web              ),
        .addra ( addra[ABITS-1:0] ), .addrb ( addrb[ABITS-1:0] ),
        .dina  ( dina             ), .dinb  ( dinb             ),
        .douta ( douta_internal   ), .doutb ( doutb_internal   )
    );

endmodule

