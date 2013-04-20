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

    always @(posedge clk)
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

module Exec(input clk, en, output[31:0] rhs, input[3:0] op,
            input signed[31:0] X, Y, A);

    reg[31:0] tmp;
    assign rhs = tmp + A;
    always @(posedge clk) if (en) begin
        case (op)
            4'b0000: tmp <=  (X  |  Y); // X bitwise or Y
            4'b0001: tmp <=  (X  &  Y); // X bitwise and Y
            4'b0010: tmp <=  (X  +  Y); // X add Y
            4'b0011: tmp <=  (X  *  Y); // X multiply Y
            4'b0100: tmp <= 32'bx;      // reserved
            4'b0101: tmp <=  (X  << Y); // X shift left Y
            4'b0110: tmp <= -(X  <  Y); // X compare < Y
            4'b0111: tmp <= -(X  == Y); // X compare == Y
            4'b1000: tmp <= -(X  >  Y); // X compare > Y
            4'b1001: tmp <=  (X  &~ Y); // X bitwise and complement Y
            4'b1010: tmp <=  (X  ^  Y); // X bitwise xor Y
            4'b1011: tmp <=  (X  -  Y); // X subtract Y
            4'b1100: tmp <=  (X  ^~ Y); // X xor ones' complement Y
            4'b1101: tmp <=  (X  >> Y); // X shift right logical Y
            4'b1110: tmp <= -(X  != Y); // X compare <> Y
            4'b1111: tmp <= 32'bx;      // reserved
        endcase
    end

endmodule

module Core(input clk, en, reset_n, inout `HALTTYPE halt,
                                   output reg[31:0] i_addr, input[31:0] i_data,
            output mem_rw, strobe, output    [31:0] d_addr, inout[31:0] d_data);

    localparam CPI = 8; // must be at least 4
`define CYC(n) cyc[(CPI / 4 * (n))]

    wire illegal, type, drhs, jumping, storing, loading;
    wire _en = en && reset_n;
    wire[ 3:0] indexX, indexY, indexZ, op;
    wire[31:0] valueX, valueY, valueZ, valueI, irhs, rhs, storand;

    reg [31:0] r_data; // latches read data
    reg [31:0] next_pc = `RESETVECTOR + 1;
    reg [31:0] insn = `INSN_NOOP;
    reg [CPI-1:0] rcyc = 1, rcycen = 0;
    wire[CPI-1:0] cyc  = rcyc & rcycen;
    reg rhalt = 0;
    assign halt[`HALT_EXEC] = rhalt;

    always @(posedge clk) begin
        if (!reset_n) begin
            i_addr  <= `RESETVECTOR;
            insn    <= `INSN_NOOP;
            next_pc <= `RESETVECTOR + 1;
            rhalt   <= 0;
            rcyc    <= 1;
            rcycen  <= 0; // out of phase with rcyc ; 1-cycle delay on startup
        end else if (_en) begin
            rcyc    <= {rcyc,rcyc[CPI-1]};
            rcycen  <= {rcycen,rcyc[CPI-1] & ~|halt};
            insn    <= i_data;

            if (`CYC(1)) begin
                rhalt   <= rhalt | illegal;
                r_data  <= d_data;
            end
            if (`CYC(2)) begin
                i_addr  <= jumping ? rhs : next_pc;
            end
            if (`CYC(3)) begin
                next_pc <= i_addr + 1;
            end
        end
    end

    // Instruction fetch happens on `CYC(0)

    // Decode and register reads happen as soon as instruction is ready
    Decode decode(.Z ( indexZ ), .insn    ( insn    ), .storing   ( storing ),
                  .X ( indexX ), .type    ( type    ), .deref_rhs ( drhs    ),
                  .Y ( indexY ), .op      ( op      ), .branch    ( jumping ),
                  .I ( valueI ),                       .illegal   ( illegal ));

    // Execution (arithmetic operation) occurs on `CYC(1)
    wire en_ex = _en && `CYC(1);
    wire[31:0] right  = type ? valueI : valueY;
    wire[31:0] addend = type ? valueY : valueI;
    Exec exec(.clk ( clk    ), .en ( en_ex ), .op ( op     ),
              .X   ( valueX ), .Y  ( right ), .A  ( addend ), .rhs ( irhs ));

    // Memory commits on `CYC(2)
    assign loading = drhs && !storing;
    assign strobe  = loading && `CYC(1) || storing && `CYC(2);
    assign mem_rw  = storing && strobe;
    assign rhs     = drhs    ? r_data  : irhs;
    assign storand = drhs    ? valueZ  : irhs;
    assign d_addr  = drhs    ? irhs    : valueZ;
    assign d_data  = storing ? storand : 32'bz;

    // Registers commit after execution, on `CYC(3)
    wire upZ = !storing && `CYC(3);
    Reg regs(.clk     ( clk     ), .indexX ( indexX ), .valueX ( valueX ),
             .en      ( _en     ), .indexY ( indexY ), .valueY ( valueY ),
             .next_pc ( next_pc ), .indexZ ( indexZ ), .valueZ ( valueZ ),
                                   .writeZ ( rhs    ), .upZ    ( upZ    ));

endmodule

