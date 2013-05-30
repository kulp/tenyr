// External Interrupt Block
// contains its own RAMs and presents a simple interrupt handler interface
// to the tenyr Core
module Eib(input clk, reset_n, rw,
           input[IRQ_COUNT-1:0] irq, output trap,
           input[31:0] addr, inout[31:0] data);

    parameter STACK_SIZE = 32;
    localparam IRQ_COUNT = 32;
    localparam MAX_DEPTH = 32;
    localparam[2:0] s0 = 0; // state definitions

    reg [ 2:0] state = s0;
    reg [ 4:0] depth = 0;
    reg [31:0] stacks[MAX_DEPTH][STACK_SIZE];
    reg [IRQ_COUNT-1:0] masks[MAX_DEPTH];
    reg [IRQ_COUNT-1:0] links[MAX_DEPTH];
    reg [IRQ_COUNT-1:0] irq_r = 0;

    wire[IRQ_COUNT-1:0] link = links[depth];
    wire[IRQ_COUNT-1:0] mask = masks[depth];

    assign trap = &(mask & irq_r); // TODO clarify update semantics

    initial begin
        masks[0] = 32'hffffffff;
        //links[0] = 0;
    end

    always @(posedge clk) begin
        if (!reset_n) begin
            depth <= 0;
            masks[0] <= 32'hffffffff;
            irq_r <= 0;
        end else begin
            irq_r <= irq;
            case (state)

            endcase
        end
    end

endmodule
