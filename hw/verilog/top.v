`include "common.vh"
`timescale 1ms/10us

`ifndef SIM
`ifndef __QUARTUS__
  `define VGA
`endif
`endif
`define SEG7
`undef SERIAL
`define INTERRUPTS

module Tenyr(
    input clk, reset, input wor `HALTTYPE halt,
    output[7:0] Led, output[7:0] seg, output[3:0] an,
    output[2:0] vgaRed, vgaGreen, output[2:1] vgaBlue, output hsync, vsync,
    input[31:0] irqs
);

    parameter LOADFILE = "default.memh";
    parameter RAMABITS = 13;

    wire d_rw, d_strobe, trap, eib_halt;
    wire valid_clk, clk_vga, clk_core;
    wire[31:0] i_addr, d_addr, i_data, out_data;
    wire[31:0] d_to_slav;
    wor [31:0] d_to_mast;

    reg[3:0] startup = 0; // delay startup for a few clocks
    wire _reset_n = startup[3] & ~reset;
    always @(posedge clk_core) startup <= {startup,valid_clk};

    assign halt[`HALT_TENYR] = ~startup[3];
    assign halt[`HALT_EIB] = eib_halt;
    assign Led[7:0] = halt;

    tenyr_mainclock clocks(
        .in     ( clk       ), .clk_core0    ( clk_core  ),
        .reset  ( 1'b0      ), .clk_core0_CE ( valid_clk ),
        .locked ( valid_clk ), .clk_vga      ( clk_vga   ),
                               .clk_vga_CE   ( valid_clk )
    );

    Core core(
        .clk    ( clk_core ), .reset_n ( _reset_n ), .mem_rw ( d_rw      ),
        .strobe ( d_strobe ), .i_addr  ( i_addr   ), .d_addr ( d_addr    ),
        .halt   ( halt     ), .i_data  ( i_data   ), .d_in   ( d_to_mast ),
        .trap   ( trap     ),                        .d_out  ( d_to_slav )
    );

`ifdef INTERRUPTS
    Eib eib(
        .clk    ( clk_core ), .reset_n ( _reset_n ), .rw     ( d_rw      ),
        .strobe ( d_strobe ), .i_addr  ( i_addr   ), .d_addr ( d_addr    ),
        .irq    ( irqs     ), .i_data  ( i_data   ), .d_in   ( d_to_slav ),
        .trap   ( trap     ), .halt    ( eib_halt ), .d_out  ( d_to_mast )
    );
`else
    assign trap = 0;
`endif

// -----------------------------------------------------------------------------
// MEMORY ----------------------------------------------------------------------

    ramwrap #(.LOAD(1), .LOADFILE(LOADFILE), .INIT(0),
        .PBITS(32), .ABITS(RAMABITS), .BASE_A(`RESETVECTOR)
    ) ram(
        .clka  ( clk_core  ), .clkb  ( clk_core   ),
        .ena   ( d_strobe  ), .enb   ( startup[2] ),
        .wea   ( d_rw      ), .web   ( 1'b0       ),
        .addra ( d_addr    ), .addrb ( i_addr     ),
        .dina  ( d_to_slav ), .dinb  ( 32'bz      ),
        .douta ( d_to_mast ), .doutb ( i_data     )
    );

// -----------------------------------------------------------------------------
// DEVICES ---------------------------------------------------------------------

`ifdef SERIAL
    // TODO xilinx-compatible serial device ; rename to eliminate `Sim`
    SimWrap_simserial #(.BASE(12'h20), .SIZE(2)) serial(
        .clk ( clk     ), .reset_n ( reset_n   ), .enable ( d_strobe ),
        .rw  ( d_rw ), .addr    ( d_addr ), .data   ( d_data   )
    );
`endif

`ifdef SEG7
    Seg7 #(.BASE(12'h100)) seg7(
        .clk     ( clk_core    ), .rw   ( d_rw   ), .seg ( seg ),
        .reset_n ( _reset_n    ), .addr ( d_addr ), .an  ( an  ),
        .enable  ( d_strobe    ), .data ( d_to_slav )
    );
`endif

`ifdef VGA
    VGAwrap vga(
        .clk_core ( clk_core ), .rw     ( d_rw     ), .vgaRed   ( vgaRed   ),
        .clk_vga  ( clk_vga  ), .addr   ( d_addr   ), .vgaGreen ( vgaGreen ),
        .en       ( 1'b1     ), .data   ( d_data   ), .vgaBlue  ( vgaBlue  ),
        .reset_n  ( _reset_n ), .strobe ( d_strobe ), .hsync    ( hsync    ),
                                                         .vsync    ( vsync    )
    );
`endif

endmodule

