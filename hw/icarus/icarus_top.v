`include "common.vh"
`timescale 1ns/10ps

module Tenyr(input clk, reset, inout `HALTTYPE halt,
             output[7:0] seg, output[3:0] an);

    wire[31:0] insn_addr, oper_addr, insn_data, out_data;
    wire[31:0] oper_data = !oper_rw ? out_data : 32'bz;
    wire oper_rw, oper_strobe;
    wire reset_n = ~reset;

    reg rhalt = 0;
    assign halt[`HALT_TENYR] = rhalt;

    Core core(
        .clk    ( clk     ), .en     ( 1'b1      ), .reset_n ( reset_n     ),
        .halt   ( halt    ), .i_addr ( insn_addr ), .i_data  ( insn_data   ),
        .mem_rw ( oper_rw ), .d_addr ( oper_addr ), .d_data  ( oper_data   ),
                                                    .strobe  ( oper_strobe )
    );

// -----------------------------------------------------------------------------
// MEMORY ----------------------------------------------------------------------

    // active on posedge clock
    SimMem #(.BASE(`RESETVECTOR)) ram(
        .clka  ( clk         ), .clkb  ( clk       ),
        .ena   ( oper_strobe ), .enb   ( 1'b1      ),
        .addra ( oper_addr   ), .addrb ( insn_addr ),
        .dina  ( oper_data   ), .dinb  ( 32'bx     ),
        .douta ( out_data    ), .doutb ( insn_data ),
        .wea   ( oper_rw     ), .web   ( 1'b0      )
    );

// -----------------------------------------------------------------------------
// DEVICES ---------------------------------------------------------------------

    SimWrap_simserial #(.BASE(12'h20), .SIZE(2)) serial(
        .clk ( clk     ), .reset_n ( reset_n   ), .enable ( oper_strobe ),
        .rw  ( oper_rw ), .addr    ( oper_addr ), .data   ( oper_data   )
    );

    Seg7 #(.BASE(12'h100)) seg7(
        .clk ( clk     ), .reset_n ( reset_n   ), .enable ( oper_strobe ),
        .rw  ( oper_rw ), .addr    ( oper_addr ), .data   ( oper_data   ),
        .seg ( seg     ), .an      ( an        )
    );

endmodule

