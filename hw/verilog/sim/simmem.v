`include "common.vh"
`timescale 1ns/10ps

module SimMem(input clka, wea, input[31:0] addra, dina, output[31:0] douta,
              input clkb, web, input[31:0] addrb, dinb, output[31:0] doutb);

    parameter ADDRBITS = 24;
    parameter BASE = 1 << 12; // TODO pull from environmental define
    parameter SIZE = (1 << ADDRBITS) - BASE;

    reg[31:0] rdouta;
    reg[31:0] rdoutb;

    reg[31:0] store[(SIZE + BASE - 1):BASE];

    wire a_inrange = (addra >= BASE && addra < SIZE + BASE);
    wire b_inrange = (addrb >= BASE && addrb < SIZE + BASE);

    assign douta = (!wea) ? rdouta : 32'bz;
    assign doutb = (!web) ? rdoutb : 32'bz;

    // Xilinx generated block RAMs use posedge ; we use nege
    always @(`EDGE clka) begin
        if (a_inrange)
            if (wea)
                store[addra] = dina;
            else
                rdouta = store[addra];
    end

    always @(`EDGE clkb) begin
        if (b_inrange)
            if (web)
                store[addrb] = dinb;
            else
                rdoutb = store[addrb];
    end

endmodule

