`include "common.vh"
`timescale 1ns/10ps

module Gpio(input clk, strobe, rw, reset,
            input[31:0] addr, input[31:0] data_i, output reg[31:0] data_o,
            inout[COUNT-1:0] gpio);

    // four registers
    // 0 : enable (1 for enable)
    // 1 : read-write (1 for write)
    // 2 : read state (1 for high)
    // 3 : write state (1 for high)
    parameter COUNT = 32;
    localparam SIZE = 4;

    localparam EN = 0, RW = 1, RS = 2, WS = 3;

    reg [COUNT-1:0] state[SIZE-1:0];

    integer k;
    initial for (k = 0; k < SIZE; k = k + 1) state[k] = 0;

    genvar i;
    generate for (i = 0; i < COUNT; i = i + 1)
        assign gpio[i] = (state[EN][i] & state[RW][i]) ? state[WS][i] : 1'bz;
    endgenerate

    always @(posedge clk) begin
        state[RS] <= state[EN] & ~state[RW] & gpio;
        if (reset) begin
            for (k = 0; k < SIZE; k = k + 1)
                state[k] <= 0;
        end else if (strobe) begin
            if (rw)
                state[addr] <= data_i;
            else
                data_o <= state[addr];
        end
    end

endmodule

