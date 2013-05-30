// External Interrupt Block
// contains its own RAMs and presents a simple interrupt handler interface
// to the tenyr Core
module Eib(input clk, reset_n, strobe, rw,
           input[IRQ_COUNT-1:0] irq, output trap,
           input[31:0] addr, inout[31:0] data);

    parameter STACK_SIZE = 32;                  // interrupt stack size in words

    localparam IRQ_COUNT = 32;                  // total count of interrupts
    localparam MAX_DEPTH = 32;                  // maximum depth of stacks
    localparam[2:0] s0 = 0;                     // state definitions

    reg [ 2:0] state = s0;                      // state variable
    reg [ 4:0] depth = 0;                       // stack pointer
    reg [31:0] stacks[MAX_DEPTH][STACK_SIZE];   // HW stack of interrupt stacks
    reg [31:0] rdata;                           // output on bus data lines

    reg [IRQ_COUNT-1:0] isr = 0;                // Interrupt Status Register
    reg [IRQ_COUNT-1:0] imrs[MAX_DEPTH];        // Interrupt Mask Register stack
    reg [IRQ_COUNT-1:0] rets[MAX_DEPTH];        // Return Address stack

    wire[IRQ_COUNT-1:0] imr = imrs[depth];      // IMR top-of-stack
    wire[IRQ_COUNT-1:0] ret = rets[depth];      // RA  top-of-stack

    assign trap = &(imr & isr);                 // TODO clarify update semantics
    assign data = (strobe & ~rw) ? rdata : 32'bz;
    wire bus_active = strobe && addr[31:16] == 16'hffff;

    initial begin
        imrs[0] = 32'hffffffff;
    end

    always @(posedge clk) begin
        if (!reset_n) begin
            isr     <= 0;
            depth   <= 0;
            imrs[0] <= 32'hffffffff;
        end else begin
            isr <= irq;
            if (bus_active && rw) begin // writing
                case (addr[15:0])
                    16'hffff: begin depth <= depth+1; rets[depth+1] <= data; end
                endcase
            end else if (bus_active && !rw) begin // reading
                case (addr[15:0])
                    16'hffff: rdata <= ret;
                endcase
            end

            case (state)
                s0: state <= s0;
            endcase
        end
    end

endmodule
