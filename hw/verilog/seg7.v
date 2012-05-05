`include "common.vh"
`timescale 1ns/10ps

// basic 7-segment driver
module Seg7(clk, enable, rw, addr, data, reset_n, seg, an); 

    parameter STATES = 4;
    parameter BASE = 1 << 4;
    parameter NDIGITS = 4;
    localparam SIZE = 2; // TODO compute based on NDIGITS ?
    localparam NI = NDIGITS - 1;
    localparam NI4 = NDIGITS * 4 - 1;
    localparam NIS = NDIGITS * STATES - 1;

    input clk;
    input enable;
    input rw;
    input[31:0] addr;
    inout[31:0] data;
    input reset_n;
    output[7:0] seg;
    output[NI:0] an;

    JCounter #(.STAGES(2), .SHIFTS(10))
        downclocker(.clk(clk), .ce(1'b1), .tick(downclk));

    reg[NIS:0] ena = 1'b1;
`ifndef SIM
    reg[NI4:0] mydata[SIZE - 1:0] = { 0, 0 };
`else
    reg[NI4:0] mydata[SIZE - 1:0];
	generate
		genvar q;
		for (q = 0; q < SIZE; q = q + 1)
			initial begin:setup mydata[q] = 0; end
	endgenerate
`endif

    wire in_range = (addr >= BASE && addr < SIZE + BASE);

    generate
        genvar i, j;
        wire[8 * NDIGITS - 1:0] bits;

        for (i = 0; i < NDIGITS; i = i + 1) begin:digit
            wire[6:0] char;
            wire[7:0] line;
            Hex2AsciiDigit digit(downclk, mydata[0][(4 * i) +: 4], char);
            lookup7 lookup(downclk, char, line);

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

    always @(negedge downclk)
        ena = {ena[NIS - 1:0],ena[NIS]};

    always @(negedge clk)
        if (in_range && enable && rw)
            mydata[addr - BASE] = data[NI4:0];

endmodule


