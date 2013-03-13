`include "common.vh"
`timescale 1ns/10ps

module Reg(input clk, en, upZ,      input[ 3:0] indexZ, indexX, indexY,
           input[31:0] writeZ, pc, output[31:0] valueZ, valueX, valueY);

    reg[31:0] store[1:14];

    wire ZisP =  &indexZ, XisP =  &indexX, YisP =  &indexY;
    wire Zis0 = ~|indexZ, Xis0 = ~|indexX, Yis0 = ~|indexY;

    assign valueZ = ~en ? 'bz : Zis0 ? 0 : ZisP ? pc + 1 : store[indexZ];
    assign valueX = ~en ? 'bz : Xis0 ? 0 : XisP ? pc + 1 : store[indexX];
    assign valueY = ~en ? 'bz : Yis0 ? 0 : YisP ? pc + 1 : store[indexY];

    always @(negedge clk)
        if (en && upZ && !Zis0 && !ZisP)
            store[indexZ] = writeZ;

endmodule

module Decode(input[31:0] insn, output[3:0] Z, X, Y, output[31:0] I,
              output[3:0] op, output type, illegal, storing, deref_rhs, branch);

    assign type      = insn[30 +: 1];
    assign storing   = insn[29 +: 1];
    assign deref_rhs = insn[28 +: 1];
    assign Z         = insn[24 +: 4];
    assign X         = insn[20 +: 4];
    assign Y         = insn[16 +: 4];
    assign op        = insn[12 +: 4];
    wire[11:0] J     = insn[ 0 +:12];
    assign I         = { {20{J[11]}}, J };

    assign illegal   = &insn;
    assign branch    = &Z && !storing;

endmodule

module Exec(input clk, en, type, output reg[31:0] rhs,
            input signed[31:0] X, Y, I, input[3:0] op);

    wire signed[31:0] O = type ? I : Y;

    always @(negedge clk) if (en) begin
        case (op)
            4'b0000: rhs =  (X  |  O);  // X bitwise or Y
            4'b0001: rhs =  (X  &  O);  // X bitwise and Y
            4'b0010: rhs =  (X  +  O);  // X add Y
            4'b0011: rhs =  (X  *  O);  // X multiply Y
            4'b0100: rhs = 32'bx;       // reserved
            4'b0101: rhs =  (X  << O);  // X shift left Y
            4'b0110: rhs = -(X  <  O);  // X compare < Y
            4'b0111: rhs = -(X  == O);  // X compare == Y
            4'b1000: rhs = -(X  >  O);  // X compare > Y
            4'b1001: rhs =  (X  &~ O);  // X bitwise and complement Y
            4'b1010: rhs =  (X  ^  O);  // X bitwise xor Y
            4'b1011: rhs =  (X  -  O);  // X subtract Y
            4'b1100: rhs =  (X  ^~ O);  // X xor ones' complement Y
            4'b1101: rhs =  (X  >> O);  // X shift right logical Y
            4'b1110: rhs = -(X  != O);  // X compare <> Y
            4'b1111: rhs = 32'bx;       // reserved
        endcase

        rhs = rhs + (type ? Y : I);
    end

endmodule

module Core(input clk, input en, input reset_n, inout `HALTTYPE halt,
            output reg[31:0] i_addr, input[31:0] i_data,
            output mem_rw, output[31:0] d_addr, inout[31:0] d_data);

    wire illegal, type, storing, drhs, jumping;
    wire[ 3:0] indexX, indexY, indexZ;
    wire[31:0] valueX, valueY, valueZ, valueI, irhs;
    wire[ 3:0] op;

    wire _en     = en && reset_n;
    wire writing = !storing;
    wire loading = !storing && drhs;
    wire right   =  storing && drhs;

    wire[31:0] rhs     = drhs ? d_data : irhs;
    wire[31:0] storand = drhs ? valueZ : irhs;

    assign d_data = storing ? storand : 32'bz;
    assign d_addr = drhs    ? irhs    : valueZ;
    assign mem_rw = storing;

    reg [ 2:0] rcyc, rcycen;
    wire[ 2:0] cyc = rcyc & rcycen;
    reg rhalt;
    assign halt[`HALT_EXEC] = rhalt;

    always @(negedge clk) begin
        if (!reset_n) begin
            i_addr <= `RESETVECTOR;
            rhalt  <= 1'b0;
            rcyc   <= 3'b1;
            rcycen <= 3'b1;
        end

        if (_en) begin
            rcyc <= {rcyc[1:0],rcyc[2]};
            rcycen <= {rcycen[1:0],rcyc[2] & ~|halt};

            case (cyc)
                3'b010: rhalt <= rhalt | illegal;
                3'b100: i_addr <= jumping ? rhs : i_addr + 1;
            endcase
        end
    end

    // Decode and register reads happen as soon as instruction is ready.
    Decode decode(.insn(i_data), .op(op), .illegal(illegal), .type(type),
                  .Z(indexZ), .X(indexX), .Y(indexY), .I(valueI),
                  .storing(storing), .deref_rhs(drhs), .branch(jumping));

    // Execution (arithmetic operation) happen the cyc after decode
    Exec exec(.clk(clk), .en(_en & cyc[1]), .op(op), .type(type),
              .rhs(irhs), .X(valueX), .Y(valueY), .I(valueI));

    // Registers and memory get written last, the cyc after execution
    Reg regs(.clk(clk), .pc(i_addr), .indexX(indexX), .valueX(valueX),
             .en(_en), .writeZ(rhs), .indexY(indexY), .valueY(valueY),
             .upZ(writing & cyc[2]), .indexZ(indexZ), .valueZ(valueZ));

endmodule

