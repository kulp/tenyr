// External Interrupt Block
// contains its own RAMs and presents a simple interrupt handler interface
// to the tenyr Core
module Eib(input clk, reset_n, strobe, rw,
           input[IRQ_COUNT-1:0] irq, output reg trap,
           input[31:0] addr, inout[31:0] data);

    parameter STACK_BITS = 5;                   // interrupt stack size in bits
    localparam STACK_SIZE = 1 << STACK_BITS;    // interrupt stack size in words
    localparam STACK_TOP = 32'hffffffdf;
    localparam IRQ_COUNT = 32;                  // total count of interrupts
    localparam MAX_DEPTH = 32;                  // maximum depth of stacks
    localparam STACK_ENTRIES = (MAX_DEPTH << STACK_BITS) - 1;

    // stack position 0 is never popped to
    // this permits us in the future to refer to the next-higher frame reliably
    reg [ 4:0] depth = 1;                       // stack pointer
    // TODO use multidimensional array when tools support them
    reg [31:0] stacks[STACK_ENTRIES:0];         // HW stack of interrupt stacks
    reg [31:0] rdata;                           // output on bus data lines

    reg [IRQ_COUNT-1:0] isr = 0;                // Interrupt Status Register
    reg [IRQ_COUNT-1:0] imrs[MAX_DEPTH-1:0];    // Interrupt Mask Register stack
    reg [IRQ_COUNT-1:0] rets[MAX_DEPTH-1:0];    // Return Address stack

    wire bus_active = strobe && addr[31:12] == 20'hfffff;
    assign data = (bus_active & ~rw) ? rdata : 32'bz;

    initial begin
        imrs[0] = 32'hffffffff;
    end

`define STACK_ADDR ((depth << STACK_BITS) | addr[STACK_BITS-1:0])

    always @(posedge clk) begin
        if (!reset_n) begin
            isr     <= 0;
            depth   <= 1;
            imrs[0] <= 32'hffffffff;
        end else begin
            isr  <= isr | irq;  // accumulate until cleared
            // For now, trap follows irq by one cycle
            trap <= |(imrs[depth] & isr);

            if (bus_active && rw) begin // writing
                casez (addr[11:0])
                    12'hfff: begin
                        depth           <= depth + 1;
                        rets[depth + 1] <= data;
                        imrs[depth + 1] <= 'b0;     // disable interrupts
                    end
                    12'hffe: isr <= isr & ~data;    // ISR clear bits
                    12'hffd: imrs[depth] <= data;   // IMR write
                    default:
                        if (STACK_TOP - addr < STACK_SIZE)
                            stacks[`STACK_ADDR] <= data;
                endcase
            end else if (bus_active && !rw) begin   // reading
                casex (addr[11:0])
                    12'hfff: begin
                        if (depth != 1)
                            depth <= depth - 1;
                        rdata <= rets[depth];       // RA  read
                    end
                    12'hffe: rdata <= isr;          // ISR read
                    12'hffd: rdata <= imrs[depth];  // IMR read
                    default:
                        if (STACK_TOP - addr < STACK_SIZE)
                            rdata <= stacks[`STACK_ADDR];
                endcase
            end
        end
    end

endmodule

