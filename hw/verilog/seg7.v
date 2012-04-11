`include "common.vh"
`timescale 1ms/10us

// basic 7-segment driver
module Seg7(clk, enable, rw, addr, data, _reset, seg, an); 

    parameter BASE = 1 << 4;
    parameter SIZE = 2;
    parameter N = 4;

    input clk;
    input enable;
    input rw;
    input[31:0] addr;
    inout[31:0] data;
    input _reset;
    output[7:0] seg;
    output reg[(N - 1):0] an;

    wire in_range = (addr >= BASE && addr < SIZE + BASE);
    wire en = in_range && enable && rw;

    reg[(N * 4 - 1):0] mydata = 0;
    wire[(N - 1):0][7:0] ascii;
    wire[(N - 1):0][6:0] lines;
    integer idx = 0;

    assign seg = lines[idx];

    generate
        genvar i;
        for (i = 0; i < N; i = i + 1) begin
            Hex2AsciiDigit digit(mydata[(4 * i) +: 4], ascii[i]);
            lookup7 lookup(ascii[i], lines[i]);
        end
    endgenerate

    always @(negedge clk) begin
        // TODO cycle more slowly than system clock
        an = {an[0],an[3:1]};
        idx = (idx + 1) % N;
        if (en)
            mydata = data[(N * 4 - 1):0];
    end

endmodule


