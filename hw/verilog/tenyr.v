`timescale 1ms/10us

`define CLOCKPERIOD 10
`define RAMDELAY (1 * `CLOCKPERIOD)
// TODO use proper reset vectors
`define RESETVECTOR 'h1000
`define SETUPTIME 2
`define SETUP #(`SETUPTIME)
`define DECODETIME `SETUP
`define EXECTIME `SETUP

// Two-port memory required if we don't have wait states ; one instruction
// fetch per cycle, and up to one read or write. Port 0 is R/W ; port 1 is R/O
module Mem(input clk, input enable, input p0rw,
        input[31:0] p0_addr, inout[31:0] p0_data,
        input[31:0] p1_addr, inout[31:0] p1_data
        );
    parameter BASE = 1 << 12; // TODO pull from environmental define
    parameter SIZE = (1 << 24) - BASE;

    reg[31:0] store[(SIZE + BASE - 1):BASE];

    wire p0_inrange = (p0_addr >= BASE && p0_addr < SIZE + BASE);
    wire p1_inrange = (p1_addr >= BASE && p1_addr < SIZE + BASE);

    assign p0_data = (enable && p0_inrange && !p0rw) ? store[p0_addr] : 32'bz;
    assign p1_data = (enable && p1_inrange         ) ? store[p1_addr] : 32'bz;

    initial begin
        `include "small.vh"
    end

    always @(negedge clk)
        if (enable && p0_inrange && p0rw)
            store[p0_addr] <= `SETUP p0_data;

endmodule

module Reg(input clk,
        input rwZ, input[3:0] indexZ, inout [31:0] valueZ, // Z is RW
                   input[3:0] indexX, output[31:0] valueX, // X is RO
                   input[3:0] indexY, output[31:0] valueY, // Y is RO
        inout[31:0] pc, input rwP);

    reg[31:0] store[0:15];

    generate
        genvar i;
        for (i = 0; i < 15; i = i + 1) // P is set externally
            initial #0 store[i] <= 'b0;
    endgenerate

    assign pc     = rwP ? 32'bz : store[15];
    assign valueZ = rwZ ? 32'bz : store[indexZ];
    assign valueX = store[indexX];
    assign valueY = store[indexY];

    always @(negedge clk) begin
        if (rwP)
            store[15] <= `SETUP pc;
        if (rwZ) begin
            if (indexZ == 0)
                $display("wrote to zero register");
            else begin
                store[indexZ] <= `SETUP valueZ;
            end
        end
    end

endmodule

module Decode(input[31:0] insn, output[3:0] Z, X, Y, output[11:0] I,
              output[3:0] op, output[1:0] deref, output flip, type, illegal,
              valid);

    reg[3:0] rZ = 0, rX = 0, rY = 0, rop = 0;
    reg[11:0] rI = 0;
    reg[1:0] rderef = 0;
    reg rflip = 0, rtype = 0, rillegal = 0, rvalid = 0;

    assign valid = rvalid;

    assign Z       = rvalid ? rZ       :  4'bz,
           X       = rvalid ? rX       :  4'bz,
           Y       = rvalid ? rY       :  4'bz,
           I       = rvalid ? rI       : 12'bz,
           op      = rvalid ? rop      :  4'bz,
           deref   = rvalid ? rderef   :  2'bz,
           flip    = rvalid ? rflip    :  1'bz,
           type    = rvalid ? rtype    :  1'bz,
           illegal = &insn;

    always @(insn) begin
        casez (insn[31:28])
            4'b0???: begin
                rderef <= { insn[29] & ~insn[28], insn[28] };
                rflip  <= insn[29] & insn[28];
                rtype  <= insn[30];

                rZ  <= insn[24 +: 4];
                rX  <= insn[20 +: 4];
                rY  <= insn[16 +: 4];
                rop <= insn[12 +: 4];
                rI  <= insn[ 0 +:12];
                rvalid <= `DECODETIME 1;
            end
            default: begin
                rvalid <= `DECODETIME &insn[31:28];
                rderef <= 2'bz;
                rflip  <= 1'bz;
                rtype  <= 1'bz;

                rZ  <=  4'bz;
                rX  <=  4'bz;
                rY  <=  4'bz;
                rop <=  4'bz;
                rI  <= 12'bz;
            end
        endcase
    end

endmodule

