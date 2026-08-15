`timescale 1ns/10ps

module SimWrap_simserial(
    input clk, input enable, input rw, input reset,
    input[31:0] addr, inout[31:0] data
);
    parameter BASE = 0;
    parameter SIZE = 0;

    wire in_range = (addr >= BASE && addr < SIZE + BASE);

    always @(posedge clk) begin
        if (enable && in_range) begin
            if (rw)
                // Use Verilator to insert a direct C call
                $c("putchar(",data,");");
        end
    end
endmodule

