`timescale 1ns/10ps

`define CYC(N) #(N * CLOCKPERIOD)

module Eib_test();

    parameter CLOCKPERIOD = 10;

    reg clk = 0; // XXX EIB does not work when clk starts at 1
    always #(CLOCKPERIOD / 2) clk = ~clk;
    initial begin
        $dumpfile("eib_tb.vcd");
        $dumpvars;
    end

    reg reset   = 1;
    reg strobe  = 0;
    reg writing = 0;
    wire trap;
    reg [31:0] irqs = 0;
    reg [31:0] addr, rdata;
    wire[31:0] data = writing ? rdata : 32'bz;

    Eib eib(
        .clk    ( clk    ), .reset_n ( ~reset  ),
        .trap   ( trap   ), .irq     ( irqs    ),
        .strobe ( strobe ), .rw      ( writing ),
        .d_addr ( addr   ), .i_addr  ( 32'b0   ),
        .d_data ( data   )//, .i_data  ( 32'bz   )
    );

    task trans_write(input [31:0] where, what);
        begin
            `CYC(0) begin
                addr    = where;
                rdata   = what;
                writing = 1;
                strobe  = 1;
            end
            `CYC(1) begin
                strobe  = 0;
                writing = 0;
            end
        end
    endtask

    task trans_read(input [31:0] where, output reg [31:0] what);
        begin
            `CYC(0) begin
                addr    = where;
                writing = 0;
                strobe  = 1;
            end
            `CYC(1) begin
                what    = data;
                strobe  = 0;
                writing = 0;
            end
        end
    endtask

    task put_pc (input [31:0] pc     ); trans_write(32'hffffffff, pc  ); endtask
    task get_pc (output reg [31:0] pc); trans_read (32'hffffffff, pc  ); endtask
    task clr_int(input [31:0] mask   ); trans_write(32'hfffffffe, mask); endtask
    task set_imr(input [31:0] mask   ); trans_write(32'hfffffffd, mask); endtask

    initial `CYC(5) reset = 0;
    reg [31:0] scratch;
    initial begin
        `CYC( 8) set_imr(32'hffffffff);

        `CYC(10) irqs = irqs |  5;
        `CYC( 1) irqs = irqs & ~5;

        `CYC(5) put_pc(32'hdeadbeef);
        `CYC(3) clr_int(1);
        `CYC(5) get_pc(scratch);

        `CYC(5) put_pc(32'hf00dbaaf);
        `CYC(3) clr_int(4);
        `CYC(5) get_pc(scratch);
    end
    initial `CYC(100) $finish;

endmodule

