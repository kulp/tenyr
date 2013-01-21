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

module Exec(input clk, en, type, valid, output signed[31:0] rhs,
            input signed[31:0] X, Y, input signed[11:0] I, input[3:0] op);

    assign rhs = valid ? i_rhs : 32'b0;
    reg signed[31:0] i_rhs;

    wire signed[31:0] J = { {20{I[11]}}, I };
    wire signed[31:0] O = (type == 0) ? Y : J;
    wire signed[31:0] A = (type == 0) ? J : Y;

    always @(negedge clk) if (en) begin
        if (valid) begin
            case (op)
                4'b0000: i_rhs =  (X  |  O) + A; // X bitwise or Y
                4'b0001: i_rhs =  (X  &  O) + A; // X bitwise and Y
                4'b0010: i_rhs =  (X  +  O) + A; // X add Y
                4'b0011: i_rhs =  (X  *  O) + A; // X multiply Y
              //4'b0100:                         // reserved
                4'b0101: i_rhs =  (X  << O) + A; // X shift left Y
                4'b0110: i_rhs = -(X  <  O) + A; // X compare < Y
                4'b0111: i_rhs = -(X  == O) + A; // X compare == Y
                4'b1000: i_rhs = -(X  >  O) + A; // X compare > Y
                4'b1001: i_rhs =  (X  &~ O) + A; // X bitwise and complement Y
                4'b1010: i_rhs =  (X  ^  O) + A; // X bitwise xor Y
                4'b1011: i_rhs =  (X  -  O) + A; // X subtract Y
                4'b1100: i_rhs =  (X  ^~ O) + A; // X xor ones' complement Y
                4'b1101: i_rhs =  (X  >> O) + A; // X shift right logical Y
                4'b1110: i_rhs = -(X  != O) + A; // X compare <> Y
              //4'b1111:                         // reserved

                default: i_rhs = 32'bx;
            endcase
        end else begin
            i_rhs = 32'bx;
        end
    end

endmodule

module Core(input clk, input en, input reset_n, `HALTTYPE halt,
            output[31:0] insn_addr, input[31:0] insn_data,
            output mem_rw, output[31:0] norm_addr, inout[31:0] norm_data);

    wire _en = en && reset_n;

    wire illegal, type;
    wire[ 3:0] indexX, indexY, indexZ;
    wire[31:0] valueX, valueY, valueZ, rhs;
    wire[11:0] valueI;
    wire[ 3:0] op;
    wire[ 1:0] deref;

    wire reg_rw     = ~deref[1];
    wire mem_active = !illegal   && |deref;
    wire mem_rw     = mem_active && deref[1];
    wire jumping    = &indexZ    && reg_rw;

    wire[31:0] deref_rhs  = (deref[0] && !mem_rw) ? norm_data : rhs;
    wire[31:0] mem_addr   = mem_active ? (deref[0] ? rhs : valueZ) : 32'b0;
    wire[31:0] mem_data   = mem_rw     ? (deref[0] ? valueZ : rhs) : 32'b0;
    wire[31:0] reg_valueZ = reg_rw     ? valueZ : 32'bz;

    assign valueZ    = reg_rw     ? deref_rhs : reg_valueZ;
    assign norm_addr = mem_active ? mem_addr  : 32'b0; // TODO default addr ?
    assign norm_data = mem_rw     ? mem_data  : 32'bz;

    reg[31:0] insn_addr;
    reg[ 2:0] cycle_state;
    reg rhalt;
    assign halt[`HALT_EXEC] = rhalt;

    always @(negedge clk) begin
        if (!reset_n) begin
            insn_addr <= `RESETVECTOR;
            rhalt <= 0;
            cycle_state <= 'b1;
        end

        if (_en)
            cycle_state <= {cycle_state[1:0],cycle_state[2] & ~|halt};

        case (cycle_state)
            3'b010: begin
                if (_en && reset_n)
                    rhalt <= rhalt | illegal;
            end
            3'b100: begin
                if (_en && ~|halt)
                    insn_addr <= jumping ? reg_valueZ : insn_addr + 1;
            end
        endcase
    end

    // Decode happens as soon as instruction is ready. Reading registers also
    // happens at this time.
    Decode decode(.insn(insn_data), .op(op), .deref(deref), .type(type),
                  .illegal(illegal),
                  .Z(indexZ), .X(indexX), .Y(indexY), .I(valueI));

    // Execution (arithmetic operation) happen the cycle after decode
    Exec exec(.clk(clk & cycle_state[1]), .en(_en), .op(op), .type(type),
              .rhs(rhs), .X(valueX), .Y(valueY), .I(valueI), .valid(1));
              // TODO only execute when "valid" (what means it ?)

    // Memory reads currently happen the half-cycle between execute and
    // register write-back

    // Registers and memory get written last, the cycle after execution
    Reg regs(.clk(clk & cycle_state[2]), .en(_en), .pc(insn_addr), .rwZ(reg_rw),
             .indexX(indexX), .indexY(indexY), .indexZ(indexZ),
             .valueX(valueX), .valueY(valueY), .valueZ(reg_valueZ));

endmodule

