`include "common.vh"
`timescale 1ns/10ps

module mmr(clk, enable, rw, addr, d_in, d_out, reset_n, we, re, val);

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
    input[PBITS - 1:0] d_in;
    output[PBITS - 1:0] d_out;
    input reset_n;
    input we, re; // write-enable, read-enable through val port
    inout[DBITS - 1:0] val;

    reg[DBITS - 1:0] store = DEFAULT;

    wire active = enable && addr == ADDR;

    assign d_out = (active && !rw && RE) ? store : 32'b0;
    assign val   = re ? store : {DBITS{1'bz}};

    always @(posedge clk)
        if (active) begin
            if (rw && WE) begin
                store <= d_in[DBITS-1:0];
            end
        end else begin
            if (we) begin
                store <= val;
            end
        end

endmodule