module Exec(input clk, output[31:0] rhs, input[31:0] X, Y, input[11:0] I,
            input[3:0] op, input flip, type, valid);

    assign rhs = valid ? i_rhs : 32'bz;
    reg[31:0] i_rhs = 0;

    // TODO signed net or integer support
    wire[31:0] Xs = X;
    wire[31:0] Xu = X;

    wire[31:0] Is = { {20{I[11]}}, I };
    wire[31:0] Ou = (type == 0) ? Y  : Is;
    wire[31:0] Os = (type == 0) ? Y  : Is;
    wire[31:0] As = (type == 0) ? Is : Y;

    always @(negedge clk) begin
        case (op)
            4'b0000: i_rhs = `EXECTIME  (Xu  |  Ou) + As; // X bitwise or Y
            4'b0001: i_rhs = `EXECTIME  (Xu  &  Ou) + As; // X bitwise and Y
            4'b0010: i_rhs = `EXECTIME  (Xs  +  Os) + As; // X add Y
            4'b0011: i_rhs = `EXECTIME  (Xs  *  Os) + As; // X multiply Y
          //4'b0100:                                   // reserved
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

            //default: $stop;
        endcase
    end

endmodule

module Core(input clk, output[31:0] insn_addr, input[31:0] insn_data,
            output rw, output[31:0] norm_addr, inout[31:0] norm_data,
            input _reset, output halt);
    reg[31:0] _norm_addr = 0;

    assign norm_addr = _norm_addr;

    wire[3:0]  indexX, indexY, indexZ;
    wire[31:0] valueX, valueY;
    wire[31:0] valueZ = reg_rw ? rhs : 32'bz;
    wire[11:0] valueI;
    wire[3:0] op;
    wire flip, illegal, type, insn_valid;
    reg state_valid = 0;
    wire[31:0] rhs;
    wire[1:0] deref;

    reg _halt = 1;
    assign halt = _reset ? 1'bz : _halt;

    // [Z] <-  ...  -- deref == 10
    //  Z  -> [...] -- deref == 11
    wire mem_active = |deref;
    assign rw = mem_active ? deref[1] : 1'b1;
    wire[31:0] mem_operand = mem_active ? (deref[0] ? valueZ : rhs) : 32'bz;
    assign norm_data = mem_active ? mem_operand : 32'bz;
    //  Z  <-  ...  -- deref == 00
    //  Z  <- [...] -- deref == 01
    wire reg_rw = ~deref[0] && indexZ != 0;
    wire jumping = indexZ == 15 && reg_rw;
    reg[31:0] new_pc    = `RESETVECTOR,
              next_pc   = `RESETVECTOR;
    wire[31:0] pc = _halt   ? new_pc :
                    jumping ? new_pc : next_pc;

    assign insn_addr = halt ? 32'bz : pc;

    always @(negedge clk, negedge _reset) begin
        if (!_reset) begin
            state_valid <= 1;
            // TODO use proper reset vectors
            new_pc      <= `RESETVECTOR;
            next_pc     <= `RESETVECTOR;
        end else begin
            _halt <= `SETUP (halt | illegal);
            if (!_halt) begin
                state_valid <= `SETUP state_valid & insn_valid;
                next_pc <= `SETUP pc + 1;
                if (jumping)
                    new_pc <= `SETUP valueZ;
            end
        end
    end

    Reg regs(.clk(clk), .pc(pc), .rwP(1'b1), .rwZ(reg_rw),
             .indexX(indexX), .indexY(indexY), .indexZ(indexZ),
             .valueX(valueX), .valueY(valueY), .valueZ(valueZ));

    Decode decode(.insn(insn_data), .op(op), .flip(flip),
                  .deref(deref), .type(type), .valid(insn_valid),
                  .Z(indexZ), .X(indexX), .Y(indexY), .I(valueI),
                  .illegal(illegal));

    Exec exec(.clk(clk), .op(op), .flip(flip), .type(type),
              .rhs(rhs), .X(valueX), .Y(valueY), .I(valueI),
              .valid(state_valid));
endmodule

module Tenyr();
    reg halt = 1;
    reg _reset = 0;
    reg clk;

    initial clk = 0;
    // TODO halt ?
    always #(`CLOCKPERIOD / 2) clk = ~clk;

    wire[31:0] insn_addr, operand_addr;
    wire[31:0] insn_data, operand_data;
    wire operand_rw;

    initial #(2 * `CLOCKPERIOD) halt = `SETUP 0;
    initial #(1 * `CLOCKPERIOD) _reset = `SETUP 1;

    wire _halt = _reset ? halt : 1'bz;
    always @(negedge clk) halt <= _halt;

    Mem #(.SIZE(16)) ram(.clk(clk), .enable(!halt), .p0rw(operand_rw),
            .p0_addr(operand_addr), .p0_data(operand_data),
            .p1_addr(insn_addr)   , .p1_data(insn_data));

/*
    SimSerial serial(.clk(clk), ._reset(_reset), .enable(!halt),
                     .rw(operand_rw), .addr(operand_addr),
                     .data(operand_data));
*/

    Core core(.clk(clk), ._reset(_reset), .rw(operand_rw),
            .norm_addr(operand_addr), .norm_data(operand_data),
            .insn_addr(insn_addr)   , .insn_data(insn_data), .halt(_halt));
endmodule

