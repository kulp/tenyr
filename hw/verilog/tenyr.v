`include "common.vh"
`timescale 1ns/10ps

module Reg(input clk, upZ,               input[ 3:0] indexZ, indexX, indexY,
           input[31:0] writeZ, next_pc, output[31:0] valueZ, valueX, valueY);

    reg[31:0] store[0:14] = { 0 };

    //              reg is P ?           reg is B-O
    assign valueX = &indexX ? next_pc : store[indexX];
    assign valueY = &indexY ? next_pc : store[indexY];
    assign valueZ = &indexZ ? next_pc : store[indexZ];

    always @(posedge clk) if (&{upZ,|indexZ,~&indexZ}) store[indexZ] <= writeZ;

endmodule

module Decode(input[31:0] insn, output[3:0] Z, X, Y, output[11:0] I,
              output[3:0] op, output kind, throw, storing, deref_rhs, branch,
              output[4:0] vector);

    assign {kind, storing, deref_rhs, Z, X, Y, op, I} = insn;
    assign vector = insn[4:0];
    assign throw  = &insn[31:30];
    assign branch = &Z & ~storing;

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

module Core(input clk, reset_n, trap, input wor `HALTTYPE halt, output strobe,
            output reg[31:0] i_addr, input[31:0] i_data, output mem_rw,
            output    [31:0] d_addr, input[31:0] d_in  , output[31:0] d_out);

    localparam[3:0] sI=0, s0=1, s1=2, s2=3, s3=4, s4=5, s5=6,
                    sE=7, sF=8, sR=9, sT=10, sW=11;

    wire throw, kind, drhs, jumping, storing, loading, deref;
    wire[ 3:0] indexX, indexY, indexZ, op;
    wire[31:0] valueX, valueY, valueZ, irhs, rhs, tostore;
    wire[11:0] valueI;
    wire[ 4:0] vector;
    reg [31:0] vect;

    reg [31:0] r_irhs, r_data, next_pc;
    reg [3:0] state = sI;
    assign halt[`HALT_EXEC] = 0; // TODO give EIB a register that pulls this

    always @(posedge clk)
        if (!reset_n)
            state <= sI;
        else case (state)
            s0: begin state <= halt ? sI : trap ? sF : throw ? sE : s1; end
            s1: begin state <= s2; r_irhs  <= irhs;                     end
            s2: begin state <= s3; /* compensate for slow multiplier */ end
            s3: begin state <= s4; r_data  <= d_in;                     end
            s4: begin state <= s5; i_addr  <= jumping ? rhs : next_pc;  end
            s5: begin state <= s0; next_pc <= i_addr + 1;               end
            sE: begin state <= sR; r_irhs  <= `VECTOR_ADDR | vector;    end
            sR: begin state <= sT; vect    <= d_in;   /* test this */   end
            sF: begin state <= sT; vect    <= `TRAMP_BOTTOM;            end
            sT: begin state <= sW; r_irhs  <= i_addr; /* wait for */    end
            sW: begin state <= s5; i_addr  <= vect; /* trap to fall */  end
            sI: begin state <= halt ? sI : s5; i_addr <= `RESETVECTOR;  end
        endcase

    // Instruction fetch happens on cycle 0

    // Decode and register reads happen as soon as instruction is ready
    Decode decode(.Z ( indexZ ), .insn  ( i_data ), .storing   ( storing ),
                  .X ( indexX ), .kind  ( kind   ), .deref_rhs ( drhs    ),
                  .Y ( indexY ), .op    ( op     ), .branch    ( jumping ),
                  .I ( valueI ), .throw ( throw  ), .vector    ( vector  ));

    // Execution (arithmetic operation) occurs on cycle 0
    wire en_ex = state == s0;
    wire[31:0] extI = { {20{valueI[11]}}, valueI[11:0] };
    Exec exec(.clk ( clk    ), .en ( en_ex  ), .op ( op   ), .swap ( kind ),
              .X   ( valueX ), .Y  ( valueY ), .I  ( extI ), .rhs  ( irhs ));

    // Memory loads or stores on cycle 3
    assign loading = (drhs && !mem_rw) || state == sR;
    assign strobe  = (state == s3 && (loading || storing)) || state == sW;
    assign mem_rw  = storing || state == sW;
    assign tostore = state == sW ? `TRAP_ADDR : valueZ;
    assign deref   = drhs && (state != sT);
    assign rhs     = deref ? r_data : r_irhs;
    assign d_out   = deref ? valueZ : r_irhs;
    assign d_addr  = deref ? r_irhs : tostore;

    // Registers commit on cycle 4
    wire updateZ = !storing && state == s4;
    Reg regs(.clk     ( clk         ), .indexX ( indexX ), .valueX ( valueX  ),
             .next_pc ( next_pc     ), .indexY ( indexY ), .valueY ( valueY  ),
                                       .indexZ ( indexZ ), .valueZ ( valueZ  ),
                                       .writeZ ( rhs    ), .upZ    ( updateZ ));

endmodule

