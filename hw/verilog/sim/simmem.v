`include "common.vh"
`timescale 1ms/10us

// Two-port memory required if we don't have wait states ; one instruction
// fetch per cycle, and up to one read or write. Port 0 is R/W ; port 1 is R/O
module SimMem(input clk, input enable, input p0rw,
        input[31:0] p0_addr, inout[31:0] p0_data,
        input[31:0] p1_addr, inout[31:0] p1_data
        );
    parameter BASE = 1 << 12; // TODO pull from environmental define
    parameter SIZE = (1 << 24) - BASE;

    reg[31:0] store[(SIZE + BASE - 1):BASE];

    wire p0_inrange = (p0_addr >= BASE && p0_addr < SIZE + BASE);
    wire p1_inrange = (p1_addr >= BASE && p1_addr < SIZE + BASE);

    assign p0_data = (enable && p0_inrange && !p0rw) ? store[p0_addr] : 32'bz;
    assign p1_data = (enable && p1_inrange         ) ? store[p1_addr] : 32'bz;

    always @(negedge clk)
        if (enable && p0_inrange && p0rw)
            store[p0_addr] <= `SETUP p0_data;

endmodule

