`include "common.vh"
`timescale 1ns/10ps

module JCounter(input clk, input ce, output tick);

    parameter STAGES = 6;
    parameter SHIFTS = 10;

    assign tick = stage[STAGES - 1].s[SHIFTS - 1];

    generate
        genvar i;
        for (i = 0; i < STAGES; i = i + 1) begin:stage
            reg[SHIFTS-1:0] s = 1;

            if (i == 0) begin:zero
                always @(posedge clk)
                    if (ce)
                        stage[i].s <= { stage[i].s, stage[i].s[SHIFTS - 1] };
            end else begin:nonzero
                always @(posedge clk)
                    if (stage[i - 1].s[SHIFTS - 1] | stage[i].s[SHIFTS - 1])
                        if (ce)
                            stage[i].s <= { stage[i].s, stage[i].s[SHIFTS - 1] };
            end
        end
    endgenerate

endmodule


