`include "common.vh"
`timescale 1ns/10ps

module mmr(clk, enable, rw, addr, data, reset_n, we, re, val);

    parameter ADDR    = 0;
    parameter RE      = 1; // reads possible ?
    parameter WE      = 1; // writes possible ?
    parameter DEFAULT = 0;
    parameter ABITS   = 32;
    parameter PBITS   = 32;
    parameter DBITS   = PBITS;

    input clk;
    input enable; // TODO convert to / add a proper strobe
    input rw;
    input[ABITS - 1:0] addr;
    inout[PBITS - 1:0] data;
    input reset_n;
    input we, re; // write-enable, read-enable through val port
    inout[DBITS - 1:0] val;

    reg[DBITS - 1:0] store = DEFAULT;

    wire active = enable && addr == ADDR;

    assign data = (active && !rw && RE) ? store : 'bz;
    assign val  = re ? store : {DBITS{1'bz}};

    always @(posedge clk)
        if (active) begin
            if (rw && WE) begin
                store <= data[DBITS-1:0];
            end
        end else begin
            if (we) begin
                store <= val;
            end
        end

endmodule

