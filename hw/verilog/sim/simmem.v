`include "common.vh"
`timescale 1ns/10ps

module SimMem(clka, wea, addra, dina, douta,
              clkb, web, addrb, dinb, doutb);

    parameter ADDRBITS = 24;
    parameter BASE = 1 << 12; // TODO pull from environmental define
    parameter SIZE = (1 << ADDRBITS) - BASE;

    input clka;
    input wea;
    input[31:0] addra;
    input[31:0] dina;
    output[31:0] douta;
    input clkb;
    input web;
    input[31:0] addrb;
    input[31:0] dinb;
    output[31:0] doutb;

    reg[31:0] rdouta;
    reg[31:0] rdoutb;

    reg[31:0] store[(SIZE + BASE - 1):BASE];

    wire a_inrange = (addra >= BASE && addra < SIZE + BASE);
    wire b_inrange = (addrb >= BASE && addrb < SIZE + BASE);

    assign douta = (!wea) ? rdouta : 32'bz;
    assign doutb = (!web) ? rdoutb : 32'bz;

    // For consistency with the Xilinx generated block RAMs, use posedge
    always @(posedge clka) begin
        if (a_inrange)
            if (wea)
                store[addra] = dina;
            else
                rdouta = store[addra];
    end

    always @(posedge clkb) begin
        if (b_inrange)
            if (web)
                store[addrb] = dinb;
            else
                rdoutb = store[addrb];
    end


endmodule

