`include "common.vh"
`timescale 1ns/10ps

module BlockRAM(
    input clka, ena, wea,
    input[ABITS-1:0] addra, input[DBITS-1:0] dina, output[DBITS-1:0] douta,
    input clkb, enb, web,
    input[ABITS-1:0] addrb, input[DBITS-1:0] dinb, output[DBITS-1:0] doutb
);

    parameter LOAD     = 0;
    parameter LOADFILE = "default.memh";
    parameter ABITS    = 32;
    parameter DBITS    = 32;
    parameter BASE_A   = 0; // TODO pull from environmental define
    parameter BASE_B   = BASE_A;
    parameter SIZE     = 1024;

    reg[DBITS-1:0] rdouta, rdoutb;
    reg[DBITS-1:0] raddra, raddrb;
    reg[DBITS-1:0] store[(SIZE + BASE_A - 1):BASE_A];

    initial if (LOAD) $readmemh(LOADFILE, store, BASE_A);

    // TODO don't use comparators, use bitmasks
    wire a_inrange = (addra >= BASE_A && addra < SIZE + BASE_A);
    wire b_inrange = (addrb >= BASE_B && addrb < SIZE + BASE_B);

    assign douta = (!wea && ena) ? rdouta : 'bz;
    assign doutb = (!web && enb) ? rdoutb : 'bz;

    always @(posedge clka) begin
        if (a_inrange)
            if (wea && ena)
                store[addra - BASE_A] <= dina;
            else
                rdouta <= store[addra - BASE_A];
    end

    always @(posedge clkb) begin
        if (b_inrange)
            if (web && enb)
                store[addrb - BASE_B] <= dinb;
            else
                rdoutb <= store[addrb - BASE_B];
    end

endmodule

