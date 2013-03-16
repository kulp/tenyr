`include "common.vh"
`timescale 1ns/10ps

module Hex2AsciiDigit(clk, hex, digit);

    input clk;
    input[3:0] hex;
    output reg[6:0] digit;

    always @(`EDGE clk) begin
        case (hex)
            4'h0: digit = 8'd048 /* '0' */;
            4'h1: digit = 8'd049 /* '1' */;
            4'h2: digit = 8'd050 /* '2' */;
            4'h3: digit = 8'd051 /* '3' */;
            4'h4: digit = 8'd052 /* '4' */;
            4'h5: digit = 8'd053 /* '5' */;
            4'h6: digit = 8'd054 /* '6' */;
            4'h7: digit = 8'd055 /* '7' */;
            4'h8: digit = 8'd056 /* '8' */;
            4'h9: digit = 8'd057 /* '9' */;

            4'hA: digit = 8'd065 /* 'A' */;
            4'hB: digit = 8'd098 /* 'b' */;
            4'hC: digit = 8'd067 /* 'C' */;
            4'hD: digit = 8'd100 /* 'd' */;
            4'hE: digit = 8'd069 /* 'E' */;
            4'hF: digit = 8'd070 /* 'F' */;
        endcase
    end

endmodule

