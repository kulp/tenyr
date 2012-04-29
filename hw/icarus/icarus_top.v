`include "common.vh"
`timescale 1ms/10us

module Tenyr(output[7:0] seg, output[3:0] an);
    reg reset_n = 0;
    reg rhalt = 1;
    reg clk = 0;

    // TODO halt ?
    always #(`CLOCKPERIOD / 2) clk = ~clk;

    wire[31:0] insn_addr, operand_addr;
    wire[31:0] insn_data, operand_data;
    wire operand_rw;

    initial #(2 * `CLOCKPERIOD) rhalt = 0;
    initial #(1 * `CLOCKPERIOD) reset_n = 1;

    wire[`HALTBUSWIDTH-1:0] halt;
    assign halt[`HALT_SIM] = rhalt;

    SimMem #(.BASE(`RESETVECTOR))
           ram(.clk(clk), .enable(!halt), .p0rw(operand_rw),
               .p0_addr(operand_addr), .p0_data(operand_data),
               .p1_addr(insn_addr)   , .p1_data(insn_data));

    SimSerial serial(.clk(clk), .reset_n(reset_n), .enable(!halt),
                     .rw(operand_rw), .addr(operand_addr),
                     .data(operand_data));

    Seg7 #(.BASE(12'h100))
             seg7(.clk(clk), .reset_n(reset_n), .enable(1'b1), // XXX use halt ?
                  .rw(operand_rw), .addr(operand_addr),
                  .data(operand_data), .seg(seg), .an(an));

    // TODO clkL
    Core core(.clk(clk), .clkL(clk), .en(1'b1), .reset_n(reset_n), .rw(operand_rw),
            .norm_addr(operand_addr), .norm_data(operand_data),
            .insn_addr(insn_addr)   , .insn_data(insn_data), .halt(halt));
endmodule

