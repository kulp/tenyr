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

    always @(`EDGE clk)
        if (en && upZ && !Zis0 && !ZisP)
            store[indexZ] = writeZ;

endmodule

module Decode(input[31:0] insn, output[3:0] Z, X, Y, output[31:0] I,
              output[3:0] op, output type, illegal, storing, deref_rhs, branch);

    assign type      = insn[30 +: 1];
    assign storing   = insn[29 +: 1];
    assign deref_rhs = insn[28 +: 1];
    assign Z         = insn[24 +: 4];
    assign X         = insn[20 +: 4];
    assign Y         = insn[16 +: 4];
    assign op        = insn[12 +: 4];
    wire[11:0] J     = insn[ 0 +:12];
    assign I         = { {20{J[11]}}, J };

    assign illegal   = &insn;
    assign branch    = &Z && !storing;

endmodule

module Exec(input clk, en, output reg[31:0] rhs, input[3:0] op,
            input signed[31:0] X, Y, A);

    always @(`EDGE clk) if (en) begin
        case (op)
            4'b0000: rhs =  (X  |  Y);  // X bitwise or Y
            4'b0001: rhs =  (X  &  Y);  // X bitwise and Y
            4'b0010: rhs =  (X  +  Y);  // X add Y
            4'b0011: rhs =  (X  *  Y);  // X multiply Y
            4'b0100: rhs = 32'bx;       // reserved
            4'b0101: rhs =  (X  << Y);  // X shift left Y
            4'b0110: rhs = -(X  <  Y);  // X compare < Y
            4'b0111: rhs = -(X  == Y);  // X compare == Y
            4'b1000: rhs = -(X  >  Y);  // X compare > Y
            4'b1001: rhs =  (X  &~ Y);  // X bitwise and complement Y
            4'b1010: rhs =  (X  ^  Y);  // X bitwise xor Y
            4'b1011: rhs =  (X  -  Y);  // X subtract Y
            4'b1100: rhs =  (X  ^~ Y);  // X xor ones' complement Y
            4'b1101: rhs =  (X  >> Y);  // X shift right logical Y
            4'b1110: rhs = -(X  != Y);  // X compare <> Y
            4'b1111: rhs = 32'bx;       // reserved
        endcase

        rhs = rhs + A;
    end

endmodule

module Core(input clk, en, reset_n, inout `HALTTYPE halt,
                                   output reg[31:0] i_addr, input[31:0] i_data,
            output mem_rw, strobe, output    [31:0] d_addr, inout[31:0] d_data);

    wire illegal, type, drhs, jumping, storing, loading;
    wire _en = en && reset_n;
    wire[ 3:0] indexX, indexY, indexZ, op;
    wire[31:0] valueX, valueY, valueZ, valueI, irhs;
    wire[31:0] rhs     = drhs ? d_data : irhs;
    wire[31:0] storand = drhs ? valueZ : irhs;

    reg [31:0] next_pc = `RESETVECTOR + 1;
    reg [31:0] insn = `INSN_NOOP;
    reg [3:0] rcyc = 1, rcycen = 0;
    wire[3:0] cyc  = rcyc & rcycen;
    reg rhalt = 0;
    assign halt[`HALT_EXEC] = rhalt;

    always @(`EDGE clk) begin
        if (!reset_n) begin
            i_addr   = `RESETVECTOR;
            next_pc <= `RESETVECTOR + 1;
            rhalt   <= 0;
            rcyc    <= 1;
            rcycen  <= 0; // out of phase with rcyc ; 1-cycle delay on startup
            insn     = `INSN_NOOP;
        end else if (_en) begin
            rcyc   <= {rcyc[2:0],rcyc[3]};
            rcycen <= {rcycen[2:0],rcyc[3] & ~|halt};
            insn    = i_data;

            if (cyc[1])
                rhalt <= rhalt | illegal;
            if (cyc[2] && ~|halt) begin
                i_addr   = jumping ? rhs : next_pc;
                next_pc <= i_addr + 1;
            end
        end
    end

    // Instruction fetch happens on cyc[0]

    // Decode and register reads happen as soon as instruction is ready
    Decode decode(.Z ( indexZ ), .insn    ( insn    ), .storing   ( storing ),
                  .X ( indexX ), .type    ( type    ), .deref_rhs ( drhs    ),
                  .Y ( indexY ), .op      ( op      ), .branch    ( jumping ),
                  .I ( valueI ), .illegal ( illegal ));

    // Execution (arithmetic operation) occurs on cyc[1]
    wire[31:0] right  = type ? valueI : valueY;
    wire[31:0] addend = type ? valueY : valueI;
    Exec exec(.clk ( clk          ), .X ( valueX ), .rhs ( irhs ),
              .en  ( _en & cyc[1] ), .Y ( right  ),
              .op  ( op           ), .A ( addend ));

    // Memory commits on cyc[2]
    assign d_data  = storing ? storand : 32'bz;
    assign d_addr  = drhs    ? irhs    : valueZ;
    assign mem_rw  = storing && cyc[2];
    assign loading = drhs && !storing;
    assign strobe  = (loading || storing) && |cyc[3:1]; // why not cyc[2]

    // Registers commit after execution, on cyc[3]
    wire upZ = !storing && cyc[3];
    Reg regs(.clk     ( clk     ), .indexX ( indexX ), .valueX ( valueX ),
             .en      ( _en     ), .indexY ( indexY ), .valueY ( valueY ),
             .next_pc ( next_pc ), .indexZ ( indexZ ), .valueZ ( valueZ ),
                                   .writeZ ( rhs    ), .upZ    ( upZ    ));

endmodule

