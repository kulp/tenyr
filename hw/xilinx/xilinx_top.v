`include "common.vh"
`timescale 1ms/10us

// For now, we expose operand_addr and operand_data as ports to keep ISE from
// optimising away the whole design. Once I figure out what I am doing, these
// should be replaced by whatever actual ports we end up with.
module Tenyr(output [31:0] operand_addr, operand_data);
    reg halt = 1;
    reg _reset = 0;
    reg clk = 0;

    // TODO halt ?
    always #(`CLOCKPERIOD / 2) clk = ~clk;

    wire[31:0] insn_addr, operand_addr;
    wire[31:0] insn_data, operand_data, in_data, out_data;
    wire operand_rw;

    initial #(2 * `CLOCKPERIOD) halt = `SETUP 0;
    initial #(1 * `CLOCKPERIOD) _reset = `SETUP 1;

    wire _halt = _reset ? halt : 1'bz;
    always @(negedge clk) halt <= _halt;

    assign in_data  =  operand_rw ? operand_data : 32'bx;
    assign out_data = !operand_rw ? operand_data : 32'bx;

    GenedBlockMem ram (
      .clka(clk), // input clka
      .wea(operand_rw), // input [0 : 0] wea
      .addra(operand_addr), // input [9 : 0] addra
      .dina(in_data), // input [31 : 0] dina
      .douta(out_data), // output [31 : 0] douta
      .clkb(clk), // input clkb
      .web(1'b0), // input [0 : 0] web
      .addrb(insn_addr), // input [9 : 0] addrb
    //.dinb(insn_data), // input [31 : 0] dinb
      .doutb(insn_data) // output [31 : 0] doutb
    );

    Core core(.clk(clk), ._reset(_reset), .rw(operand_rw),
            .norm_addr(operand_addr), .norm_data(operand_data),
            .insn_addr(insn_addr)   , .insn_data(insn_data), .halt(_halt));
endmodule


