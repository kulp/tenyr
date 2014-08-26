`include "common.vh"
`timescale 1ns/10ps

module Reg(input clk, upZ, output signed[31:0] valZ, valX, valY,
           input[3:0] idxZ, idxX, idxY, input signed[31:0] nextZ, nextP);

    reg[31:0] store[0:14];
    initial store[0] = 0;

    assign valX = &idxX ? nextP : store[idxX];
    assign valY = &idxY ? nextP : store[idxY];
    assign valZ = &idxZ ? nextP : store[idxZ];

    always @(posedge clk) if (&{upZ,|idxZ,~&idxZ}) store[idxZ] <= nextZ;

endmodule

module Decode(input[31:0] insn, output[3:0] idxZ, output reg[3:0] idxX, idxY,
              output reg signed[31:0] valI, output reg[3:0] op,
              output[1:0] kind, output storing, loading, deref_rhs, branching);

    wire [23:0] tI;

    assign {kind, storing, deref_rhs, idxZ, tI} = insn;

    always @* case (kind)
        2'b11:   {idxX,idxY,op,valI[23:0],valI[31:24]} = {32'h0,tI,{8{tI[23]}}};
        default: {idxX,idxY,op,valI[11:0],valI[31:12]} = {tI,{20{tI[11]}}};
    endcase

    assign branching = &idxZ & ~storing;
    assign loading   = deref_rhs & ~storing;

endmodule

module Shuf(input[1:0] kind, input signed[31:0] valX, valY, valI,
                        output reg signed[31:0] valA, valB, valC);

    always @* case (kind)
        2'b00: {valA,valB,valC} = {valX ,valY ,valI};
        2'b01: {valA,valB,valC} = {valX ,valI ,valY};
        2'b10: {valA,valB,valC} = {valI ,valX ,valY};
        2'b11: {valA,valB,valC} = {32'h0,32'h0,valI};
    endcase

endmodule

module Exec(input clk, en, output reg done, output reg[31:0] valZ,
            input[3:0] op, input signed[31:0] valA, valB, valC);

    reg signed[31:0] rY, rZ, rA, rB, rC;
    reg[3:0] rop;
    reg add, act, staged;

    always @(posedge clk) begin
        { rop , valZ , rA   , rB   , rC   , done , add , act    , staged } <=
        { op  , rZ   , valA , valB , valC , add  , act , staged , en     } ;
        if (staged) case (rop)
            4'b0000: rY <=  (rA  |  rB  ); 4'b1000: rY <= -(rA  >= rB);
            4'b0001: rY <=  (rA  &  rB  ); 4'b1001: rY <=  (rA  &~ rB);
            4'b0010: rY <=  (rA  +  rB  ); 4'b1010: rY <=  (rA  ^  rB);
            4'b0011: rY <=  (rA  *  rB  ); 4'b1011: rY <=  (rA  -  rB);
            4'b0100: rY <=  {rA,rB[11:0]}; 4'b1100: rY <=  (rA  ^~ rB);
            4'b0101: rY <=  (rA  << rB  ); 4'b1101: rY <=  (rA  >> rB);
            4'b0110: rY <= -(rA  <  rB  ); 4'b1110: rY <= -(rA  != rB);
            4'b0111: rY <= -(rA  == rB  ); 4'b1111: rY <=  (rA >>> rB);
        endcase
        if (act) rZ <= rY + rC;
    end

endmodule

module Core(input clk, reset_n, inout wor `HALTTYPE halt, output strobe,
            output reg[31:0] i_addr, input[31:0] i_data, output mem_rw,
            output    [31:0] d_addr, input[31:0] d_in  , output[31:0] d_out);

    localparam[3:0] s0=0, s1=1, s2=2, s3=3, s4=4, s5=5, s6=6;

    wire deref_rhs, branching, storing, loading, edone;
    wire[3:0] idxX, idxY, idxZ, op;
    wire signed[31:0] valX, valY, valZ, valI, valA, valB, valC, rhs, nextZ;
    wire[1:0] kind;
    reg[31:0] insn, r_data;
    reg signed[31:0] r_irhs, nextP;
    reg[3:0] state = s5;

    always @(posedge clk)
        if (!reset_n) begin
            state  <= s5;
            i_addr <= `RESETVECTOR;
        end else case (state)
            s0: begin state <= halt  ? s0 : s1;                         end
            s1: begin state <= edone ? s2 : s1; r_irhs <= rhs;          end
            s2: begin state <= s3; /* compensate for slow memory */     end
            s3: begin state <= s4; r_data <= d_in;                      end
            s4: begin state <= s5; i_addr <= branching ? nextZ : nextP; end
            s5: begin state <= s6; nextP  <= i_addr + 1;                end
            s6: begin state <= s0; insn   <= i_data; /* why extra ? */  end
        endcase

    Decode decode(.insn, .op, .idxZ, .idxX, .idxY, .valI, .deref_rhs,
                  .kind, .branching, .loading, .storing);

    Shuf shuf(.kind, .valX, .valA, .valY, .valB, .valI, .valC);

    Exec exec(.clk, .en ( state == s0 ), .done ( edone ),
              .op, .valA, .valB, .valC,  .valZ ( rhs   ));

    assign mem_rw = storing;
    assign strobe = state == s3 && (loading || storing);
    assign nextZ  = deref_rhs ? r_data : r_irhs;
    assign d_out  = deref_rhs ? valZ   : r_irhs;
    assign d_addr = deref_rhs ? r_irhs : valZ;

    wire upZ = !storing && state == s4;
    Reg regs(.clk, .nextP, .idxX, .idxY, .idxZ,
             .upZ, .nextZ, .valX, .valY, .valZ);

endmodule

