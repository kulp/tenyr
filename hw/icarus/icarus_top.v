`include "common.vh"
`timescale 1ns/10ps

module Top();
    Tenyr tenyr();

    reg [100:0] filename;
    initial #0 begin
        $dumpfile("Top.vcd");
        if ($value$plusargs("LOAD=%s", filename))
            $tenyr_load(filename);
        $dumpvars;
        #(32 * `CLOCKPERIOD) $finish;
    end
endmodule

module Tenyr(output[7:0] seg, output[3:0] an);
    reg reset_n = 0;
    reg rhalt = 1;

    reg clk_core0   = 1,
        clk_core90  = 0,
        clk_core180 = 0,
        clk_core270 = 0;
    wire clk_datamem = clk_core180;
    wire clk_insnmem = clk_core0;

    always #(`CLOCKPERIOD / 2) begin
        {clk_core270,clk_core180,clk_core90,clk_core0} = {clk_core180,clk_core90,clk_core0,clk_core270};
    end

    wire[31:0] insn_addr, operand_addr, insn_data, out_data;
    wire[31:0] in_data      =  operand_rw ? operand_data : 32'bx;
    wire[31:0] operand_data = !operand_rw ?     out_data : 32'bz;

    wire operand_rw;

    // TODO currently can't come out of reset before coming out of halt
    initial #(2 * `CLOCKPERIOD) rhalt = 0;
    initial #(3 * `CLOCKPERIOD) reset_n = 1;

    wire[`HALTBUSWIDTH-1:0] halt;
    assign halt[`HALT_SIM] = rhalt;
    assign halt[`HALT_TENYR] = rhalt;

    // active on posedge clock
    SimMem #(.BASE(`RESETVECTOR))
        ram(.clka(~clk_datamem), .wea(operand_rw), .addra(operand_addr),
            .dina(in_data), .douta(out_data),
            .clkb(~clk_insnmem), .web(1'b0), .addrb(insn_addr),
            .dinb(32'bx), .doutb(insn_data));

    SimSerial serial(.clk(clk_datamem), .reset_n(reset_n), .enable(!halt),
                     .rw(operand_rw), .addr(operand_addr),
                     .data(operand_data));

    Seg7 #(.BASE(12'h100))
             seg7(.clk(clk_datamem), .reset_n(reset_n), .enable(1'b1), // XXX use halt ?
                  .rw(operand_rw), .addr(operand_addr),
                  .data(operand_data), .seg(seg), .an(an));

    // TODO clkL
    Core core(.clk0(clk_core0), .clk90(clk_core90), .clk180(clk_core180), .clk270(clk_core270),
              .en(1'b1), .reset_n(reset_n), .rw(operand_rw),
              .norm_addr(operand_addr), .norm_data(operand_data),
              .insn_addr(insn_addr)   , .insn_data(insn_data), .halt(halt));
endmodule

