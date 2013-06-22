`include "common.vh"
`timescale 1ns/10ps

// External Interrupt Block
// contains its own RAMs and presents a simple interrupt handler interface
// to the tenyr Core
module Eib(input clk, reset_n, strobe, rw,
           input[IRQ_COUNT-1:0] irq, output reg trap,
           input[31:0] i_addr, output [31:0] i_data,
           input[31:0] d_addr, inout  [31:0] d_data);

    localparam IRQ_COUNT    = 32;               // total count of interrupts
    localparam DEPTH_BITS   = 2;                // maximum depth of stacks
    localparam MAX_DEPTH    = 1 << DEPTH_BITS;  // depth of stacks in words

    parameter  VECTS_BOTTOM = `VECTOR_ADDR;
    localparam VECTS_BITS   = 5;                // vector table size in bits
    localparam VECTS_SIZE   = 1 << VECTS_BITS;  // vector table size in words
    parameter  VECTORFILE   = "vectors.memh";

    parameter  STACK_TOP    = `ISTACK_TOP;
    parameter  STACK_BITS   = 5;                // interrupt stack size in bits
    localparam STACK_SIZE   = 1 << STACK_BITS;  // interrupt stack size in words
    localparam STACK_WORDS  = (MAX_DEPTH << STACK_BITS) - 1;

    parameter  TRAMP_BOTTOM = `TRAMP_BOTTOM;
    parameter  TRAMP_BITS   = 8;                // trampoline size in bits
    localparam TRAMP_SIZE   = 1 << TRAMP_BITS;  // trampoline size in words

    reg [31:0] d_rdata, i_rdata;                // output on bus data lines

    reg [DEPTH_BITS-1:0] depth = 0;             // stack pointer
    reg [ IRQ_COUNT-1:0] isr = 0;               // Interrupt Status Register

    wire d_inrange = d_addr[31:12] == 20'hfffff;
    wire i_inrange = i_addr[31:12] == 20'hfffff;
    wire d_active  = d_inrange && strobe;
    assign d_data  = (d_active & ~rw) ? d_rdata : 32'bz;
    assign i_data  = i_inrange ? i_rdata : 32'bz;

`define IS_STACK(X)     ((STACK_TOP - STACK_SIZE) < (X) && (X) <= STACK_TOP)
`define IS_TRAMP(X)     (TRAMP_BOTTOM <= (X) && (X) < TRAMP_BOTTOM + TRAMP_SIZE)
`define IS_VECTS(X)     (VECTS_BOTTOM <= (X) && (X) < VECTS_BOTTOM + VECTS_SIZE)
`define STACK_ADDR(X)   ((depth << STACK_BITS) | X[(STACK_BITS - 1):0])
`define TRAMP_ADDR(X)   (X[(TRAMP_BITS - 1):0])
`define VECTS_ADDR(X)   (X[(VECTS_BITS - 1):0])

    wire[31:0] vects_dout_d, vects_dout_i;
    wire[31:0] stack_dout_d, stack_dout_i;
    wire[31:0] tramp_dout_d, tramp_dout_i;
    reg [31:0] cntrl_dout_d, cntrl_dout_i; // XXX cntrl_dout_i is never set

    wire[31:0] vects_ad = `VECTS_ADDR(d_addr), vects_ai = `VECTS_ADDR(i_addr);
    wire[31:0] stack_ad = `STACK_ADDR(d_addr), stack_ai = `STACK_ADDR(i_addr);
    wire[31:0] tramp_ad = `TRAMP_ADDR(d_addr), tramp_ai = `TRAMP_ADDR(i_addr);

    wire d_is_vects = d_active  && `IS_VECTS(d_addr),
         i_is_vects = i_inrange && `IS_VECTS(i_addr),
         d_is_tramp = d_active  && `IS_TRAMP(d_addr),
         i_is_tramp = i_inrange && `IS_TRAMP(i_addr),
         d_is_stack = d_active  && `IS_STACK(d_addr),
         i_is_stack = i_inrange && `IS_STACK(i_addr);
     
    wire d_is_cntrl = !(d_is_vects | d_is_tramp | d_is_stack);
    wire i_is_cntrl = !(i_is_vects | i_is_tramp | i_is_stack);

    initial begin
        isr     <= 0;
    end

    ramwrap #( // TODO vects doesn't need to be accessible to instruction port
        .LOAD(1), .LOADFILE(VECTORFILE), .ABITS(VECTS_BITS), .SIZE(VECTS_SIZE)
    ) vects(
        .clka  ( clk          ), .clkb  ( clk          ),
        .ena   ( d_is_vects   ), .enb   ( i_is_vects   ),
        .wea   ( rw           ), .web   ( 1'b0         ),
        .addra ( vects_ad     ), .addrb ( vects_ai     ),
        .dina  ( d_data       ), .dinb  ( 32'bx        ),
        .douta ( vects_dout_d ), .doutb ( vects_dout_i )
    );

    ramwrap #(
        .LOAD(1), .LOADFILE("../verilog/trampoline.memh"),
        .ABITS(TRAMP_BITS), .SIZE(TRAMP_SIZE)
    ) tramp(
        .clka  ( clk          ), .clkb  ( clk          ),
        .ena   ( d_is_tramp   ), .enb   ( i_is_tramp   ),
        .wea   ( rw           ), .web   ( 1'b0         ),
        .addra ( tramp_ad     ), .addrb ( tramp_ai     ),
        .dina  ( d_data       ), .dinb  ( 32'bx        ),
        .douta ( tramp_dout_d ), .doutb ( tramp_dout_i )
    );

    ramwrap #(.ABITS(STACK_BITS), .SIZE(STACK_SIZE)) stack(
        .clka  ( clk          ), .clkb  ( clk          ),
        .ena   ( d_is_stack   ), .enb   ( i_is_stack   ),
        .wea   ( rw           ), .web   ( 1'b0         ),
        .addra ( stack_ad     ), .addrb ( stack_ai     ),
        .dina  ( d_data       ), .dinb  ( 32'bx        ),
        .douta ( stack_dout_d ), .doutb ( stack_dout_i )
    );

    wire       pushing   = (d_addr[11:0] == 12'hfff) && d_active && rw;
    wire       imrs_rw   = (d_addr[11:0] == 12'hffd) || pushing;
    wire[31:0] imrs_addr = pushing ? depth + 1 : depth;
    wire[31:0] imrs_din  = pushing ? 32'b0     : d_data;
    wire[31:0] imrs_dout;

    BlockRAM #(.ABITS(DEPTH_BITS), .SIZE(MAX_DEPTH), .INIT(1)) imrs(
        .clka  ( clk      ), .addra ( imrs_addr ),
        .ena   ( d_active ), .dina  ( imrs_din  ),
        .wea   ( imrs_rw  ), .douta ( imrs_dout )
    );

    wire       rets_rw   = d_addr[11:0] == 12'hfff;
    wire[31:0] rets_addr = rets_rw ? depth + 1 : depth;
    wire[31:0] rets_din  = d_data;
    wire[31:0] rets_dout;

    BlockRAM #(.ABITS(DEPTH_BITS), .SIZE(MAX_DEPTH), .INIT(1)) rets(
        .clka  ( clk      ), .addra ( rets_addr ),
        .ena   ( d_active ), .dina  ( rets_din  ),
        .wea   ( rets_rw  ), .douta ( rets_dout )
    );

    always @*
        case ({i_is_stack,i_is_tramp,i_is_vects,i_is_cntrl})
            4'b1000: i_rdata = stack_dout_i;
            4'b0100: i_rdata = tramp_dout_i;
            4'b0010: i_rdata = vects_dout_i;
            4'b0001: i_rdata = cntrl_dout_i;
            default: i_rdata = 32'bx;
        endcase

    always @*
        case ({d_is_stack,d_is_tramp,d_is_vects,d_is_cntrl})
            4'b1000: d_rdata = stack_dout_d;
            4'b0100: d_rdata = tramp_dout_d;
            4'b0010: d_rdata = vects_dout_d;
            4'b0001: d_rdata = cntrl_dout_d;
            default: d_rdata = 32'bx;
        endcase

    always @(posedge clk) begin
        if (!reset_n) begin
            isr     <= 0;
            depth   <= 0;
        end else begin
            isr  <= isr | irq;  // accumulate until cleared
            // For now, trap follows irq by one cycle
            trap <= |(imrs_dout & isr);

            if (d_active && rw) begin // writing
                if (d_is_cntrl) case (d_addr[11:0])
                    12'hfff: begin
                        if (depth == MAX_DEPTH - 1) begin
                            $display("Tried to push too many interrupt frames");
                            $stop;
                        end else
                            depth <= depth + 1;

                        // disabling interrupts for depth + 1 handled above
                    end
                    12'hffe: isr <= isr & ~d_data;      // ISR clear bits
                    12'hffd: /* write handled above */; // IMR write
                    default:
                        $display("Unhandled write of %x @ %x", d_data, d_addr);
                endcase
            end else if (d_inrange) begin   // reading // XXX why not d_active
                if (d_is_cntrl) case (d_addr[11:0])
                    12'hfff: begin
                        if (d_active) begin
                            if (depth == 0) begin
                                $display("Tried to pop interrupt when empty");
                                $stop;
                            end else
                                depth <= depth - 1;
                        end

                        cntrl_dout_d <= rets_dout;          // RA  read
                    end
                    12'hffe: cntrl_dout_d <= isr;           // ISR read
                    // XXX is there too much delay in this IMR read ?
                    12'hffd: cntrl_dout_d <= imrs_dout;     // IMR read
                    default:
                        if (d_active) $display("Unhandled read @ %x", d_addr);
                endcase
            end
        end
    end

endmodule

