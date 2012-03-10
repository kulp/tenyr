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

module Reg(input clk,
        input rwZ, input[3:0] indexZ, inout [31:0] valueZ, // Z is RW
                   input[3:0] indexX, output[31:0] valueX, // X is RO
                   input[3:0] indexY, output[31:0] valueY, // Y is RO
        output[23:0] pc);

    reg[31:0] store[0:15];
    reg[31:0] r_valueZ = 0,
              r_valueX = 0,
              r_valueY = 0;

    reg r_rwZ = 0;

    generate
        genvar i;
        for (i = 0; i < 16; i = i + 1)
            initial #0 store[i] = 'b0;
    endgenerate

    assign pc = store[15];
    assign valueZ = rwZ ? 'bz : r_valueZ;
    assign valueX = r_valueX;
    assign valueY = r_valueY;

    always @(negedge clk) begin
        r_rwZ <= #3 rwZ;
        if (rwZ)
            if (indexZ == 0)
                $display("wrote to zero register");
            else begin
                store[indexZ] <= #3 valueZ;
            end
        else begin
            r_valueZ <= #3 store[indexZ];
        end

        r_valueX <= #3 store[indexX];
        r_valueY <= #3 store[indexY];
    end

endmodule

module Core(input clk, output mem_rw, output[23:0] _addr, inout[31:0] _data);

    reg[31:0] insn = 0;
    wire[3:0] _type = insn[31:28];
    wire[31:0] _reg_valueZ,
               _reg_valueX,
               _reg_valueY;
    reg[31:0] reg_valueX,
              reg_valueY;
    reg[3:0] reg_indexZ = 0,
             reg_indexX = 0,
             reg_indexY = 0;
    wire[23:0] pc;
    reg reg_rw = 0;
    reg[23:0] addr = 0;

    reg[31:0] mem_data = 0;
    wire[23:0] _mem_addr;
    wire[31:0] _mem_data = writing ? mem_data : 'bz;
    reg writing = 0;
    reg reading = 0;

    wire mem_rw = writing;

    Reg regs(.clk(clk),
            .rwZ(reg_rw), .indexZ(reg_indexZ), .valueZ(_reg_valueZ),
                          .indexX(reg_indexX), .valueX(_reg_valueX),
                          .indexY(reg_indexY), .valueY(_reg_valueY),
            .pc(pc));

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

