`include "common.vh"
`timescale 1ns/10ps

module Reg(input clk, upZ,             input[ 3:0] idxZ, idxX, idxY,
           input[31:0] nextZ, nextP, output[31:0] valZ, valX, valY);

    reg[31:0] store[0:14];
    initial store[0] = 0;

    //            reg is P ?      reg is B-O
    assign valX = &idxX ? nextP : store[idxX];
    assign valY = &idxY ? nextP : store[idxY];
    assign valZ = &idxZ ? nextP : store[idxZ];

    always @(posedge clk) if (&{upZ,|idxZ,~&idxZ}) store[idxZ] <= nextZ;

endmodule

module Decode(input[31:0] insn, output[3:0] Z, X, Y, output[31:0] I,
              output[3:0] op, output[1:0] kind,
              output throw, storing, deref_rhs, branch, output[4:0] vector);

    assign {kind, storing, deref_rhs, Z, X, Y, op, I[11:0]} = insn;
    assign I[31:12] = {20{I[11]}};
    assign vector = insn[4:0];
    assign throw  = &kind;
    assign branch = &Z & ~storing;

endmodule

module Shuf(input[1:0] kind, input[31:0] X, Y, I, output reg[31:0] A, B, C);

    always @* case (kind)
        2'b00:   {A,B,C} = {X,Y,I};
        2'b01:   {A,B,C} = {X,I,Y};
        2'b10:   {A,B,C} = {I,X,Y};
        default: {A,B,C} = 'bx;
    endcase

endmodule

module Exec(input clk, en, output reg done, output reg[31:0] Z, input[3:0] op,
            input signed[31:0] A, B, C);

    reg signed[31:0] rY, rZ, rA, rB, rC;
    reg[3:0] rop;
    reg add, act, staged;

    always @(posedge clk) begin
        {rop,Z,rA,rB,rC,done,add,act,staged} <= {op,rZ,A,B,C,add,act,staged,en};
        if (staged) case (rop)
            4'b0000: rY <=  (rA  |  rB); 4'b1000: rY <= -(rA  >= rB);
            4'b0001: rY <=  (rA  &  rB); 4'b1001: rY <=  (rA  &~ rB);
            4'b0010: rY <=  (rA  +  rB); 4'b1010: rY <=  (rA  ^  rB);
            4'b0011: rY <=  (rA  *  rB); 4'b1011: rY <=  (rA  -  rB);
            default: rY <= 32'bx;        4'b1100: rY <=  (rA  ^~ rB);
            4'b0101: rY <=  (rA  << rB); 4'b1101: rY <=  (rA  >> rB);
            4'b0110: rY <= -(rA  <  rB); 4'b1110: rY <= -(rA  != rB);
            4'b0111: rY <= -(rA  == rB); 4'b1111: rY <=  (rA >>> rB);
        endcase
        if (act) rZ <= rY + rC;
    end

endmodule

module Core(input clk, reset_n, trap, inout wor `HALTTYPE halt, output strobe,
            output reg[31:0] i_addr, input[31:0] i_data, output mem_rw,
            output    [31:0] d_addr, input[31:0] d_in  , output[31:0] d_out);

    localparam[3:0] sI=0, s0=1, s1=2, s2=3, s3=4, s4=5, s5=6, s6=7,
                    sE=8, sF=9, sR=10, sT=11, sW=12;

    wire throw, drhs, jumping, storing, loading, deref, edone;
    wire[ 3:0] idxX, idxY, idxZ, op;
    wire[31:0] valX, valY, valZ, valI, valA, valB, valC, rhs, nextZ;
    wire[ 4:0] vector;
    wire[ 1:0] kind;

    reg [31:0] r_irhs, r_data, nextP, v_addr, insn;
    reg [ 3:0] state = sI;

    always @(posedge clk)
        if (!reset_n)
            state <= sI;
        else case (state)
            s0: begin state <= halt ? sI : trap ? sF : throw ? sE : s1; end
            s1: begin state <= edone ? s2 : s1; r_irhs <= rhs;          end
            s2: begin state <= s3; /* compensate for slow memory */     end
            s3: begin state <= s4; r_data <= d_in;                      end
            s4: begin state <= s5; i_addr <= jumping ? nextZ : nextP;   end
            s5: begin state <= s6; nextP  <= i_addr + 1;                end
            s6: begin state <= s0; insn   <= i_data; /* why extra ? */  end
            // TODO return to instruction OF a trap, but AFTER a throw
            sE: begin state <= sR; r_irhs <= `VECTOR_ADDR | vector;     end
            sR: begin state <= sT; v_addr <= d_in;   /* test this */    end
            sF: begin state <= sT; v_addr <= `TRAMP_BOTTOM;             end
            sT: begin state <= sW; r_irhs <= i_addr; /* wait for */     end
            sW: begin state <= s5; i_addr <= v_addr; /* trap's fall */  end
            sI: begin state <= halt ? sI : s5; i_addr <= `RESETVECTOR;  end
        endcase

    // Instruction fetch happens on cycle 0

    // Decode and register reads happen as soon as instruction is ready
    Decode decode(.Z ( idxZ ), .insn  ( insn  ), .storing   ( storing ),
                  .X ( idxX ), .kind  ( kind  ), .deref_rhs ( drhs    ),
                  .Y ( idxY ), .op    ( op    ), .branch    ( jumping ),
                  .I ( valI ), .throw ( throw ), .vector    ( vector  ));

    // Shuffle occurs on cycle 0
    Shuf shuf(.kind ( kind ), .X ( valX ), .A ( valA ),
                              .Y ( valY ), .B ( valB ),
                              .I ( valI ), .C ( valC ));

    // Execution (arithmetic operation) occurs on cycle 0
    Exec exec (.clk ( clk         ), .op   ( op    ), .A ( valA ),
               .en  ( state == s0 ), .Z    ( rhs   ), .B ( valB ),
                                     .done ( edone ), .C ( valC ));

    // Memory loads or stores on cycle 3
    assign loading = (drhs && !mem_rw) || state == sR;
    assign strobe  = (state == s3 && (loading || storing)) || state == sW;
    assign mem_rw  = storing || state == sW;
    assign deref   = drhs && (state != sT);
    assign nextZ   = deref ? r_data : r_irhs;
    assign d_out   = deref ? valZ   : r_irhs;
    assign d_addr  = deref ? r_irhs : state == sW ? `TRAP_ADDR : valZ;

    // Registers commit on cycle 4
    wire upZ = !storing && state == s4;
    Reg regs(.clk   ( clk   ),               .idxX ( idxX ), .valX ( valX ),
             .nextP ( nextP ),               .idxY ( idxY ), .valY ( valY ),
             .nextZ ( nextZ ), .upZ ( upZ ), .idxZ ( idxZ ), .valZ ( valZ ));

endmodule

