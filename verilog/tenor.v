`timescale 1ms/10us

`define CLOCKPERIOD 5
`define RAMDELAY (1 * `CLOCKPERIOD)

module Mem(input clk, input enable, input rw, input[23:0] _addr, inout[31:0] _data);
    parameter BASE = 0;
    parameter SIZE = 1 << 24;

    reg[23:0] ram[(SIZE + BASE - 1):BASE];
    // it's not strictly necessary to register the address, so perhaps elide
    reg[23:0] addr = 0;
    reg[31:0] data = 0;

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

module Reg(input clk, input rw, input[3:0] _index, inout[31:0] _value, output[23:0] pc);

    reg[31:0] store[15:0];
    reg[31:0] value = 0;

    initial store[15] = 0; // PC inits to zero

    assign pc = store[15];
    assign _value = rw ? 'bz : value;

    always @(negedge clk)
        if (rw)
            if (_index == 0)
                $display("wrote to zero register");
            else
                store[_index] = _value;
        else
            value = store[_index];

endmodule

module Core(input clk, output mem_rw, output[23:0] _addr, inout[31:0] _data);

    reg[31:0] insn = 0;
    wire[3:0] _type = insn[31:28];
    wire[31:0] _reg_value;
    reg[3:0] reg_index = 0;
    wire[23:0] pc;
    reg reg_rw = 0;
    reg[23:0] addr = 0;

    reg[31:0] mem_data = 0;
    wire[23:0] _mem_addr;
    wire[31:0] _mem_data = writing ? mem_data : 'bz;
    reg writing = 0;
    reg reading = 0;

    wire mem_rw = writing;

    Reg regs(clk, reg_rw, reg_index, _reg_value, pc);

    always @(negedge clk) begin
        if (reading)
            mem_data <= _mem_data;
    end

    assign _addr = addr;

    always @(negedge clk) begin
        // instruction fetch
        addr <= pc;
        insn <= #(`RAMDELAY) _data;
    end

endmodule

module Top();
    reg clk = 0;

    always #(`CLOCKPERIOD) clk = !clk;

    wire _mem_rw;
    wire[23:0] _mem_addr;
    wire[31:0] _mem_data;

    Mem #(.BASE(0), .SIZE(1 << 12)) ram(clk, 'b1, _mem_rw, _mem_addr, _mem_data);
    Core core(clk, _mem_rw, _mem_addr, _mem_data);
endmodule

