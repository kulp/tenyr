`include "common.vh"
`timescale 1ns/10ps

// TODO use reset_n
module mmr(clk, enable, rw, addr, data, reset_n, val);

    parameter ADDR = `VIDEO_ADDR;

    input clk;
    input enable;
    input rw;
    input[31:0] addr;
    inout[31:0] data;
    input reset_n;

    inout val;

    wire in_range = addr == ADDR;
    wire active = enable && in_range;

    assign data = (active && !rw) ? val  : 32'bz;
    assign val  = (active &&  rw) ? data : 32'bz;

endmodule

