`include "common.vh"
`timescale 1ns/10ps

module mmr(input clk, strobe, rw, re, we, output[PBITS-1:0] d_out,
           input[ABITS-1:0] addr, input[PBITS-1:0] d_in, inout[DBITS-1:0] val);

    parameter ADDR    = 0;
    parameter RE      = 1; // reads possible ?
    parameter WE      = 1; // writes possible ?
    parameter DEFAULT = 0;
    parameter ABITS   = 32;
    parameter PBITS   = 32;
    parameter DBITS   = PBITS;

    reg[DBITS - 1:0] store = DEFAULT;

    wire active  = strobe && addr == ADDR;
    assign val   = we              ? store : 32'bz; // high-Z for driving side
    assign d_out = (active && !rw) ? store : 32'b0; // zero for addressed side

    always @(posedge clk)
        if (active && rw && WE)
            store <= d_in[DBITS-1:0];
        else if (re && RE)
            store <= val;

endmodule

