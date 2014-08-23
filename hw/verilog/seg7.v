`include "common.vh"
`timescale 1ns/10ps

// basic 7-segment driver
// TODO permit reads
module Seg7(input clk, strobe, rw, reset_n, input[31:0] addr, data,
            output[7:0] seg, output reg[NUM_DIGITS-1:0] an);

    parameter  ADDR_BASE    = 1 << 4;
    parameter  COUNTER_BITS = 16; // 80MHz input => full screen refresh in 2ms
    localparam NUM_DIGITS   = 4;

    reg[COUNTER_BITS-1:0] counter = 0;
    reg[NUM_DIGITS-1:0][3:0] store[1:0];
    reg[1:0] dig = 0;

    wire in_range = (addr & ~1) == ADDR_BASE;
    assign seg[7] = (store[1][0] & ~an) == 0;
    Hex2Segments lookup(clk, store[0][dig], seg[6:0]);

    initial an <= ~1;

    always @(posedge clk) begin
        if (!reset_n) begin
            an       <= ~1;
            dig      <= 0;
            counter  <= 0;
            store[0] <= 0;
            store[1] <= 0;
        end else begin
            counter <= counter + 1;
            if (&counter) begin
                an  <= {an[NUM_DIGITS-2:0],an[NUM_DIGITS-1]};
                dig <= dig + 1;
            end else if (in_range && strobe && rw)
                store[addr & 1] <= data;
        end
    end

endmodule

