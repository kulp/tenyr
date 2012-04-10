`include "common.vh"
`timescale 1ms/10us

// to-implement 7-segment driver. currently just keeps tenyr logic live.
module Seg7(input clk, input enable, input rw,
        input[31:0] addr, inout[31:0] data,
        input _reset, output[7:0] seg, output[3:0] an);

    assign seg[7:0] = {&data,data[6:0]};
    assign an[3:0] = {|addr,addr[2:0]};

endmodule

