`include "common.vh"
`timescale 1ns/10ps

module Reg(input clk, upZ, output signed[31:0] valZ, valX, valY,
           input[3:0] idxZ, idxX, idxY, input signed[31:0] nextZ, nextP);

    reg[31:0] store[0:14];
    integer k;
    initial for (k = 0; k < 15; k = k + 1) store[k] = 0;

    assign valX = &idxX ? nextP : store[idxX];
    assign valY = &idxY ? nextP : store[idxY];
    assign valZ = &idxZ ? nextP : store[idxZ];

    always @(posedge clk) if (&{upZ,|idxZ,~&idxZ}) store[idxZ] <= nextZ;

endmodule

module Decode(input[31:0] insn, output[3:0] idxZ, idxX, output reg[3:0] idxY,
              output reg signed[31:0] valI, output reg[3:0] op,
              output[1:0] kind, output storing, loading, deref_rhs, branching);

    wire [19:0] tI;
    wire [1:0] dd;

    assign {kind, dd, idxZ, idxX, tI} = insn;

    always @* case (kind)
        default: {idxY,op,valI[11:0],valI[31:12]} = {      tI,{20{tI[11]}}};
        2'b11:   {idxY,op,valI[19:0],valI[31:20]} = {24'h0,tI,{12{tI[19]}}};
    endcase

    assign storing   = ^dd;
    assign loading   = &dd;
    assign deref_rhs = dd[0];
    assign branching = &idxZ & ~storing;

endmodule

module Shuf(input[1:0] kind, input signed[31:0] valX, valY, valI,
                        output reg signed[31:0] valA, valB, valC);

    always @* case (kind)
        2'b00: {valA,valB,valC} = {valX ,valY,valI};
        2'b01: {valA,valB,valC} = {valX ,valI,valY};
        2'b10: {valA,valB,valC} = {valI ,valX,valY};
        2'b11: {valA,valB,valC} = {32'h0,valX,valI};
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
            4'h0: rY <=  (rA  |  rB); 4'h8: rY <=  (rA  |~ rB  );
            4'h1: rY <=  (rA  &  rB); 4'h9: rY <=  (rA  &~ rB  );
            4'h2: rY <=  (rA  ^  rB); 4'ha: rY <=  {rA,rB[11:0]};
            4'h3: rY <=  (rA >>> rB); 4'hb: rY <=  (rA  >> rB  );
            4'h4: rY <=  (rA  +  rB); 4'hc: rY <=  (rA  -  rB  );
            4'h5: rY <=  (rA  *  rB); 4'hd: rY <=  (rA  << rB  );
            4'h6: rY <= -(rA  == rB); 4'he: rY <= -(!(!(rA & (1 << rB))));
            4'h7: rY <= -(rA  <  rB); 4'hf: rY <= -(rA  >= rB  );
        endcase
        if (act) rZ <= rY + rC;
    end

endmodule

module Core(
    input clk, reset,
    /* channel : data    insn    */
    output[31:0] adrD_o, adrI_o, // address
    input [31:0] datD_i, datI_i, // data in
    output[31:0] datD_o, datI_o, // data out
    output       wenD_o, wenI_o, // write enable
    output[ 3:0] selD_o, selI_o, // select
    output       stbD_o, stbI_o, // strobe
    input        ackD_i, ackI_i, // acknowledge
    input        errD_i, errI_i, // error
    input        rtyD_i, rtyI_i, // retry
    output       cycD_o, cycI_o, // cycle
    inout wor halt
);

    localparam[3:0] s0=0, s1=1, s2=2, s3=3, s4=4, s5=5, s6=6;

    wire deref_rhs, branching, storing, loading, edone;
    wire[3:0] idxX, idxY, idxZ, op;
    wire signed[31:0] valX, valY, valZ, valI, valA, valB, valC, rhs, nextZ;
    wire[1:0] kind;
    reg[31:0] insn, _data, _adrI_o;
    reg signed[31:0] _irhs, nextP;
    reg[3:0] state = s5;

    always @(posedge clk)
        if (reset) begin
            state   <= s5;
            _adrI_o <= `RESETVECTOR;
        end else case (state)
            s0: begin state <= halt  ? s0 : s1;                             end
            s1: begin state <= edone ? s2 : s1; _irhs <= rhs;               end
            s2: begin state <= s3; /* compensate for slow memory */         end
            s3: begin state <= s4; _data   <= datD_i;                       end
            s4: begin state <= s5; _adrI_o <= branching ? nextZ : nextP;    end
            s5: begin state <= s6; nextP   <= adrI_o + 1;                   end
            s6: begin state <= s0; insn    <= datI_i; /* why extra ? */     end
        endcase

    Decode decode(.insn, .op, .idxZ, .idxX, .idxY, .valI, .deref_rhs,
                  .kind, .branching, .loading, .storing);

    Shuf shuf(.kind, .valX, .valA, .valY, .valB, .valI, .valC);

    Exec exec(.clk, .en ( state == s0 ), .done ( edone ),
              .op,  .valA, .valB, .valC, .valZ ( rhs   ));

    assign wenD_o = storing;
    assign stbD_o = state == s3 && (loading || storing);
    assign nextZ  = deref_rhs ? _data : _irhs;
    assign datD_o = deref_rhs ? valZ  : _irhs;
    assign adrD_o = deref_rhs ? _irhs : valZ;
    assign selD_o = 4'hf, selI_o = 4'hf;
    assign cycD_o = stbD_o;
    assign stbI_o = state == s4;
    assign cycI_o = stbI_o;
    assign adrI_o = _adrI_o;
    assign wenI_o = 1'b0;
    assign halt   = errD_i | errI_i | rtyD_i | rtyI_i;

    wire upZ = !storing && state == s4;
    Reg regs(.clk, .nextP, .idxX, .idxY, .idxZ,
             .upZ, .nextZ, .valX, .valY, .valZ);

endmodule

