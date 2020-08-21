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
        default: {idxY,op,valI[11:0],valI[31:12]} = {     tI,{20{tI[11]}}};
        2'b11:   {idxY,op,valI[19:0],valI[31:20]} = {8'h0,tI,{12{tI[19]}}};
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

    function Bit(input[31:0] rA, rB);
        Bit = rA >> rB;
    endfunction

    always @(posedge clk) begin
        { rop , valZ , rA   , rB   , rC   , done , add , act    , staged } <=
        { op  , rZ   , valA , valB , valC , add  , act , staged , en     } ;
        if (staged) case (rop)
            4'h0: rY <=    (rA  |  rB) ; 4'h8: rY <=    (rA  |~ rB  );
            4'h1: rY <=    (rA  &  rB) ; 4'h9: rY <=    (rA  &~ rB  );
            4'h2: rY <=    (rA  ^  rB) ; 4'ha: rY <=  {rA[19:0],rB[11:0]};
            4'h3: rY <=    (rA >>> rB) ; 4'hb: rY <=    (rA  >> rB  );
            4'h4: rY <=    (rA  +  rB) ; 4'hc: rY <=    (rA  -  rB  );
            4'h5: rY <=    (rA  *  rB) ; 4'hd: rY <=    (rA  << rB  );
            4'h6: rY <= {32{rA ==  rB}}; 4'he: rY <= {32{Bit(rA, rB)}};
            4'h7: rY <= {32{rA  <  rB}}; 4'hf: rY <= {32{rA  >= rB  }};
        endcase
        if (act) rZ <= rY + rC;
    end

endmodule

module Core(
    input clk, reset,
    output reg[31:0] adr_o, // address
    input     [31:0] dat_i, // data in
    output    [31:0] dat_o, // data out
    output           wen_o, // write enable
    output    [ 3:0] sel_o, // select
    output           stb_o, // strobe
    input            ack_i, // acknowledge
    input            err_i, // error
    input            rty_i, // retry
    output           cyc_o, // cycle
    inout halt
);

    localparam[3:0] s0=0, s1=1, s2=2, s3=3, s4=4, s5=5, s6=6, s7=7;

    wire deref_rhs, branching, storing, loading, edone;
    wire[3:0] idxX, idxY, idxZ, op;
    wire signed[31:0] valX, valY, valZ, valI, valA, valB, valC, rhs;
    wire[1:0] kind;
    reg[31:0] insn, _data;
    reg signed[31:0] _irhs, nextP;
    reg[3:0] state;

    wire signed[31:0] nextI = branching ? nextZ : nextP;
    wire signed[31:0] nextZ = deref_rhs ? _data : _irhs;
    wire       [31:0] _adrD = deref_rhs ? _irhs : valZ;

    wire   memory = loading | storing;
    wire   i_strb = state == s5;
    wire   d_strb = state == s3 &&  memory;
    wire   upZ    = state == s4 && !storing;
    assign sel_o  = 4'hf;
    assign stb_o  = d_strb | i_strb;
    assign wen_o  = d_strb & storing;
    assign dat_o  = deref_rhs ? valZ : _irhs;
    assign cyc_o  = stb_o;
    assign halt   = err_i | rty_i;

    always @(posedge clk)
        if (reset) begin
            state <= s5;
            adr_o <= `RESETVECTOR;
        end else case (state)
            s0: begin state <= !halt  ? s1 : s0;                        end
            s1: begin state <= edone  ? s2 : s1; _irhs <= rhs;          end
            s2: begin state <= memory ? s3 : s4; adr_o <= _adrD;        end
            s3: begin state <= ack_i  ? s4 : s3; _data <= dat_i;        end
            s4: begin state <=          s5     ; adr_o <= nextI;        end
            s5: begin state <= ack_i  ? s6 : s5; nextP <= adr_o + 1;    end
            s6: begin state <=          s0     ; insn  <= dat_i;        end
            s7: begin state <= s5;                                      end
        endcase

    Decode decode(.insn, .op, .idxZ, .idxX, .idxY, .valI, .deref_rhs,
                  .kind, .branching, .loading, .storing);

    Shuf shuf(.kind, .valX, .valA, .valY, .valB, .valI, .valC);

    Exec exec(.clk, .en ( state == s0 ), .done ( edone ),
              .op,  .valA, .valB, .valC, .valZ ( rhs   ));

    Reg regs(.clk, .nextP, .idxX, .idxY, .idxZ,
             .upZ, .nextZ, .valX, .valY, .valZ);

endmodule

