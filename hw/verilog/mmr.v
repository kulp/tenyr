`include "common.vh"
`timescale 1ns/10ps

module mmr(clk, enable, rw, addr, data, reset_n, we, re, val);

    parameter ADDR = 0;
    parameter RE = 1; // reads possible ?
    parameter WE = 1; // writes possible ?
    parameter DEFAULT = 0;
    parameter BUS_ADDR_WIDTH = 32;
    parameter BUS_DATA_WIDTH = 32;
    parameter MMR_WIDTH = BUS_DATA_WIDTH;

    input clk;
    input enable;
    input rw;
    input[BUS_ADDR_WIDTH - 1:0] addr;
    inout[BUS_DATA_WIDTH - 1:0] data;
    input reset_n;
    input we, re; // write-enable, read-enable through val port
    inout[MMR_WIDTH - 1:0] val;

    reg[MMR_WIDTH - 1:0] store = DEFAULT;

    wire active = enable && addr == ADDR;

    assign data = (active && !rw && RE) ? store : 'bz;
    assign val  = re ? store : 'bz;

    always @(`EDGE clk)
        if (active) begin
            if (rw && WE) begin
                store <= data;
            end
        end else begin
            if (we) begin
                store <= val;
            end
        end

endmodule

