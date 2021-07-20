`include "common.vh"
`timescale 1ns/10ps

module TwoPortRAM(
    clka, ena, wea, clkb, enb, web, acka, ackb,
    addra, dina, douta, addrb, dinb, doutb
);

    parameter INIT     = 0;
    parameter ZERO     = 0;
    parameter LOADH    = 0;
    parameter LOADB    = 0;
    parameter LOADFILE = "default.memh";
    parameter PBITS    = 32; // port address bits
    parameter ABITS    = 10; // internal address bits
    parameter DBITS    = 32;
    parameter SIZE     = 1 << ABITS;
    parameter OFFSET   = 0;

    input  wire            clka, ena, wea, clkb, enb, web;
    output reg             acka, ackb;
    input  wire[PBITS-1:0] addra, addrb;
    input  wire[DBITS-1:0] dina , dinb;
    output reg [DBITS-1:0] douta, doutb;

    reg[DBITS-1:0] store[0:SIZE - 1];

    integer i;
    initial begin
        if (INIT)
            for (i = 0; i < SIZE; i = i + 1)
                store[i] = ZERO;
        if (LOADH) $readmemh(LOADFILE, store);
        if (LOADB) $readmemb(LOADFILE, store);
    end

    always @(posedge clka) begin
        douta = store[addra - OFFSET];
        if (wea && ena)
            store[addra - OFFSET] = dina;
        acka = ena;
    end

    always @(posedge clkb) begin
        doutb = store[addrb - OFFSET];
        if (web && enb)
            store[addrb - OFFSET] = dinb;
        ackb = enb;
    end

endmodule

