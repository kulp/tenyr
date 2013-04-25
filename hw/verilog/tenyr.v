`include "common.vh"
`timescale 1ns/10ps

module Reg(input clk, en, upZ,           input[ 3:0] indexZ, indexX, indexY,
           input[31:0] writeZ, next_pc, output[31:0] valueZ, valueX, valueY);

    reg[31:0] store[1:14];

    wire XisP = &indexX, Xis0 = ~|indexX,
         YisP = &indexY, Yis0 = ~|indexY,
         ZisP = &indexZ, Zis0 = ~|indexZ;

    assign valueX = Xis0 ? 0 : XisP ? next_pc : store[indexX];
    assign valueY = Yis0 ? 0 : YisP ? next_pc : store[indexY];
    assign valueZ = Zis0 ? 0 : ZisP ? next_pc : store[indexZ];

    always @(posedge clk) if (en & upZ & ~Zis0 & ~ZisP) store[indexZ] <= writeZ;

endmodule

module Decode(input[31:0] insn, output[3:0] Z, X, Y, output[11:0] I,
              output[3:0] op, output kind, illegal, storing, deref_rhs, branch);

    assign {kind, storing, deref_rhs, Z, X, Y, op, I} = insn;
    assign illegal = &insn;
    assign branch  = &Z && !storing;

endmodule

module Exec(input clk, en, swap, output reg[31:0] rhs, input[3:0] op,
            input signed[31:0] X, Y, I);

    wire[31:0] O = swap ? I : Y;
    wire[31:0] A = swap ? Y : I;
    always @(posedge clk) if (en)
        case (op)
            4'b0000: rhs <=  (X  |  O) + A; // X bitwise or Y
            4'b0001: rhs <=  (X  &  O) + A; // X bitwise and Y
            4'b0010: rhs <=  (X  +  O) + A; // X add Y
            4'b0011: rhs <=  (X  *  O) + A; // X multiply Y
            4'b0100: rhs <= 32'bx;          // reserved
            4'b0101: rhs <=  (X  << O) + A; // X shift left Y
            4'b0110: rhs <= -(X  <  O) + A; // X compare < Y
            4'b0111: rhs <= -(X  == O) + A; // X compare == Y
            4'b1000: rhs <= -(X  >  O) + A; // X compare > Y
            4'b1001: rhs <=  (X  &~ O) + A; // X bitwise and complement Y
            4'b1010: rhs <=  (X  ^  O) + A; // X bitwise xor Y
            4'b1011: rhs <=  (X  -  O) + A; // X subtract Y
            4'b1100: rhs <=  (X  ^~ O) + A; // X xor ones' complement Y
            4'b1101: rhs <=  (X  >> O) + A; // X shift right logical Y
            4'b1110: rhs <= -(X  != O) + A; // X compare <> Y
            4'b1111: rhs <= 32'bx;          // reserved
        endcase

endmodule

module Core(input clk, reset_n, inout `HALTTYPE halt,
                                   output reg[31:0] i_addr, input[31:0] i_data,
            output mem_rw, strobe, output    [31:0] d_addr, inout[31:0] d_data);

    localparam[2:0] sI=0, s0=1, s1=2, s2=3, s3=4, s4=5, s5=6, sH=7;

    wire illegal, kind, drhs, jumping, storing, loading;
    wire[ 3:0] indexX, indexY, indexZ, op;
    wire[31:0] valueX, valueY, valueZ, valueI, irhs, rhs, storand;

    reg [31:0] r_irhs, r_data, next_pc;
    reg [2:0] state = sI;
    assign halt[`HALT_EXEC] = state == sH;

    always @(posedge clk)
        if (!reset_n)
            state <= sI;
        else case (state)
            sI: begin state <= |halt ? sI : s5; i_addr <= `RESETVECTOR; end
            s0: begin state <= |halt ? sI : illegal ? sH : s1;          end
            s1: begin state <= s2; r_irhs  <= irhs;                     end
            s2: begin state <= s3; /* compensate for pipelined mult */  end
            s3: begin state <= s4; r_data  <= d_data;                   end
            s4: begin state <= s5; i_addr  <= jumping ? rhs : next_pc;  end
            s5: begin state <= s0; next_pc <= i_addr + 1;               end
            default:  state <= sH;
        endcase

    // Instruction fetch happens on cycle 0

    // Decode and register reads happen as soon as instruction is ready
    Decode decode(.Z ( indexZ ), .insn ( i_data ), .storing   ( storing ),
                  .X ( indexX ), .kind ( kind   ), .deref_rhs ( drhs    ),
                  .Y ( indexY ), .op   ( op     ), .branch    ( jumping ),
                  .I ( valueI ),                   .illegal   ( illegal ));

    // Execution (arithmetic operation) occurs on cycle 0
    wire en_ex = state == s0;
    wire[31:0] extI = { {20{valueI[11]}}, valueI[11:0] };
    Exec exec(.clk ( clk    ), .en ( en_ex  ), .op ( op   ), .swap ( kind ),
              .X   ( valueX ), .Y  ( valueY ), .I  ( extI ), .rhs  ( irhs ));

    // Memory loads or stores on cycle 3
    assign loading = drhs && !storing;
    assign strobe  = state == s3 && (loading || storing);
    assign mem_rw  = storing && strobe;
    assign rhs     = drhs    ? r_data  : r_irhs;
    assign storand = drhs    ? valueZ  : r_irhs;
    assign d_addr  = drhs    ? r_irhs  : valueZ;
    assign d_data  = storing ? storand : 32'bz;

    // Registers commit on cycle 4
    wire updateZ = !storing && state == s4;
    Reg regs(.clk     ( clk     ), .indexX ( indexX ), .valueX ( valueX  ),
             .en      ( 1'b1    ), .indexY ( indexY ), .valueY ( valueY  ),
             .next_pc ( next_pc ), .indexZ ( indexZ ), .valueZ ( valueZ  ),
                                   .writeZ ( rhs    ), .upZ    ( updateZ ));

endmodule

