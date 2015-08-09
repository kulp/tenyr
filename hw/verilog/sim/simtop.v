`include "common.vh"
`timescale 1ns/10ps

module Top();

    parameter LOADFILE = "default.memh";

    reg clk = 1;
    reg rhalt = 1;
    reg reset = 1;

    always #(`CLOCKPERIOD / 2) clk <= ~clk;

    wire halt = 0;

    // rhalt and reset timing should be independent of each other, and
    // do indeed appear to be so.
    initial #(1 * 4 * `CLOCKPERIOD) rhalt = 0;
    initial #(1 * 3 * `CLOCKPERIOD) reset = 0;

    Tenyr #(.LOADFILE(LOADFILE)) tenyr(.clk, .reset, .halt);

`ifdef __ICARUS__
    // TODO The `ifdef guard should really be controlling for VPI availability
    reg [800:0] filename;
    reg [800:0] logfile = "Top.vcd";
    integer periods = 64;
    integer clk_count = 0;
    integer insn_count = 0;
    integer temp;
    initial #0 begin
        if ($value$plusargs("LOAD=%s", filename))
            $tenyr_load(filename); // replace with $readmemh ?
        if ($value$plusargs("PERIODS=%d", temp))
            periods = temp;
        if ($value$plusargs("LOGFILE=%s", filename))
            logfile = filename;
        $dumpfile(logfile);
        $dumpvars;
        #(periods * `CLOCKPERIOD) begin:ending
            integer row, col, i;
            if ($test$plusargs("DUMPENDSTATE")) begin
                for (row = 0; row < 3; row = row + 1) begin
                    $write("state: ");
                    for (col = 0; col < 6 && row * 6 + col < 16; col = col + 1) begin
                        i = row * 6 + col;
                        $write("%c %08x ", 65 + i, tenyr.core.regs.store[i]);
                    end
                    $write("\n");
                end
            end
            $finish;
        end
    end

    always #`CLOCKPERIOD begin
        clk_count = clk_count + 1;
        if (tenyr.core.state == tenyr.core.s3) begin
            insn_count = insn_count + 1;
        end
    end

`endif

endmodule

