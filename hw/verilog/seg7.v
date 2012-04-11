`include "common.vh"
`timescale 1ms/10us

// basic 7-segment driver
module Seg7(clk, enable, rw, addr, data, _reset, seg, an); 

    parameter BASE = 1 << 4;
    parameter SIZE = 1;
    parameter N = 4;
    localparam NI = N - 1;
    localparam NI4 = N * 4 - 1;

    input clk;
    input enable;
    input rw;
    input[31:0] addr;
    inout[31:0] data;
    input _reset;
    output[7:0] seg;
    output[NI:0] an;

    integer idx = 0;
    reg[NI4:0] mydata = 0;
    wire[NI:0][7:0] ascii;
    wire[NI:0][6:0] lines;

    assign seg = lines[idx];
    assign an = 1'b1 << idx;

    wire in_range = (addr >= BASE && addr < SIZE + BASE);

    generate
        genvar i;
        for (i = 0; i < N; i = i + 1) begin:digit
            Hex2AsciiDigit digit(mydata[(4 * i) +: 4], ascii[i]);
            lookup7 lookup(ascii[i], lines[i]);
        end
    endgenerate

    // TODO cycle more slowly than system clock
    always @(negedge clk) begin
        //an = {an[2:0],an[3]};
        idx = (idx + 1) % N;
        if (in_range && enable && rw)
            mydata = data[NI4:0];
    end

endmodule


