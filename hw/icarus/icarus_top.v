`include "common.vh"
`timescale 1ms/10us

module Tenyr(output[7:0] seg, output[3:0] an);
    reg halt = 1;
    reg _reset = 0;
    reg clk = 0;

    // TODO halt ?
    always #(`CLOCKPERIOD / 2) clk = ~clk;

    wire[31:0] insn_addr, operand_addr;
    wire[31:0] insn_data, operand_data;
    wire operand_rw;

    initial #(2 * `CLOCKPERIOD) halt = `SETUP 0;
    initial #(1 * `CLOCKPERIOD) _reset = `SETUP 1;

    wire _halt = _reset ? halt : 1'bz;
    always @(negedge clk) halt <= _halt;

    SimMem #(.BASE(`RESETVECTOR))
           ram(.clk(clk), .enable(!halt), .p0rw(operand_rw),
               .p0_addr(operand_addr), .p0_data(operand_data),
               .p1_addr(insn_addr)   , .p1_data(insn_data));

    SimSerial serial(.clk(clk), ._reset(_reset), .enable(!halt),
                     .rw(operand_rw), .addr(operand_addr),
                     .data(operand_data));

    Seg7 #(.BASE(12'h100))
             seg7(.clk(clk), ._reset(_reset), .enable(1'b1), // XXX use halt ?
                  .rw(operand_rw), .addr(operand_addr),
                  .data(operand_data), .seg(seg), .an(an));

    Core core(.clk(clk), ._reset(_reset), .rw(operand_rw),
            .norm_addr(operand_addr), .norm_data(operand_data),
            .insn_addr(insn_addr)   , .insn_data(insn_data), .halt(_halt));
endmodule

