`timescale 1ms/10us

`define CLOCKPERIOD 10

module Top();
    Tenyr tenyr();

    reg [100:0] filename;
    initial #0 begin
        $dumpfile("Top.vcd");
        if ($value$plusargs("LOAD=%s", filename))
            $tenyr_load(filename);
        $dumpvars;
        #(16 * `CLOCKPERIOD) $finish;
    end
endmodule

