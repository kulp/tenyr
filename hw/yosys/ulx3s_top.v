module Top(
    input clk_25mhz,
    input[6:0] btn,
    output[7:0] led, inout[27:0] gp
);

    // Unconditionally ignore halt for now, just to prevent a wired-or from
    // breaking synthesis.
    wire halt = btn[3] ? '0 : '0;
    Tenyr tenyr(.clk(clk_25mhz), .reset(0), .halt(halt), .seg(led));

    // Force the number of digits in the seg7 display to 1, to prevent
    // digits from being overlaid on top of one another.
    defparam tenyr.seg7.DIGITS = 1;

endmodule
