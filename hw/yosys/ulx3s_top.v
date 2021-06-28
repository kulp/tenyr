`default_nettype none

module Top(
    input clk_25mhz,
    input[6:0] btn,
    output[7:0] led, inout[27:0] gp,
    output [3:0] gpdi_dp
);

    wire clk = clk_25mhz;

    reg[3:0] startup = 1;
    wire reset = ~startup[3];
    always @(posedge clk)
        startup <= {startup,1'b1};

    wire [2:0] vgaGreen;
    wire hsync, vsync, inframe;

    Tenyr tenyr(.clk(clk), .reset(reset), .seg(led), .vgaGreen, .hsync, .vsync, .inframe);

    TMDSify gpdi(.clkin(clk), .gpdi_dp, .val(|vgaGreen), .hsync, .vsync, .inframe);

    // Force the number of digits in the seg7 display to 1, to prevent
    // digits from being overlaid on top of one another.
    defparam tenyr.seg7.DIGITS = 1;

endmodule
