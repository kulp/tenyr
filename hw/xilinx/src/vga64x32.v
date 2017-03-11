module range_counter(
    /*  input */ clk, reset, en,
    /* output */ out, wrap
  );

  parameter integer MIN = 0;
  parameter integer MAX = 255;

  input clk, reset, en;
  output integer out;
  output reg wrap;

  always @(posedge clk)
    if (reset)
      out <= MIN;
    else if (en)
      if (out == MAX) begin
        out <= MIN;
        wrap <= 1;
      end else begin
        out <= out + 1;
        wrap <= 0;
      end

endmodule

module shift_reg(
    /*  input */ clk, en, in,
    /* output */ out
  );

  parameter integer LEN = 16;

  input clk, en, in;
  output out;

  reg [LEN:1] store;

  assign out = store & 1;

  always @(posedge clk)
    if (en)
      store <= {in,store[LEN:2]};

endmodule

module vga64x32(
        /*  input */ reset, clk25MHz, TEXT_D, FONT_D,
        /* output */ R, G, B, hsync, vsync, TEXT_A, FONT_A
    );

  localparam integer ScrnCols = 640;
  localparam integer ScrnRows = 480;
  localparam integer TextCols = 64;
  localparam integer TextRows = 32;
  localparam integer FontCols = 10;
  localparam integer FontRows = 15;

  localparam integer HSynStrt = 656;
  localparam integer HSynStop = 720;
  localparam integer LineCols = 800;

  localparam integer VSynStrt = 483;
  localparam integer VSynStop = 487;
  localparam integer FramRows = 501;

  localparam [3:0]
    sActive = 0, // ^ ^
    sHFront = 1, // | | line
    sHSync  = 2, // | |
    sHBack  = 3, // | v
    sVFront = 4, // |
    sVSync  = 5, // | frame
    sVBack  = 6, // v
    sInit   = 7; // initialization

  input reset, clk25MHz; // TODO use or explicitly ignore reset
  output reg [$clog2(TextCols * TextRows):1] TEXT_A; // 1-indexed
  output reg [$clog2(FontRows):1] FONT_A;
  input      [7:0] TEXT_D; // 8-bit characters
  input      [FontCols:1] FONT_D; // FontCols-wide row of pixels
  output R, G, B, hsync, vsync;
  reg W; // white pixel value

  reg [3:0] state = sInit;

  wire [$clog2(LineCols):1] hctr;
  wire [$clog2(FramRows):1] vctr;
  wire [$clog2(TextCols):1] tcol;
  wire [$clog2(TextRows):1] trow;
  wire [$clog2(FontCols):1] fcol;
  wire [$clog2(FontRows):1] frow;

  reg [FontCols:1] pixels;

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
  wire HBack_done, VBack_done, Height_done, Width_done;

  wire init   = state == sInit;
  wire active = state == sActive;

  // Pixels counters
  range_counter #(.MAX(LineCols - 1)) horz_pixels(
        .clk(clk25MHz), .reset(init), .en(1),
        .out(hctr), .wrap(HBack_done)
      );
  range_counter #(.MAX(FramRows - 1)) vert_pixels(
        .clk(clk25MHz), .reset(init), .en(HBack_done),
        .out(vctr), .wrap(VBack_done)
      );

  // Font counters
  range_counter #(.MAX(FontCols - 1)) font_cols(
        .clk(clk25MHz), .reset(init || HBack_done), .en(active),
        .out(fcol), .wrap(Width_done)
      );
  range_counter #(.MAX(FontRows - 1)) font_rows(
        .clk(clk25MHz), .reset(init || VBack_done), .en(HBack_done),
        .out(frow), .wrap(Height_done)
      );

  // Text counters
  range_counter #(.MAX(TextCols - 1)) text_cols(
        .clk(clk25MHz), .reset(init || HBack_done), .en(Width_done),
        .out(tcol)
      );
  range_counter #(.MAX(TextRows - 1)) text_rows(
        .clk(clk25MHz), .reset(init || VBack_done), .en(Height_done),
        .out(trow)
      );

  // pipeline alignment to make syncs match pixels
  shift_reg #(.LEN(3)) hsync_delay(
        .clk(clk25MHz), .en(1), .in(HFront_done && !HSync_done),
        .out(hsync)
      );
  shift_reg #(.LEN(3)) vsync_delay(
        .clk(clk25MHz), .en(1), .in(VFront_done && !VSync_done),
        .out(vsync)
      );

  always @(posedge clk25MHz) begin
    TEXT_A <= trow * TextCols + tcol;
    FONT_A <= TEXT_D * FontRows + frow;
    pixels <= Width_done ? FONT_D : pixels >> 1;
    W <= pixels & active & 1;

    case (state)
      sActive: state <= Active_done ? sHFront : sActive; // ^ ^
      sHFront: state <= HFront_done ? sHSync  : sHFront; // | | line
      sHSync : state <= HSync_done  ? sHBack  : sHSync;  // | |
      sHBack : state <= Frame_done  ? sVFront : HBack_done ? sActive : sHBack;
      sVFront: state <= VFront_done ? sVSync  : sVFront; // |
      sVSync : state <= VSync_done  ? sVBack  : sVSync;  // | frame
      sVBack : state <= VBack_done  ? sHSync  : sVBack;  // v
      sInit  : begin
        pixels <= 0;
        state <= sActive;
      end
      default: state <= sInit;
    endcase
  end

endmodule

// vi:set ts=2 sw=2 et:
