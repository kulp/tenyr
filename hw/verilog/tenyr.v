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

            rZ  <= insn[24 +: 4];
            rX  <= insn[20 +: 4];
            rY  <= insn[16 +: 4];
            rop <= insn[12 +: 4];
            rI  <= insn[ 0 +:12];
        end
        4'b1111: rillegal <= &insn;
        default: $stop(1);
    endcase

endmodule

module Core(input clk);
    wire _operand_rw, reg_rw;
    wire[23:0] pc  , _operand_addr;
    wire[31:0] insn, _operand_data;
    reg[3:0] reg_indexZ,
             reg_indexX,
             reg_indexY;
    wire[3:0] _reg_indexZ,
              _reg_indexX,
              _reg_indexY;
    reg[31:0] reg_dataZ,
              reg_dataX,
              reg_dataY,
              reg_dataI;
    wire[31:0] _reg_dataZ = reg_rw ? reg_dataZ : 'bz;
    wire[31:0] _reg_dataX, _reg_dataY;
    wire[11:0] _reg_dataI;
    wire[3:0] op;
    wire flip, illegal, type;
    reg writing = 0;
    reg reading = 0;

    Reg regs(.clk(clk),
            .rwZ(reg_rw), .indexZ(reg_indexZ), .valueZ(_reg_dataZ),
                          .indexX(reg_indexX), .valueX(_reg_dataX),
                          .indexY(reg_indexY), .valueY(_reg_dataY),
            .pc(pc));

    Mem ram(.clk(clk), .enable('b1), .p0rw(_operand_rw),
            .p0_addr(_operand_addr), .p0_data(_operand_data),
            .p1_addr(pc)           , .p1_data(insn));
    Decode decode(.insn(insn), .Z(_reg_indexZ), .X(_reg_indexX), .Y(_reg_indexY),
                  .I(_reg_dataI), .op(op), .flip(flip));
endmodule

module Top();
    reg clk = 0;
    always #(`CLOCKPERIOD) clk = !clk;

    Core core(clk);
endmodule

