`include "common.vh"
`timescale 1ns/10ps

module Top();
    Tenyr tenyr();

    reg [100:0] filename;
    integer periods = 64;
    integer temp;
    initial #0 begin
        $dumpfile("Top.vcd");
        if ($value$plusargs("LOAD=%s", filename))
            $tenyr_load(filename);
        if ($value$plusargs("PERIODS=%d", temp))
            periods = temp;
        $dumpvars;
        #(periods * `CLOCKPERIOD) $finish;
    end
endmodule

module Tenyr(output[7:0] seg, output[3:0] an);
    reg reset_n = 0;
    reg rhalt = 1;

    reg  _clk_core = 1;
    reg  en_core   = 0;
    wire clk_core  = en_core & _clk_core;

    // TODO proper data clock timing
    wire clk_datamem = ~clk_core;
    wire clk_insnmem =  clk_core;

    always #(`CLOCKPERIOD / 2) begin
        _clk_core = ~_clk_core;
        en_core   = ~rhalt;
    end

    wire[31:0] insn_addr, oper_addr, insn_data, out_data;
    wire[31:0] oper_data = !oper_rw ? out_data : 32'bz;

    wire oper_rw;

    // rhalt and reset_n timing should be independent of each other, and
    // do indeed appear to be so.
    initial #(4 * `CLOCKPERIOD) rhalt = 0;
    initial #(3 * `CLOCKPERIOD) reset_n = 1;

`ifdef DEMONSTRATE_HALT
    initial #(21 * `CLOCKPERIOD) rhalt = 1;
    initial #(23 * `CLOCKPERIOD) rhalt = 0;
`endif

    wire `HALTTYPE halt;
    assign halt[`HALT_SIM] = rhalt;
    assign halt[`HALT_TENYR] = rhalt;
    assign halt[`HALT_EXTERNAL] = 0;

    Core core(
        .clk    ( clk_core ), .en     ( 1'b1      ), .reset_n ( reset_n   ),
        .halt   ( halt     ), .i_addr ( insn_addr ), .i_data  ( insn_data ),
        .mem_rw ( oper_rw  ), .d_addr ( oper_addr ), .d_data  ( oper_data )
    );

// -----------------------------------------------------------------------------
// MEMORY ----------------------------------------------------------------------

    // active on posedge clock
    SimMem #(.BASE(`RESETVECTOR)) ram(
        .clka  ( ~clk_datamem ), .clkb  ( ~clk_insnmem ),
        .addra ( oper_addr    ), .addrb ( insn_addr    ),
        .dina  ( oper_data    ), .dinb  ( 32'bx        ),
        .douta ( out_data     ), .doutb ( insn_data    ),
        .wea   ( oper_rw      ), .web   ( 1'b0         )
    );

// -----------------------------------------------------------------------------
// DEVICES ---------------------------------------------------------------------

    SimSerial serial(
        .clk ( clk_datamem ), .reset_n ( reset_n   ), .enable ( !halt     ),
        .rw  ( oper_rw     ), .addr    ( oper_addr ), .data   ( oper_data )
    );

    Seg7 #(.BASE(12'h100)) seg7(
        .clk ( clk_datamem ), .reset_n ( reset_n   ), .enable ( 1'b1      ),
        .rw  ( oper_rw     ), .addr    ( oper_addr ), .data   ( oper_data ),
        .seg ( seg         ), .an      ( an        )
    );

endmodule

