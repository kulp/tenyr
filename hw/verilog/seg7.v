`include "common.vh"
`timescale 1ns/10ps

// basic 7-segment driver
// TODO distinguish enable and strobe
module Seg7(input clk, enable, rw, reset_n, input[31:0] addr, data,
            output[7:0] seg, output[NI:0] an);

    parameter STATES = 4;
    parameter BASE = 1 << 4;
    parameter NDIGITS = 4;
    parameter DOWNCTRBITS = 6;
    localparam SIZE = 2; // TODO compute based on NDIGITS ?
    localparam NI = NDIGITS - 1;
    localparam NI4 = NDIGITS * 4 - 1;
    localparam NIS = NDIGITS * STATES - 1;

    reg downclk = 0;
    reg[DOWNCTRBITS-1:0] downctr = 0;
    always @(posedge clk) begin
        downctr <= downctr + 1;
        if (downctr == 0)
            downclk <= ~downclk;
    end

    reg[NIS:0] ena = 1'b1;
    reg[NI4:0] mydata[SIZE - 1:0];

    wire in_range = (addr >= BASE && addr < SIZE + BASE);

    integer k;
    generate
        genvar i, j;
        always @(posedge clk)
            if (!reset_n)
                for (k = 0; k < SIZE; k = k + 1)
                    mydata[k] = 0;
            else if (in_range && enable && rw)
                mydata[addr - BASE] = data[NI4:0];

        wire[8 * NDIGITS - 1:0] bits;

        for (i = 0; i < NDIGITS; i = i + 1) begin:digit
            wire[6:0] char;
            wire[7:0] line;
            Hex2AsciiDigit digit(downclk, mydata[0][(4 * i) +: 4], char);
            lookup7 lookup(downclk, char, line);

            // digit segments
            for (j = 0; j < 7; j = j + 1) begin:abit
                assign bits[j * NDIGITS + i] = ena[i * STATES] ? line[j] : 1'b1;
            end
            // decimal points
            assign bits[7 * NDIGITS + i] = ena[i * STATES] ? ~mydata[1][i] : 1'b1;
        end

        for (j = 0; j < 8; j = j + 1) begin:abit
            assign seg[j] = &bits[j * NDIGITS +: NDIGITS];
        end

        for (j = 0; j < NDIGITS; j = j + 1) begin:en
            assign an[j] = ~ena[j * STATES];
        end
    endgenerate

    always @(posedge downclk)
        ena = {ena[NIS - 1:0],ena[NIS]};

endmodule


