`timescale 1ms/10us

`define CLOCKPERIOD 10
`define RAMDELAY (1 * `CLOCKPERIOD)

module SimSerial(input clk, input enable, input rw,
        input[31:0] addr, inout[31:0] data
        );
    parameter BASE = 1 << 5;
    parameter SIZE = 2;

    wire in_range = (addr >= BASE && addr < SIZE + BASE);

    always @(negedge clk) begin
        if (enable && in_range) begin
            if (rw)
                $putchar(data);
            else
                $getchar(data);
        end
    end
endmodule

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

    assign p0_data = (enable && p0_inrange && !p0rw) ? store[p0_addr] : 'bz;
    assign p1_data = (enable && p1_inrange         ) ? store[p1_addr] : 'bz;

    always @(negedge clk)
        if (enable && p0_inrange && p0rw)
            store[p0_addr] = p0_data;

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
            initial #0 store[i] = 'b0;
    endgenerate

    assign pc     = rwP ? 'bz : store[15];
    assign valueZ = rwZ ? 'bz : store[indexZ];
    assign valueX = store[indexX];
    assign valueY = store[indexY];

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
              output[3:0] op, output[1:0] deref, output flip, type, illegal,
              valid);

    wire[31:0] insn;
    reg[3:0] rZ = 0, rX = 0, rY = 0, rop = 0;
    reg[11:0] rI = 0;
    reg[1:0] rderef = 0;
    reg rflip = 0, rtype = 0, rillegal = 0, valid = 0;

    assign Z = rZ, X = rX, Y = rY, I = rI, op = rop, deref = rderef,
           flip = rflip, type = rtype, illegal = rillegal;

    always @(insn) begin
        valid = 1;
        casex (insn[31:28])
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
            default: valid = 0;
        endcase
    end

endmodule

module Exec(input clk, output[31:0] rhs, input[31:0] X, Y, input[11:0] I,
            input[3:0] op, input flip, input type);

    wire[31:0] X, Y, O, Is;
    reg[31:0] rhs = 0;
    wire[31:0] Xu, Ou;
    // TODO signed net or integer support
    wire[31:0] Xs, Os, As;

    assign Xs = X;
    assign Xu = X;

    assign Is = { {20{I[11]}}, I };
    assign Ou = (type == 0) ? Y  : Is;
    assign Os = (type == 0) ? Y  : Is;
    assign As = (type == 0) ? Is : Y;

    always @(negedge clk) begin
        case (op)
            4'b0000: rhs =  (Xu  |  Ou) + As; // X bitwise or Y
            4'b0001: rhs =  (Xu  &  Ou) + As; // X bitwise and Y
            4'b0010: rhs =  (Xs  +  Os) + As; // X add Y
            4'b0011: rhs =  (Xs  *  Os) + As; // X multiply Y
          //4'b0100:                          // reserved
            4'b0101: rhs =  (Xu  << Ou) + As; // X shift left Y
            4'b0110: rhs =  (Xs  <= Os) + As; // X compare <= Y
            4'b0111: rhs =  (Xs  == Os) + As; // X compare == Y
            4'b1000: rhs = ~(Xu  |  Ou) + As; // X bitwise nor Y
            4'b1001: rhs = ~(Xu  &  Ou) + As; // X bitwise nand Y
            4'b1010: rhs =  (Xu  ^  Ou) + As; // X bitwise xor Y
            4'b1011: rhs =  (Xs  + -Os) + As; // X add two's complement Y
            4'b1100: rhs =  (Xu  ^ ~Ou) + As; // X xor ones' complement Y
            4'b1101: rhs =  (Xu  >> Ou) + As; // X shift right logical Y
            4'b1110: rhs =  (Xs  >  Os) + As; // X compare > Y
            4'b1111: rhs =  (Xs  != Os) + As; // X compare <> Y

            //default: $stop;
        endcase
    end

endmodule

module Core(input clk, output[31:0] insn_addr, input[31:0] insn_data,
            output rw, output[31:0] norm_addr, inout[31:0] norm_data);
    reg[31:0] norm_addr = 0;
    wire[3:0] indexZ,
              indexX,
              indexY;
    wire[31:0] valueZ = reg_rw ? rhs : 'bz;
    wire[31:0] valueX, valueY;
    wire[11:0] valueI;
    wire[3:0] op;
    wire flip, illegal, type, insn_valid;
    wire[31:0] rhs;
    wire[1:0] deref;

    wire[31:0] pc;
    // [Z] <-  ...  -- deref == 10
    //  Z  -> [...] -- deref == 11
    wire norm_rw = deref[1];
    //  Z  <-  ...  -- deref == 00
    //  Z  <- [...] -- deref == 01
    wire reg_rw = ~deref[0] && indexZ != 0;
    wire jumping = indexZ == 15 && reg_rw;
    wire writeP = 1; //XXX !jumping;
    reg[31:0] insn_addr = 4096; // TODO
    reg[31:0] new_pc = 4096, next_pc = 4096; // TODO
    assign pc = jumping ? new_pc : next_pc;
    always @(negedge clk) next_pc <= #2 pc + 1;
    always @(negedge clk) if (jumping) new_pc <= #2 valueZ;
    always @(negedge clk) insn_addr <= #2 pc;


    Reg regs(.clk(clk),
            .rwZ(reg_rw), .indexZ(indexZ), .valueZ(valueZ),
                          .indexX(indexX), .valueX(valueX),
                          .indexY(indexY), .valueY(valueY),
            .pc(pc), .rwP(writeP));

    Decode decode(.insn(insn_data), .Z(indexZ), .X(indexX),
                  .Y(indexY), .I(valueI), .op(op), .flip(flip),
                  .deref(deref), .type(type), .valid(insn_valid));
    Exec exec(.clk(clk), .rhs(rhs), .X(valueX), .Y(valueY),
              .I(valueI), .op(op), .flip(flip),
              .type(type));
endmodule

module Top();
    reg halt = 1;
    reg clk = 1; // TODO if we start at 0 it changes behaviour (shouldn't)
    always #(`CLOCKPERIOD / 2) if (!halt) clk = !clk;
    wire[31:0] pc, _operand_addr;
    wire[31:0] insn, _operand_data;
    wire _operand_rw;

    initial #(`CLOCKPERIOD * 2) halt = 0;

    initial #0 begin
        $dumpfile("Top.vcd");
        $dumpvars;
        #100 $finish;
    end

    Mem ram(.clk(clk), .enable('b1), .p0rw(_operand_rw),
            .p0_addr(_operand_addr), .p0_data(_operand_data),
            .p1_addr(pc)           , .p1_data(insn));
    SimSerial serial(.clk(clk), .enable('b1), .rw(_operand_rw),
                     .addr(_operand_addr), .data(_operand_data));
    Core core(.clk(clk), .rw(_operand_rw),
            .norm_addr(_operand_addr), .norm_data(_operand_data),
            .insn_addr(pc)           , .insn_data(insn));
endmodule

