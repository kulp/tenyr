`include "common.vh"
`timescale 1ns/10ps

module Reg(input clk,
        input rwZ, input[3:0] indexZ, inout [31:0] valueZ, // Z is RW
                   input[3:0] indexX, output[31:0] valueX, // X is RO
                   input[3:0] indexY, output[31:0] valueY, // Y is RO
        inout[31:0] pc, input rwP);

    (* KEEP = "TRUE" *)
    reg[31:0] store[0:15];

    generate
        genvar i;
        for (i = 0; i < 15; i = i + 1) // P is set externally
            initial begin:setup
                #0 store[i] = 32'b0;
            end
    endgenerate

    wire ZisP = indexZ == 15;
    wire XisP = indexX == 15;
    wire YisP = indexY == 15;

    assign pc     = rwP ? 32'bz : store[15];
    assign valueZ = rwZ ? 32'bz : (ZisP ? pc : store[indexZ]);
    assign valueX = XisP ? pc : store[indexX];
    assign valueY = YisP ? pc : store[indexY];

    always @(negedge clk) begin
        if (rwP)
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

module Decode(input[31:0] insn, output[3:0] Z, X, Y, output[11:0] I,
              output[3:0] op, output[1:0] deref, output type, illegal,
              valid);

    reg[3:0] rZ = 0, rX = 0, rY = 0, rop = 0;
    reg[11:0] rI = 0;
    reg[1:0] rderef = 0;
    reg rtype = 0, rillegal = 0, rvalid = 0;

    assign valid = rvalid;

    assign Z       = rvalid ? rZ       :  4'bx,
           X       = rvalid ? rX       :  4'bx,
           Y       = rvalid ? rY       :  4'bx,
           I       = rvalid ? rI       : 12'bx,
           op      = rvalid ? rop      :  4'bx,
           deref   = rvalid ? rderef   :  2'bx,
           type    = rvalid ? rtype    :  1'bx,
           illegal = rvalid ? rillegal :  1'b0;

    always @(insn) begin
        casez (insn[31:28])
            4'b0???: begin
                rderef <= insn[28 +: 2];
                rtype  <= insn[30];

                rZ  <= insn[24 +: 4];
                rX  <= insn[20 +: 4];
                rY  <= insn[16 +: 4];
                rop <= insn[12 +: 4];
                rI  <= insn[ 0 +:12];
                rvalid <= `DECODETIME 1;
                rillegal <= 0;
            end
            default: begin
                casex (insn[31:28])
                    4'b1111: begin
                        rillegal <= 1;
                        rvalid   <= 1; //&insn[27:0];
                    end
                    default: begin
                        rillegal <= 0;
                        rvalid   <= 0;
                    end
                endcase
                //rillegal <= &insn[31:28];
                rderef <= 2'bx;
                rtype  <= 1'bx;

                rZ  <=  4'bx;
                rX  <=  4'bx;
                rY  <=  4'bx;
                rop <=  4'bx;
                rI  <= 12'bx;
            end
        endcase
    end

endmodule

module Exec(input clk, output[31:0] rhs, input[31:0] X, Y, input[11:0] I,
            input[3:0] op, input type, valid);

    assign rhs = valid ? i_rhs : 32'bx;
    reg[31:0] i_rhs = 0;

    // TODO signed net or integer support
    wire[31:0] Xs = X;
    wire[31:0] Xu = X;

    wire[31:0] Is = { {20{I[11]}}, I };
    wire[31:0] Ou = (type == 0) ? Y  : Is;
    wire[31:0] Os = (type == 0) ? Y  : Is;
    wire[31:0] As = (type == 0) ? Is : Y;

    // cheat and use posedge clk to do calculations so they are ready by
    // negedge clk
    always @(posedge clk) begin
        if (valid) begin
            case (op)
                4'b0000: i_rhs = `EXECTIME  (Xu  |  Ou) + As; // X bitwise or Y
                4'b0001: i_rhs = `EXECTIME  (Xu  &  Ou) + As; // X bitwise and Y
                4'b0010: i_rhs = `EXECTIME  (Xs  +  Os) + As; // X add Y
                4'b0011: i_rhs = `EXECTIME  (Xs  *  Os) + As; // X multiply Y
              //4'b0100:                                      // reserved
                4'b0101: i_rhs = `EXECTIME  (Xu  << Ou) + As; // X shift left Y
                4'b0110: i_rhs = `EXECTIME  (Xs  <= Os) + As; // X compare <= Y
                4'b0111: i_rhs = `EXECTIME  (Xs  == Os) + As; // X compare == Y
                4'b1000: i_rhs = `EXECTIME ~(Xu  |  Ou) + As; // X bitwise nor Y
                4'b1001: i_rhs = `EXECTIME ~(Xu  &  Ou) + As; // X bitwise nand Y
                4'b1010: i_rhs = `EXECTIME  (Xu  ^  Ou) + As; // X bitwise xor Y
                4'b1011: i_rhs = `EXECTIME  (Xs  + -Os) + As; // X add two's complement Y
                4'b1100: i_rhs = `EXECTIME  (Xu  ^ ~Ou) + As; // X xor ones' complement Y
                4'b1101: i_rhs = `EXECTIME  (Xu  >> Ou) + As; // X shift right logical Y
                4'b1110: i_rhs = `EXECTIME  (Xs  >  Os) + As; // X compare > Y
                4'b1111: i_rhs = `EXECTIME  (Xs  != Os) + As; // X compare <> Y

                default: i_rhs = 32'bx;
            endcase
        end else begin
            i_rhs = 32'bx;
        end
    end

