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
    localparam DEPTH_BITS   =  5;               // maximum depth of stacks
    localparam MAX_DEPTH    = 1 << DEPTH_BITS;  // depth of stacks in words

    parameter  VEC_BOTTOM   = `VECTOR_ADDR;
    localparam VEC_SIZE     = 32;               // vector table size in words

    parameter  STACK_TOP    = `ISTACK_TOP;
    parameter  STACK_BITS   = 5;                // interrupt stack size in bits
    localparam STACK_SIZE   = 1 << STACK_BITS;  // interrupt stack size in words
    localparam STACK_WORDS  = (MAX_DEPTH << STACK_BITS) - 1;

    parameter  TRAMP_BOTTOM = 32'hfffff800;
    parameter  TRAMP_BITS   = 8;                // trampoline size in bits
    localparam TRAMP_SIZE   = 1 << TRAMP_BITS;  // trampoline size in words

    // TODO use multidimensional array when tools support them
    reg [31:0] stacks[STACK_WORDS:0];           // HW stack of interrupt stacks
    reg [31:0] tramp[TRAMP_SIZE-1:0];           // single interrupt trampoline
    reg [31:0] vecs[VEC_SIZE-1:0];              // interrupt vector table
    reg [31:0] rdata, i_rdata;                  // output on bus data lines

    reg [DEPTH_BITS-1:0] depth = 0;             // stack pointer
    reg [ IRQ_COUNT-1:0] isr = 0;               // Interrupt Status Register
    reg [ IRQ_COUNT-1:0] imrs[MAX_DEPTH-1:0];   // Interrupt Mask Register stack
    reg [ IRQ_COUNT-1:0] rets[MAX_DEPTH-1:0];   // Return Address stack

    wire d_active = d_addr[31:12] == 20'hfffff && strobe;
    wire i_active = i_addr[31:12] == 20'hfffff;
    assign d_data = (d_active & ~rw) ? rdata : 32'bz;
    assign i_data = i_active ? i_rdata : 32'bz;

    initial begin
        imrs[0] = 0;
        $readmemh("../verilog/trampoline.memh", tramp);
    end

`define IS_STACK(X)     ((STACK_TOP - STACK_SIZE) < (X) && (X) <= STACK_TOP)
`define IS_TRAMP(X)     (TRAMP_BOTTOM <= (X) && (X) < TRAMP_BOTTOM + TRAMP_SIZE)
`define IS_VEC(X)       (  VEC_BOTTOM <= (X) && (X) <   VEC_BOTTOM +   VEC_SIZE)
`define STACK_ADDR(X)   ((depth << STACK_BITS) | (STACK_TOP - (X)))
`define TRAMP_ADDR(X)   ((X) - TRAMP_BOTTOM)
`define VEC_ADDR(X)     ((X) -   VEC_BOTTOM)

    always @(posedge clk) begin
        if (!reset_n) begin
            isr     <= 0;
            depth   <= 0;
            imrs[0] <= 0;
        end else begin
            isr  <= isr | irq;  // accumulate until cleared
            // For now, trap follows irq by one cycle
            trap <= |(imrs[depth] & isr);

                 if (`IS_STACK(i_addr)) i_rdata <= stacks[`STACK_ADDR(i_addr)];
            else if (`IS_TRAMP(i_addr)) i_rdata <= tramp [`TRAMP_ADDR(i_addr)];
            else if (`IS_VEC  (i_addr)) i_rdata <= vecs  [  `VEC_ADDR(i_addr)];
            else i_rdata <= 32'bx;

            if (d_active && rw) begin // writing
                     if (`IS_STACK(d_addr)) stacks[`STACK_ADDR(d_addr)] <= d_data;
                else if (`IS_TRAMP(d_addr)) tramp [`TRAMP_ADDR(d_addr)] <= d_data;
                else if (`IS_VEC  (d_addr)) vecs  [`VEC_ADDR(d_addr)  ] <= d_data;
                else case (d_addr[11:0])
                    12'hfff: begin
                        if (depth == MAX_DEPTH - 1) begin
                            $display("Tried to push too many interrupt frames");
                            $stop;
                        end else
                            depth <= depth + 1;

                        rets[depth + 1] <= d_data;
                        imrs[depth + 1] <= 'b0;     // disable interrupts
                    end
                    12'hffe: isr <= isr & ~d_data;    // ISR clear bits
                    12'hffd: imrs[depth] <= d_data;   // IMR write
                    default: $display("Unhandled write of %x @ %x", d_data, d_addr);
                endcase
            end else if (d_active && !rw) begin   // reading
                     if (`IS_STACK(d_addr)) rdata <= stacks[`STACK_ADDR(d_addr)];
                else if (`IS_TRAMP(d_addr)) rdata <= tramp [`TRAMP_ADDR(d_addr)];
                else if (`IS_VEC  (d_addr)) rdata <= vecs  [`VEC_ADDR(d_addr)  ];
                else case (d_addr[11:0])
                    12'hfff: begin
                        if (depth == 0) begin
                            $display("Tried to pop too many interrupt frames");
                            $stop;
                        end else
                            depth <= depth - 1;

                        rdata <= rets[depth];       // RA  read
                    end
                    12'hffe: rdata <= isr;          // ISR read
                    12'hffd: rdata <= imrs[depth];  // IMR read
                    default: $display("Unhandled read @ %x", d_addr);
                endcase
            end
        end
    end

endmodule

