`default_nettype none

module TMDSify(
   input clkin,
   input val, hsync, vsync, inframe,
   output [3:0] gpdi_dp
);

    wire pixclk;
    wire tmds_clk;

    reg pix;

    tmds_pll pll(.clkin, .clkout0(tmds_clk), .clkout1(pixclk));

    always @(posedge pixclk)
        pix <= val;

    wire [9:0] TMDS_grn, TMDS_blu;
    TMDS_mono_enc encode_grn(.clk(pixclk), .pix, .cd(2'b00)        , .inframe, .tmds(TMDS_grn));
    TMDS_mono_enc encode_blu(.clk(pixclk), .pix, .cd({vsync,hsync}), .inframe, .tmds(TMDS_blu));

    reg [9:0] every_ten = 1;
    wire should_load = every_ten[9];
    always @(posedge tmds_clk)
        every_ten <= {every_ten[8:0],every_ten[9]};

    reg [9:0] TMDS_shift_grn = 0, TMDS_shift_blu = 0;
    always @(posedge tmds_clk) begin
        TMDS_shift_grn <= should_load ? TMDS_grn : TMDS_shift_grn[9:1];
        TMDS_shift_blu <= should_load ? TMDS_blu : TMDS_shift_blu[9:1];
    end

    assign gpdi_dp[0] = TMDS_shift_blu[0];
    assign gpdi_dp[1] = TMDS_shift_grn[0];
    assign gpdi_dp[2] = TMDS_shift_grn[0];
    assign gpdi_dp[3] = pixclk;

endmodule

module TMDS_mono_enc(
        input wire clk,
        input wire pix,
        input [1:0] cd,
        input inframe,
        output reg [9:0] tmds
);
    wire [9:0] word = 10'b1100000111;
    wire [9:0] data = pix ? ~word : word;
    wire [9:0] base = cd[1] ? 10'b0101010100 : 10'b1101010100;
    wire [9:0] ctrl = cd[0] ? ~base : base;

    always @(posedge clk)
        tmds <= inframe ? data : ctrl;

endmodule

