`include "common.vh"
`timescale 1ns/10ps

module Reg(input clk, en, rwZ, input[3:0] indexZ, indexX, indexY,
           inout[31:0] valueZ, output[31:0] valueX, valueY, input[31:0] pc,
           input writeP);

    //(* KEEP = "TRUE" *)
    reg[31:0] store[1:15];

    wire ZisP = &indexZ;
    wire XisP = &indexX;
    wire YisP = &indexY;

    wire Zis0 = ~|indexZ;
    wire Xis0 = ~|indexX;
    wire Yis0 = ~|indexY;

    wire[31:0] pcp1 = ~en ?  pc : pc + 1;
    assign valueZ   = ~en ? 'bz : rwZ ? 32'bz : (Zis0 ? 0 : ZisP ? pcp1 : store[indexZ]);
    assign valueX   = ~en ? 'bz :               (Xis0 ? 0 : XisP ? pcp1 : store[indexX]);
    assign valueY   = ~en ? 'bz :               (Yis0 ? 0 : YisP ? pcp1 : store[indexY]);

    always @(negedge clk) if (en) begin
        if (writeP)
            store[15] = pc;
        if (rwZ) begin
            if (indexZ == 0)
                $display("wrote to zero register");
            else begin
                store[indexZ] = valueZ;
            end
        end
    end

endmodule

module Decode(input[31:0] insn, input en,
              output[3:0] Z, output[3:0] X, output[3:0] Y, output[11:0] I,
              output[3:0] op, output[1:0] deref, output type, output illegal,
              output valid);

    wire iclass     = ~en ? 'bz : insn[31];
    wire except     = ~en ? 'bz : &insn[31:28];
    assign type     = ~en ? 'bz : insn[30];
    assign deref    = ~en ? 'bz : insn[28 +: 2];
    assign Z        = ~en ? 'bz : insn[24 +: 4];
    assign X        = ~en ? 'bz : insn[20 +: 4];
    assign Y        = ~en ? 'bz : insn[16 +: 4];
    assign op       = ~en ? 'bz : insn[12 +: 4];
    assign I        = ~en ? 'bz : insn[ 0 +:12];
    assign illegal  = ~en ? 'bz : (except & &insn[27:0]);
    assign valid    = ~en ? 'bz : (~iclass | illegal);

endmodule

module Exec(input clk, input en, output signed[31:0] rhs,
            input signed[31:0] X, input signed[31:0] Y, input signed[11:0] I,
            input[3:0] op, input type, input valid);

    assign rhs = valid ? i_rhs : 32'b0;
    reg signed[31:0] i_rhs = 0;

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
                4'b1100: i_rhs =  (X  ^ ~O) + A; // X xor ones' complement Y
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

module Core(input clk, en, output[31:0] insn_addr, input[31:0] insn_data,
            output rw, output[31:0] norm_addr, inout[31:0] norm_data,
            input reset_n, `HALTTYPE halt);

    wire _en = en && reset_n;

    wire[31:0] rhs;
    wire[1:0] deref;
    wire reg_rw;

    wire[3:0]  indexX, indexY, indexZ;
    wire[31:0] valueX, valueY;

    wire[31:0] deref_rhs, reg_valueZ, deref_lhs;
    wire[31:0] valueZ = reg_rw ? deref_rhs : reg_valueZ;
    wire[11:0] valueI;
    wire[3:0] op;
    wire illegal, type;
    reg insn_valid = 1; // XXX
    reg manual_invalidate = 0;
    wire decode_valid; // FIXME decode_valid never deasserts
    wire state_valid = insn_valid && !manual_invalidate;
    assign deref_rhs = (deref[0] && !rw) ? norm_data : rhs;
    assign deref_lhs = (deref[1] && !rw) ? norm_data : reg_valueZ;
    assign reg_valueZ = reg_rw ? valueZ : 32'bz;

    reg rhalt = 0;
    assign halt[`HALT_EXEC] = rhalt;
    wire lhalt = |halt;

    // [Z] <-  ...  -- deref == 10
    //  Z  -> [...] -- deref == 11
    wire mem_active = (state_valid && !illegal) ? |deref : 1'b0;
    wire rw = mem_active ? deref[1] : 1'b0;
    // TODO Find a safe place to use for default address
    wire[31:0] mem_addr = mem_active ? (deref[0] ? rhs : valueZ) : 32'b0;
    wire[31:0] mem_data = rw         ? (deref[0] ? valueZ : rhs) : 32'b0;
    assign norm_data = (rw && mem_active) ? mem_data : 32'bz;
    assign norm_addr = mem_active ? mem_addr : 32'b0;
    //  Z  <-  ...  -- deref == 00
    //  Z  <- [...] -- deref == 01
    assign reg_rw  = state_valid ? (~deref[1] && indexZ != 0) : 1'b0;
    wire   jumping = state_valid ? (indexZ == 15 && reg_rw)   : 1'b0 ;
    reg[31:0] insn_addr = `RESETVECTOR;

    wire[31:0] insn = state_valid ? insn_data : 32'b0;

    reg[2:0] cycle_state = 1'b1;

    always @(negedge clk) begin
        cycle_state <= {cycle_state[1:0],cycle_state[2]};
        case (cycle_state)
            3'b001: begin
                if (_en && reset_n) begin
                    rhalt <= (rhalt | (insn_valid ? illegal : 1'b0));
                end
            end
            3'b010: begin
                if (_en && state_valid) begin
                    if (!reset_n) begin
                        insn_addr <= `RESETVECTOR;
                    end else if (!lhalt) begin
                        insn_addr <= jumping ? deref_lhs : insn_addr + 1;
                    end
                end
            end
        endcase
    end

    // Decode happens as soon as instruction is ready. Reading registers also
    // happens at this time.
    Decode decode(.en(_en && state_valid), .insn(insn), .op(op), .deref(deref), .type(type),
                  .valid(decode_valid), .illegal(illegal),
                  .Z(indexZ), .X(indexX), .Y(indexY), .I(valueI));

    // Execution (arithmetic operation) happen the cycle after decode
    Exec exec(.clk(clk & cycle_state[1]), .en(_en && state_valid), .op(op), .type(type), .rhs(rhs),
              .X(valueX), .Y(valueY), .I(valueI),
              .valid(state_valid));

    // Memory reads currently happen the half-cycle between execute and
    // register write-back

    // Registers and memory get written last, the cycle after execution
    Reg regs(.clk(clk & cycle_state[2]), .en(_en && state_valid), .pc(insn_addr), .writeP(1'b1), .rwZ(reg_rw),
             .indexX(indexX), .indexY(indexY), .indexZ(indexZ),
             .valueX(valueX), .valueY(valueY), .valueZ(reg_valueZ));

endmodule

