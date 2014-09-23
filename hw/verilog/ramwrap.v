`include "common.vh"
`timescale 1ns/10ps

// port B is address-offset, and padded to 32 bits
module ramwrap(
    input clka, ena, wea, clkb, enb, web, output acta, actb,
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
    // BASEs are assumed to have ABITS-worth of zeros in the LSBs
    parameter [PBITS-1:0] BASE_A = 0;
    parameter [PBITS-1:0] BASE_B = BASE_A;

    wire[DBITS-1:0] douta_internal, doutb_internal;

    assign douta = (acta && !wea) ? douta_internal : 32'bz;
    assign doutb = (actb && !web) ? doutb_internal : 32'bz;

`define APROPOS(Base,Addr) `IN_RANGE(PBITS,ABITS,Base,Addr)

    assign acta = `APROPOS(BASE_A,addra);
    assign actb = `APROPOS(BASE_B,addrb);

    BlockRAM #(
        .INIT(INIT), .ZERO(ZERO), .LOAD(LOAD), .LOADFILE(LOADFILE), .SIZE(SIZE),
        .PBITS(ABITS), .ABITS(ABITS), .DBITS(DBITS), .OFFSET(BASE_A)
    ) wrapped(
        .clka  ( clka             ), .clkb  ( clkb             ),
        .ena   ( ena & acta       ), .enb   ( enb & actb       ),
        .wea   ( wea              ), .web   ( web              ),
        .addra ( addra[ABITS-1:0] ), .addrb ( addrb[ABITS-1:0] ),
        .dina  ( dina             ), .dinb  ( dinb             ),
        .douta ( douta_internal   ), .doutb ( doutb_internal   )
    );

endmodule

