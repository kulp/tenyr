`timescale 1ms/10us

`define CLOCKPERIOD 5
`define RAMDELAY (1 * `CLOCKPERIOD)

// Two-port memory required if we don't have wait states ; one instruction
// fetch per cycle, and up to one read or write. Port 0 is R/W ; port 1 is R/O
module Mem(input clk, input enable, input p0rw,
        input[23:0] p0_addr, inout[31:0] p0_data,
        input[23:0] p1_addr, inout[31:0] p1_data
        );
    parameter BASE = 1 << 12;
    parameter SIZE = (1 << 24) - (1 << 12);

    reg[23:0] ram[(SIZE + BASE - 1):BASE];
    reg[31:0] p0data = 0;
    reg[31:0] p1data = 0;

    wire p0_inrange, p1_inrange;

    assign p0_data = (enable && !p0rw && p0_inrange) ? p0data : 'bz;
    assign p1_data = enable ? p1data : 'bz;

    assign p0_inrange = (p0_addr >= BASE && p0_addr < SIZE + BASE);
    assign p1_inrange = (p1_addr >= BASE && p1_addr < SIZE + BASE);

    always @(negedge clk) begin
        if (enable) begin
            if (p0_inrange) begin
                // rw = 1 is writing
                if (p0rw) begin
                    p0data = p0_data;
                    ram[p0_addr] = p0data;
                end else begin
                    p0data = ram[p0_addr];
                end
            end

            if (p1_inrange)
                p1data = ram[p1_addr];
        end
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
        r_rwZ <= rwZ;
        if (rwZ) begin
            if (indexZ == 0)
                $display("wrote to zero register");
            else begin
                store[indexZ] <= valueZ;
            end
        end
    end

    always @(indexZ) begin
        r_valueZ <= store[indexZ];
    end

    always @(negedge clk or indexX or indexY) begin
        r_valueX <= store[indexX];
        r_valueY <= store[indexY];
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

module Decode(input[31:0] insn, output[3:0] Z, X, Y, output[11:0] I,
              output[3:0] op, output[1:0] deref, output flip, type, illegal);

    wire[31:0] insn;
    reg[3:0] rZ, rX, rY, rop;
    reg[11:0] rI;
    reg[1:0] rderef;
    reg rflip, rtype, rillegal;

    assign Z = rZ, X = rX, Y = rY, op = rop, deref = rderef, flip = rflip,
           type = rtype, illegal = rillegal;

    always @(insn) casex (insn[31:28])
        4'b0???: begin
            rderef <= { insn[29] & ~insn[28], insn[28] };
            rflip  <= insn[29] & insn[28];
            rtype  <= insn[30];

            rZ  <= insn[27:24];
            rX  <= insn[23:20];
            rY  <= insn[19:16];
            rop <= insn[15:12];
            rI  <= insn[11: 0];
        end
        4'b1111: rillegal <= &insn;
        default: $stop(1);
    endcase

endmodule

module Top();
    reg clk = 0;

    always #(`CLOCKPERIOD) clk = !clk;

    wire _operand_rw;
    wire[23:0] _insn_addr, _operand_addr;
    wire[31:0] _insn_data, _operand_data;

    // TODO make BASE (1 << 12)
    Mem #(.BASE(0), .SIZE(1 << 12))
        ram(.clk(clk), .enable('b1), .p0rw(_operand_rw),
            .p0_addr(_operand_addr), .p0_data(_operand_data),
            .p1_addr(_insn_addr)   , .p1_data(_insn_data));
    Core core(clk, _operand_rw, _operand_addr, _operand_data);
endmodule

