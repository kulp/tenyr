`include "common.vh"
`timescale 1ns/10ps

module mmr(input clk, strobe, rw, reset_n, we, re, output reg[PBITS-1:0] d_out,
           input[ABITS-1:0] addr, input[PBITS-1:0] d_in, inout[DBITS-1:0] val);

    parameter ADDR    = 0;
    parameter RE      = 1; // reads possible ?
    parameter WE      = 1; // writes possible ?
    parameter DEFAULT = 0;
    parameter ABITS   = 32;
    parameter PBITS   = 32;
    parameter DBITS   = PBITS;

    reg[DBITS - 1:0] store = DEFAULT;

    assign val = re ? store : 32'bz;

    always @(posedge clk)
        if (strobe && addr == ADDR) begin
            if (rw && WE)
                store <= d_in[DBITS-1:0];
            else if (!rw && RE)
                d_out <= store;
        end else begin
            if (we)
                store <= val;
        end

endmodule

