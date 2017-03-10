module vga64x32(
        /*  input */ reset, clk25MHz, TEXT_A, TEXT_D, FONT_A, FONT_D,
        /* output */ R, G, B, hsync, vsync
    );

  localparam [9:0] ScrnCols = 640;
  localparam [8:0] ScrnRows = 480;
  localparam [6:0] TextCols = 64;
  localparam [5:0] TextRows = 32;
  localparam [3:0] FontCols = 10;
  localparam [3:0] FontRows = 15;

  localparam [9:0] HSynStrt = 656;
  localparam [9:0] HSynStop = 720;
  localparam [9:0] LineCols = 800;

  localparam [9:0] VSynStrt = 483;
  localparam [9:0] VSynStop = 487;
  localparam [9:0] FramRows = 501;

  localparam [3:0]
    sActive = 0, // ^ ^
    sHFront = 1, // | | line
    sHSync  = 2, // | |
    sHBack  = 3, // | v
    sVFront = 4, // |
    sVSync  = 5, // | frame
    sVBack  = 6; // v

  input reset, clk25MHz; // TODO use or explicitly ignore reset
  output reg [$clog2(TextCols * TextRows):1] TEXT_A; // 1-indexed
  output reg [$clog2(FontRows):1] FONT_A;
  input      [7:0] TEXT_D; // 8-bit characters
  input      [FontCols:1] FONT_D; // FontCols-wide row of pixels
  output R, G, B, hsync, vsync;

  reg [3:0] state = sActive;

  reg [$clog2(LineCols):1] hctr = 0;
  reg [$clog2(FramRows):1] vctr = 0;
  reg [$clog2(TextCols):1] tcol = 0;
  reg [$clog2(TextRows):1] trow = 0;
  reg [$clog2(FontCols):1] fcol = 0;
  reg [$clog2(FontRows):1] frow = 0;

  reg [FontCols:1] pixels;

  wire y = pixels & 1; // luminance
  wire W = y && state == sActive;
  assign R = W;
  assign G = W;
  assign B = W;

  // All _done signals are true for exactly one cycle of clk25MHz
  wire Active_done = hctr == ScrnCols;
  wire Frame_done  = vctr == ScrnRows;
  wire HFront_done = hctr == HSynStrt;
  wire VFront_done = vctr == VSynStrt;
  wire HSync_done  = hctr == HSynStop;
  wire VSync_done  = vctr == VSynStop;
  wire HBack_done  = hctr == LineCols - 1;
  wire VBack_done  = vctr == FramRows - 1;
  wire Char_done   = fcol == FontCols - 1;

  always @(posedge clk25MHz) begin
    pixels <= Char_done ? FONT_D : pixels >> 1;
    FONT_A <= {TEXT_D,frow};
    hctr <= HBack_done ? 0 : hctr + 1;

    if (HBack_done) begin
      vctr <= VBack_done ? 0 : vctr + 1;
      if (frow == FontRows - 1)
        frow <= 0;
      else
        frow <= frow + 1;

      if (VBack_done || trow == TextRows - 1)
        trow <= 0;
      else
        trow <= trow + 1;
    end

    if (HBack_done || tcol == TextCols - 1)
      tcol <= 0;
    else
      tcol <= tcol + 1;

    if (fcol == FontCols - 1)
      fcol <= 0;
    else
      fcol <= fcol + 1;

    case (state)
      sActive: state <= Active_done ? sHFront : sActive; // ^ ^
      sHFront: state <= HFront_done ? sHSync  : sHFront; // | | line
      sHSync : state <= HSync_done  ? sHBack  : sHSync;  // | |
      sHBack : state <= Frame_done  ? sVFront : HBack_done ? sActive : sHBack;
      sVFront: state <= VFront_done ? sVSync  : sVFront; // |
      sVSync : state <= VSync_done  ? sVBack  : sVSync;  // | frame
      sVBack : state <= VBack_done  ? sHSync  : sVBack;  // v
      default: begin // recovery
        state <= sActive;
        hctr <= 0;
        vctr <= 0;
        tcol <= 0;
        trow <= 0;
        fcol <= 0;
        frow <= 0;
      end
    endcase
  end

endmodule

// vi:set ts=2 sw=2 et:
