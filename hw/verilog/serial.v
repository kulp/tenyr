`include "common.vh"
`timescale 1ns/10ps

// Mostly-dummy serial device to give synthesis something to produce output
// from that doesn't need to be high-speed.
module Serial(input clk, input enable, input rw,
        input[31:0] addr, inout[31:0] data,
        input _reset, output txd, input rxd);
    parameter BASE = 1 << 5;
    parameter SIZE = 2;

    reg rtxd = 0;
    assign txd = rtxd;

    wire in_range = (addr >= BASE && addr < SIZE + BASE);

    reg[31:0] rdata = 0;
    //assign data = (enable && in_range && !rw) ? rdata : 32'bz;

    reg[15:0] sclk = 16'b1;
    always @(negedge clk) sclk = {sclk[0],sclk[15:1]};

    always @(negedge sclk[0]) begin
        if (enable && in_range) begin
            if (rw)
                rtxd <= &data;
            else
                rdata[0] <= rxd;
        end
    end
endmodule

