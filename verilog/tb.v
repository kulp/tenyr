`timescale 1ms/10us

module test;
    reg clk = 0;
    always #5 clk = !clk;

    top top(clk);
    reg[23:0] mem_addr;
    reg[31:0] mem_data;
    wire[31:0] _mem_data = (mem_enable && writing) ? mem_data : 'bz;
    reg writing = 0;
    reg reading = 0;

    reg[31:0] val;

    wire mem_enable = reading ^ writing;
    mem testmem(clk, mem_enable, writing, mem_addr, _mem_data);

    always @(negedge clk) begin
        if (reading)
            mem_data <= _mem_data;
    end

    initial begin
        #1;

        mem_addr = 2;
        mem_data = 3;
        writing = 1;
        #10 writing = 0;

        #10;

        mem_addr = 5;
        mem_data = 6;
        writing = 1;
        #10 writing = 0;

        #10;

        mem_addr = 2;
        mem_data = 0;
        reading = 1;
        #10 reading = 0;
        val = mem_data;

        #10;

        mem_addr = 5;
        mem_data = 0;
        reading = 1;
        #10 reading = 0;
        val = mem_data;

    end

    initial begin
        $dumpfile("test.vcd");
        $dumpvars(0,test);
        $display("hallo, Welt");
        #100 $finish;
    end
endmodule


