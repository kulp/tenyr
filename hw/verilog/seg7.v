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
    //output reg[NI:0] an = (1'b1 << (N - 1));
    output reg[NI:0] an = ~4'b0001;

    reg[NI4:0] mydata = 0;

    wire in_range = (addr >= BASE && addr < SIZE + BASE);

    generate
        genvar i, j;
        wire[8 * N - 1:0] bits;

        for (i = 0; i < N; i = i + 1) begin:digit
            wire[7:0] char;
            wire[7:0] line;
            Hex2AsciiDigit digit(mydata[(4 * i) +: 4], char);
            lookup7 lookup(char, line);

            for (j = 0; j < 8; j = j + 1) begin:bit
                assign bits[j * N + i] = ~an[i] & line[j];
            end
        end

        for (j = 0; j < 8; j = j + 1) begin:bit
            assign seg[j] = |bits[j * N +: N];
        end
    endgenerate

    always @(negedge clk) begin
        an = {an[NI - 1:0],an[NI]};
        if (in_range && enable && rw)
            mydata = data[NI4:0];
    end

endmodule


