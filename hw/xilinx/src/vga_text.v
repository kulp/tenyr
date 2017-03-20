module range_counter(
    /*  input */ clk, reset, en,
    /* output */ out, empty, full
  );

  parameter integer MIN = 0;
  parameter integer MAX = 255;
  parameter integer INC = 1;    // increment (per enabled cycle)

  input clk, reset, en;
  output integer out;
  output reg empty; // true for at most one cycle
  output reg full; // true for at most one cycle

  always @(posedge clk)
    if (reset) begin
      out <= MIN;
      full <= 0;
      empty <= 0;
    end else if (en) begin
      full  <= (out == MAX - INC);
      empty <= (out == MAX);
      out   <= (out == MAX) ? MIN : out + INC;
    end else begin
      // not reset, and not enabled, so leave `out` alone
      full <= 0;
      empty <= 0;
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

module vga_text(
        /*  input */ reset, clk, TEXT_D, FONT_D,
        /* output */ R, G, B, hsync, vsync, TEXT_A, FONT_A
    );

  parameter integer ScrnCols = 640;
  parameter integer ScrnRows = 480;
  parameter integer TextCols = 64;
  parameter integer TextRows = 32;
  parameter integer FontCols = 10;
  parameter integer FontRows = 15;

  parameter integer HSynStrt = 656;
  parameter integer HSynStop = 720;
  parameter integer LineCols = 800;

  parameter integer VSynStrt = 483;
  parameter integer VSynStop = 487;
  parameter integer FramRows = 501;

  localparam integer
    sActive = 0, // ^ ^
    sHFront = 1, // | | line
    sHSync  = 2, // | |
    sHBack  = 3, // | v
    sVFront = 4, // |
    sVSync  = 5, // | frame
    sVBack  = 6, // v
    sInit   = 7; // initialization

  input reset, clk; // TODO use or explicitly ignore reset
  output reg [$clog2(TextCols * TextRows):1] TEXT_A; // 1-indexed
  output reg [$clog2(256 * FontRows):1] FONT_A;
  input      [7:0] TEXT_D; // 8-bit characters
  input      [FontCols:1] FONT_D; // FontCols-wide row of pixels
  output R, G, B, hsync, vsync;
  wire hsync_p, vsync_p; // positive-going versions
  reg W; // white pixel value
  assign hsync = ~hsync_p; // need negative polarity
  assign vsync = ~vsync_p; // need negative polarity

  reg [3:0] state = sInit;

  // TODO change to integers
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

  // All _done signals are true for exactly one cycle of clk
  wire Active_done = hctr == ScrnCols - 1;
  wire Frame_done  = vctr == ScrnRows - 1;
  wire HFront_done = hctr == HSynStrt - 1;
  wire VFront_done = vctr == VSynStrt - 1;
  wire HSync_done  = hctr == HSynStop - 1;
  wire VSync_done  = vctr == VSynStop - 1;
  wire HBack_done, VBack_done, Height_done, Width_done;

  wire init   = state == sInit;
  wire active = state == sActive;

  // Pixels counters
  range_counter #(.MAX(LineCols - 1)) horz_pixels(
        .clk(clk), .reset(init), .en(1),
        .out(hctr), .full(HBack_done)
      );
  range_counter #(.MAX(FramRows - 1)) vert_pixels(
        .clk(clk), .reset(init), .en(HBack_done),
        .out(vctr), .full(VBack_done)
      );

  // Font counters
  range_counter #(.MAX(FontCols - 1)) font_cols(
        .clk(clk), .reset(init || HBack_done), .en(active),
        .out(fcol), .full(Width_done)
      );
  range_counter #(.MAX(FontRows - 1)) font_rows(
        .clk(clk), .reset(init || VBack_done), .en(active && HBack_done),
        .out(frow), .full(Height_done)
      );

  // Text counters
  range_counter #(.MAX(TextCols - 1)) text_cols(
        .clk(clk), .reset(init || HBack_done), .en(active && Width_done),
        .out(tcol)
      );
  range_counter #(.MAX(TextRows - 1)) text_rows(
        .clk(clk), .reset(init || VBack_done), .en(active && Height_done),
        .out(trow)
      );

  // pipeline alignment to make syncs match pixels
  shift_reg #(.LEN(3)) hsync_delay(
        .clk(clk), .en(1), .in(HFront_done && !HSync_done),
        .out(hsync_p)
      );
  shift_reg #(.LEN(3)) vsync_delay(
        .clk(clk), .en(1), .in(VFront_done && !VSync_done),
        .out(vsync_p)
      );

  always @(posedge clk) begin
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
