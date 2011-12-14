`timescale 1ms/10us

module mem(input clk, input enable, input rw, input[23:0] _addr, inout[31:0] _data);
    parameter BASE = 0;
    parameter SIZE = 1 << 24;

    reg[23:0] ram[(SIZE + BASE - 1):BASE];
    // it's not strictly necessary to register the address, but why not
    reg[23:0] addr;
    reg[31:0] data;

    reg sticky = 0;

    assign _data = (enable && !rw && sticky) ? data : 'bz;

    always @(negedge clk) begin
        if (enable && _addr >= BASE && _addr < SIZE + BASE) begin
            addr = _addr;
            sticky = 1;

            // rw = 1 is writing
            if (rw) begin
                data = _data;
                ram[addr] = data;
            end else begin
                data = ram[addr];
            end
        end else
            sticky = 0;
    end

endmodule

module top(clk);
    input wire clk;

    reg[23:0] mem_addr;
    reg[31:0] mem_data;
    wire[31:0] _mem_data = (mem_enable && writing) ? mem_data : 'bz;
    reg writing = 0;
    reg reading = 0;

    reg[31:0] val;

    wire mem_enable = reading ^ writing;
    mem #(.BASE(0), .SIZE(1 << 12)) ram(clk, mem_enable, writing, mem_addr, _mem_data);

    always @(negedge clk) begin
        if (reading)
            mem_data <= _mem_data;
    end
endmodule

