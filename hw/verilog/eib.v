`timescale 1ns/10ps

// External Interrupt Block
// contains its own RAMs and presents a simple interrupt handler interface
// to the tenyr Core
module Eib(input clk, reset_n, strobe, rw,
           input[IRQ_COUNT-1:0] irq, output reg trap,
           input[31:0] addr, inout[31:0] data);

    localparam IRQ_COUNT     = 32;              // total count of interrupts
    localparam MAX_DEPTH     = 32;              // maximum depth of stacks

    parameter  STACK_TOP     = 32'hffffffdf;
    parameter  STACK_BITS    = 5;               // interrupt stack size in bits
    localparam STACK_SIZE    = 1 << STACK_BITS; // interrupt stack size in words
    localparam STACK_ENTRIES = (MAX_DEPTH << STACK_BITS) - 1;

    parameter  TRAMP_BOTTOM  = 32'hfffff800;
    parameter  TRAMP_BITS    = 8;               // trampoline size in bits
    localparam TRAMP_SIZE    = 1 << TRAMP_BITS; // trampoline size in words

    reg [ 4:0] depth = 0;                       // stack pointer
    // TODO use multidimensional array when tools support them
    reg [31:0] stacks[STACK_ENTRIES:0];         // HW stack of interrupt stacks
    reg [31:0] tramp[TRAMP_SIZE-1:0];           // single interrupt trampoline
    reg [31:0] rdata;                           // output on bus data lines

    reg [IRQ_COUNT-1:0] isr = 0;                // Interrupt Status Register
    reg [IRQ_COUNT-1:0] imrs[MAX_DEPTH-1:0];    // Interrupt Mask Register stack
    reg [IRQ_COUNT-1:0] rets[MAX_DEPTH-1:0];    // Return Address stack

    wire bus_active = strobe && addr[31:12] == 20'hfffff;
    assign data = (bus_active & ~rw) ? rdata : 32'bz;

    initial imrs[0] = 0;

`define IS_STACK(X) ((STACK_TOP - STACK_SIZE) < (X) && (X) <= STACK_TOP)
`define IS_TRAMP(X) (TRAMP_BOTTOM <= (X) && (X) < TRAMP_BOTTOM + TRAMP_SIZE)
`define STACK_ADDR  ((depth << STACK_BITS) | (STACK_TOP - addr))
`define TRAMP_ADDR  (addr[TRAMP_BITS-1:0] - TRAMP_BOTTOM)

    always @(posedge clk) begin
        if (!reset_n) begin
            isr     <= 0;
            depth   <= 0;
            imrs[0] <= 0;
        end else begin
            isr  <= isr | irq;  // accumulate until cleared
            // For now, trap follows irq by one cycle
            trap <= |(imrs[depth] & isr);

            if (bus_active && rw) begin // writing
                case (addr[11:0])
                    12'hfff: begin
                        if (depth == MAX_DEPTH - 1) begin
                            $display("Tried to push too many interrupt frames");
                            $stop;
                        end else
                            depth <= depth + 1;

                        rets[depth + 1] <= data;
                        imrs[depth + 1] <= 'b0;     // disable interrupts
                    end
                    12'hffe: isr <= isr & ~data;    // ISR clear bits
                    12'hffd: imrs[depth] <= data;   // IMR write
                    default:
                        if (`IS_STACK(addr))
                            stacks[`STACK_ADDR] <= data;
                        else if (`IS_TRAMP(addr))
                            tramp[`TRAMP_ADDR] <= data;
                endcase
            end else if (bus_active && !rw) begin   // reading
                case (addr[11:0])
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
                    default:
                        if (`IS_STACK(addr))
                            rdata <= stacks[`STACK_ADDR];
                        else if (`IS_TRAMP(addr))
                            rdata <= tramp[`TRAMP_ADDR];
                endcase
            end
        end
    end

endmodule

