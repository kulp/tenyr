`include "common.vh"
`timescale 1ns/10ps

module Reg(input clk, en, rwZ, input[3:0] indexZ, indexX, indexY,
           inout[31:0] valueZ, output[31:0] valueX, valueY, input[31:0] pc);

    reg[31:0] store[1:14];

    wire ZisP =  &indexZ, XisP =  &indexX, YisP =  &indexY;
    wire Zis0 = ~|indexZ, Xis0 = ~|indexX, Yis0 = ~|indexY;

    assign valueZ = ~en ? 'bz : rwZ ? 32'bz : Zis0 ? 0 : ZisP ? pc + 1 : store[indexZ];
    assign valueX = ~en ? 'bz :               Xis0 ? 0 : XisP ? pc + 1 : store[indexX];
    assign valueY = ~en ? 'bz :               Yis0 ? 0 : YisP ? pc + 1 : store[indexY];

    always @(negedge clk)
        if (en && rwZ && !Zis0 && !ZisP)
            store[indexZ] = valueZ;

endmodule

module Decode(input[31:0] insn, output[3:0] Z, X, Y, output[11:0] I,
              output[3:0] op, output[1:0] deref, output type, illegal);

    assign type     = insn[30];
    assign deref    = insn[28 +: 2];
    assign Z        = insn[24 +: 4];
    assign X        = insn[20 +: 4];
    assign Y        = insn[16 +: 4];
    assign op       = insn[12 +: 4];
    assign I        = insn[ 0 +:12];
    assign illegal  = &insn;

endmodule

module Exec(input clk, en, type, output reg[31:0] rhs,
            input signed[31:0] X, Y, input signed[11:0] I, input[3:0] op);

    wire signed[31:0] J = { {20{I[11]}}, I };
    wire signed[31:0] O = type ? J : Y;
    wire signed[31:0] A = type ? Y : J;

    always @(negedge clk) if (en) begin
        case (op)
            4'b0000: rhs =  (X  |  O) + A;  // X bitwise or Y
            4'b0001: rhs =  (X  &  O) + A;  // X bitwise and Y
            4'b0010: rhs =  (X  +  O) + A;  // X add Y
            4'b0011: rhs =  (X  *  O) + A;  // X multiply Y
            4'b0100: rhs = 32'bx;           // reserved
            4'b0101: rhs =  (X  << O) + A;  // X shift left Y
            4'b0110: rhs = -(X  <  O) + A;  // X compare < Y
            4'b0111: rhs = -(X  == O) + A;  // X compare == Y
            4'b1000: rhs = -(X  >  O) + A;  // X compare > Y
            4'b1001: rhs =  (X  &~ O) + A;  // X bitwise and complement Y
            4'b1010: rhs =  (X  ^  O) + A;  // X bitwise xor Y
            4'b1011: rhs =  (X  -  O) + A;  // X subtract Y
            4'b1100: rhs =  (X  ^~ O) + A;  // X xor ones' complement Y
            4'b1101: rhs =  (X  >> O) + A;  // X shift right logical Y
            4'b1110: rhs = -(X  != O) + A;  // X compare <> Y
            4'b1111: rhs = 32'bx;           // reserved
        endcase
    end

endmodule

module Core(input clk, input en, input reset_n, `HALTTYPE halt,
            output reg[31:0] i_addr, input[31:0] i_data,
            output mem_rw, output[31:0] d_addr, inout[31:0] d_data);

    wire _en = en && reset_n;

    wire illegal, type;
    wire[ 3:0] indexX, indexY, indexZ;
    wire[31:0] valueX, valueY, rhs;
    wire[11:0] valueI;
    wire[ 3:0] op;
    wire[ 1:0] deref;

    wire reg_rw     = ~deref[1];
    wire jumping    = &indexZ  && reg_rw;
    wire mem_active = !illegal && |deref;

    wire[31:0] deref_rhs = (deref[0] && !mem_rw) ? d_data : rhs;
    wire[31:0] mem_addr  = mem_active ? (deref[0] ? rhs : interZ) : 32'bz;
    wire[31:0] mem_data  = mem_rw     ? (deref[0] ? interZ : rhs) : 32'b0;
    wire[31:0] interZ    = reg_rw     ? deref_rhs : valueZ;
    wire[31:0] valueZ    = reg_rw     ? interZ : 32'bz;

    assign mem_rw = mem_active && deref[1];
    assign d_addr = mem_active ? mem_addr  : 32'bz;
    assign d_data = mem_rw     ? mem_data  : 32'bz;

    reg[ 2:0] cyc;
    reg rhalt;
    assign halt[`HALT_EXEC] = rhalt;

    always @(negedge clk) begin
        if (!reset_n) begin
            i_addr <= `RESETVECTOR;
            rhalt <= 1'b0;
            cyc <= 3'b1;
        end

        if (_en) begin
            cyc <= {cyc[1:0],cyc[2] & ~|halt};

            case (cyc)
                3'b010: if (reset_n) rhalt <= rhalt | illegal;
                3'b100: if (~|halt) i_addr <= jumping ? valueZ : i_addr + 1;
            endcase
        end
    end

    // Decode and register reads happen as soon as instruction is ready.
    Decode decode(.insn(i_data), .op(op), .deref(deref), .illegal(illegal),
                  .type(type), .Z(indexZ), .X(indexX), .Y(indexY), .I(valueI));

    // Execution (arithmetic operation) happen the cyc after decode
    Exec exec(.clk(clk), .en(_en & cyc[1]), .op(op), .type(type),
              .rhs(rhs), .X(valueX), .Y(valueY), .I(valueI));

    // Registers and memory get written last, the cyc after execution
    Reg regs(.clk(clk), .pc(i_addr), .indexX(indexX), .valueX(valueX),
             .en(_en & |cyc[2:1]),   .indexY(indexY), .valueY(valueY),
             .rwZ(reg_rw & cyc[2]),  .indexZ(indexZ), .valueZ(valueZ));

endmodule

