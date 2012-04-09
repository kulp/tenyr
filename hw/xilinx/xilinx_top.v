`include "common.vh"
`timescale 1ms/10us

// For now, we expose operand_addr and operand_data as ports to keep ISE from
// optimising away the whole design. Once I figure out what I am doing, these
// should be replaced by whatever actual ports we end up with.
module Tenyr(input clk, output [31:0] operand_addr, operand_data);
    reg halt = 1;
    reg _reset = 0;

    wire[31:0] insn_addr; //, operand_addr;
    wire[31:0] insn_data, in_data, out_data; //, operand_data;
    wire operand_rw;

    initial #(2 * `CLOCKPERIOD) halt = `SETUP 0;
    initial #(1 * `CLOCKPERIOD) _reset = `SETUP 1;

    wire _halt = _reset ? halt : 1'bz;
    always @(negedge clk) halt <= _halt;

    assign in_data      =  operand_rw ? operand_data : 32'bx;
    assign operand_data = !operand_rw ?     out_data : 32'bz;

    GenedBlockMem ram (.clka(clk), .wea(operand_rw), .addra(operand_addr[9:0]),
                       .dina(in_data), .douta(out_data), .clkb(clk), .web(1'b0),
                       .addrb(insn_addr[9:0]), .dinb(32'bx), .doutb(insn_data));

    Core core(.clk(clk), ._reset(_reset), .rw(operand_rw),
            .norm_addr(operand_addr), .norm_data(operand_data),
            .insn_addr(insn_addr)   , .insn_data(insn_data), .halt(_halt));
endmodule


