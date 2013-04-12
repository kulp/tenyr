`include "common.vh"
`timescale 1ns/10ps

module `SIMMEM(
    input clka, ena, wea, input[31:0] addra, dina, output[31:0] douta,
    input clkb, enb, web, input[31:0] addrb, dinb, output[31:0] doutb
);

    parameter ADDRBITS = 24;
    parameter BASE = `RESETVECTOR; // TODO pull from environmental define
    parameter SIZE = (1 << ADDRBITS) - BASE;

    reg[31:0] rdouta;
    reg[31:0] rdoutb;

    reg[31:0] store[(SIZE + BASE - 1):BASE];

    wire a_inrange = (addra >= BASE && addra < SIZE + BASE);
    wire b_inrange = (addrb >= BASE && addrb < SIZE + BASE);

    assign douta = (!wea) ? rdouta : 32'bz;
    assign doutb = (!web) ? rdoutb : 32'bz;

    // Xilinx generated block RAMs use posedge
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