endmodule

module Core(clk, insn_addr, insn_data, rw, norm_addr, norm_data, _reset, halt);
    input clk;
    output[31:0] insn_addr;
    input[31:0] insn_data;
    output rw;
    output[31:0] norm_addr;
    inout[31:0] norm_data;
    input _reset;

    wire[3:0]  indexX, indexY, indexZ;
    wire[31:0] valueX, valueY;
    wire[31:0] valueZ = reg_rw ? rhs : 32'bz;
    wire[11:0] valueI;
    wire[3:0] op;
    wire illegal, type, insn_valid;
    reg state_valid = 0;
    wire[31:0] rhs;
    wire[1:0] deref;

    reg rhalt = 0;
    output wor halt;
   
    assign halt = rhalt ? rhalt : 1'bz;

    // [Z] <-  ...  -- deref == 10
    //  Z  -> [...] -- deref == 11
    wire mem_active = state_valid ? |deref : 1'b0;
    assign rw = mem_active ? deref[1] : 1'b0;
    wire[31:0] mem_data = state_valid ? (deref[0] ? valueZ : rhs) : 32'bz;
    wire[31:0] mem_addr = state_valid ? (deref[0] ? rhs : valueZ) : 32'bz;
    assign norm_data = (rw && mem_active) ? mem_data : 32'bz;
    assign norm_addr = mem_active ? mem_addr : 32'b0;
    //  Z  <-  ...  -- deref == 00
    //  Z  <- [...] -- deref == 01
    wire reg_rw = ~deref[0] && indexZ != 0;
    wire jumping = indexZ == 15 && reg_rw;
    reg[31:0] new_pc    = `RESETVECTOR,
              next_pc   = `RESETVECTOR;
//    wire[31:0] pc = halt    ? new_pc :
//                    jumping ? new_pc : next_pc;
// FIXME
    wire[31:0] pc = next_pc;

    assign insn_addr = halt ? 32'bz : pc;
    wire[31:0] insn = state_valid ? insn_data : 32'b0;

    always @(negedge clk or negedge _reset) begin
        if (!_reset) begin
            new_pc      = `RESETVECTOR;
            next_pc     = `RESETVECTOR;
        end else if (!halt) begin
            state_valid = 1;
        end
    end

    always @(negedge clk) begin
        if (state_valid) begin
            //rhalt <= 0;
            // FIXME
            if (illegal)
                state_valid = 0;
            rhalt <= `SETUP (rhalt | insn_valid ? illegal : 0);
            if (!rhalt && !halt && state_valid) begin
                state_valid <= `SETUP state_valid & insn_valid & !illegal;
                next_pc <= #(`CLOCKPERIOD / 2) pc + 1;
                if (jumping)
                    new_pc <= #(`CLOCKPERIOD / 2) valueZ;
            end
        end
    end

    Reg regs(.clk(clk), .pc(pc), .rwP(1'b1), .rwZ(reg_rw),
             .indexX(indexX), .indexY(indexY), .indexZ(indexZ),
             .valueX(valueX), .valueY(valueY), .valueZ(valueZ));

    Decode decode(.insn(insn), .op(op), .deref(deref), .type(type),
                  .valid(insn_valid), .illegal(illegal),
                  .Z(indexZ), .X(indexX), .Y(indexY), .I(valueI));

    Exec exec(.clk(clk), .op(op), .type(type), .rhs(rhs),
              .X(valueX), .Y(valueY), .I(valueI),
              .valid(state_valid));
endmodule

