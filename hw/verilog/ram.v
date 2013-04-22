`include "common.vh"
`timescale 1ns/10ps

module BlockRAM(
    input clka, ena, wea, clkb, enb, web,
    input[ABITS-1:0] addra, input[DBITS-1:0] dina, output reg[DBITS-1:0] douta,
    input[ABITS-1:0] addrb, input[DBITS-1:0] dinb, output reg[DBITS-1:0] doutb
);

    parameter INIT     = 0;
    parameter ZERO     = 0;
    parameter LOAD     = 0;
    parameter LOADFILE = "default.memh";
    parameter ABITS    = 32;
    parameter DBITS    = 32;
    parameter SIZE     = 1024;

    reg[DBITS-1:0] store[(SIZE - 1):0];

    integer i, j;
    initial begin
        if (INIT)
            for (i = 0; i < (SIZE + 7) / 8; i = i + 1)
                for (j = 0; j < 8 && i * 8 + j < SIZE; j = j + 1)
                    store[i * 8 + j] = ZERO;
        if (LOAD) $readmemh(LOADFILE, store);
    end

    always @(posedge clka) begin
        if (wea && ena)
            store[addra] <= dina;
        douta <= store[addra];
    end

    always @(posedge clkb) begin
        if (web && enb)
            store[addrb] <= dinb;
        doutb <= store[addrb];
    end

endmodule

