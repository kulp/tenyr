`include "common.vh"
`timescale 1ns/10ps

module BlockRAM(
    input clka, ena, wea, input[ABITS-1:0] addra, dina, output[DBITS-1:0] douta,
    input clkb, enb, web, input[ABITS-1:0] addrb, dinb, output[DBITS-1:0] doutb
);

    parameter LOAD     = 0;
    parameter LOADFILE = "default.memh";
    parameter ABITS    = 32;
    parameter DBITS    = 32;
    parameter BASE     = 0; // TODO pull from environmental define
    parameter SIZE     = 1024;

    reg[31:0] rdouta, rdoutb;
    reg[31:0] store[(SIZE + BASE - 1):BASE];

    initial if (LOAD) $readmemh(LOADFILE, store, BASE);

    // TODO don't use comparators, use bitmasks
    wire a_inrange = (addra >= BASE && addra < SIZE + BASE);
    wire b_inrange = (addrb >= BASE && addrb < SIZE + BASE);

    assign douta = (!wea && ena) ? rdouta : 'bz;
    assign doutb = (!web && enb) ? rdoutb : 'bz;

    always @(posedge clka) begin
        if (a_inrange)
            if (wea && ena)
                store[addra] <= dina;
            else
                rdouta <= store[addra];
    end

    always @(posedge clkb) begin
        if (b_inrange)
            if (web && enb)
                store[addrb] <= dinb;
            else
                rdoutb <= store[addrb];
    end
    
endmodule

