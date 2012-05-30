`include "common.vh"
`timescale 1ns/10ps

module SimSerial(input clk, input enable, input rw,
        input[31:0] addr, inout[31:0] data,
        input reset_n);
    parameter BASE = 1 << 5;
    parameter SIZE = 2;

    wire in_range = (addr >= BASE && addr < SIZE + BASE);

    always @(negedge clk) begin
        if (enable && in_range) begin
            if (rw)
                $tenyr_putchar(data);
            else
                $tenyr_getchar(data);
        end
    end
endmodule

