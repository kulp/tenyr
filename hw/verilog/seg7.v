`include "common.vh"
`timescale 1ns/10ps

// basic 7-segment driver
// TODO distinguish enable and strobe
module Seg7(input clk, enable, rw, reset_n, input[31:0] addr, data,
            output[7:0] seg, output[NI:0] an);

    parameter STATES = 4;
    parameter BASE = 1 << 4;
    parameter NDIGITS = 4;
    localparam SIZE = 2; // TODO compute based on NDIGITS ?
    localparam NI = NDIGITS - 1;
    localparam NI4 = NDIGITS * 4 - 1;
    localparam NIS = NDIGITS * STATES - 1;

    wire downclk;
    JCounter #(.STAGES(2), .SHIFTS(10))
        downclocker(.clk(clk), .ce(1'b1), .tick(downclk));

    reg[NIS:0] ena = 1'b1;
    reg[NI4:0] mydata[SIZE - 1:0]
`ifndef __ICARUS__
`ifndef ISIM
    = { 0 }
`endif
`endif
    ;

    wire in_range = (addr >= BASE && addr < SIZE + BASE);

    generate
        genvar i, j;
        // XXX reset state with reset_n
        /*
        for (i = 0; i < SIZE; i = i + 1) begin:reset
            always @(posedge clk)
                if (!reset_n)
                    mydata[i] = 0;
        end
        */

        wire[8 * NDIGITS - 1:0] bits;

        for (i = 0; i < NDIGITS; i = i + 1) begin:digit
            wire[6:0] char;
            wire[7:0] line;
            Hex2AsciiDigit digit(downclk, mydata[0][(4 * i) +: 4], char);
            lookup7 lookup(downclk, char, line);

            // digit segments
            for (j = 0; j < 7; j = j + 1) begin:bit
                assign bits[j * NDIGITS + i] = ena[i * STATES] ? line[j] : 1'b1;
            end
            // decimal points
            assign bits[7 * NDIGITS + i] = ena[i * STATES] ? ~mydata[1][i] : 1'b1;
        end

        for (j = 0; j < 8; j = j + 1) begin:bit
            assign seg[j] = &bits[j * NDIGITS +: NDIGITS];
        end

        for (j = 0; j < NDIGITS; j = j + 1) begin:en
            assign an[j] = ~ena[j * STATES];
        end
    endgenerate

    always @(posedge downclk)
        ena = {ena[NIS - 1:0],ena[NIS]};

    always @(posedge clk)
        if (in_range && enable && rw && reset_n)
            mydata[addr - BASE] = data[NI4:0];

endmodule


