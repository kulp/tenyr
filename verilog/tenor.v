`timescale 1ms/10us

module mem(input clk, input enable, input rw, input[23:0] _addr, inout[31:0] _data);
    reg[23:0] ram[23:0];
    reg[23:0] addr;
    reg[31:0] data;

    assign _data = (enable && !rw) ? data : 'bz;

    always @(negedge clk) begin
        if (enable) begin
            addr = _addr;

            // rw = 1 is writing
            if (rw) begin
                data = _data;
                ram[addr] = data;
            end else begin
                data = ram[addr];
            end
        end
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
    mem mem(clk, mem_enable, writing, mem_addr, _mem_data);

    always @(negedge clk) begin
        if (reading)
            mem_data <= _mem_data;
    end
endmodule

