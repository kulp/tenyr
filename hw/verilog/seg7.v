`include "common.vh"
`timescale 1ns/10ps

// basic 7-segment driver
module Seg7(clk, enable, rw, addr, data, _reset, seg, an); 

    parameter STATES = 4;
    parameter BASE = 1 << 4;
    parameter SIZE = 1;
    parameter NDIGITS = 4;
    localparam NI = NDIGITS - 1;
    localparam NI4 = NDIGITS * 4 - 1;
    localparam NIS = NDIGITS * STATES - 1;

    input clk;
    input enable;
    input rw;
    input[31:0] addr;
    inout[31:0] data;
    input _reset;
    output[7:0] seg;
    output[NI:0] an;

    reg[NIS:0] ena = 1'b1;
    reg[NI4:0] mydata = 0;

    wire in_range = (addr >= BASE && addr < SIZE + BASE);

    generate
        genvar i, j;
        wire[8 * NDIGITS - 1:0] bits;

        for (i = 0; i < NDIGITS; i = i + 1) begin:digit
            wire[7:0] char;
            wire[7:0] line;
            Hex2AsciiDigit digit(mydata[(4 * i) +: 4], char);
            lookup7 lookup(char, line);

            for (j = 0; j < 8; j = j + 1) begin:bit
                assign bits[j * NDIGITS + i] = ena[i * STATES] ? line[j] : 1'b1;
            end
        end

        for (j = 0; j < 8; j = j + 1) begin:bit
            assign seg[j] = &bits[j * NDIGITS +: NDIGITS];
        end

        for (j = 0; j < NDIGITS; j = j + 1) begin:en
            assign an[j] = ~ena[j * STATES];
        end
    endgenerate

    always @(negedge clk) begin
        ena = {ena[NIS - 1:0],ena[NIS]};
        if (in_range && enable && rw)
            mydata = data[NI4:0];
    end

endmodule


