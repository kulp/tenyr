`include "common.vh"
`timescale 1ns/10ps

module BlockRAM(
    input clka, ena, wea, input[ADDRBITS-1:0] addra, dina, output[31:0] douta,
    input clkb, enb, web, input[ADDRBITS-1:0] addrb, dinb, output[31:0] doutb
);

    parameter LOAD = 0;
    parameter LOADFILE = "default.memh";
    parameter ADDRBITS = 32;
    parameter BASE = 0; // TODO pull from environmental define
    parameter SIZE = 1024;

    reg[31:0] rdouta;
    reg[31:0] rdoutb;

    reg[31:0] store[(SIZE + BASE - 1):BASE];

    initial if (LOAD) $readmemh(LOADFILE, store, BASE);

    // TODO don't use comparators, use bitmasks
    wire a_inrange = (addra >= BASE && addra < SIZE + BASE);
    wire b_inrange = (addrb >= BASE && addrb < SIZE + BASE);

    assign douta = (!wea) ? rdouta : 32'bz;
    assign doutb = (!web) ? rdoutb : 32'bz;

    always @(posedge clka) begin
        if (ena && a_inrange)
            if (wea)
                store[addra] = dina;
            else
                rdouta = store[addra];
    end

    always @(posedge clkb) begin
        if (enb && b_inrange)
            if (web)
                store[addrb] = dinb;
            else
                rdoutb = store[addrb];
    end
    
endmodule

