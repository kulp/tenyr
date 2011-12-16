`timescale 1ms/10us

module Test();
    reg clk = 0;
    always #5 clk = !clk;

    Top top();
    reg[23:0] mem_addr;
    reg[31:0] mem_data;
    wire[31:0] _mem_data = (mem_enable && writing) ? mem_data : 'bz;
    reg writing = 0;
    reg reading = 0;

    reg[31:0] val;

    wire mem_enable = reading ^ writing;
    Mem #(.BASE(0), .SIZE(4)) testmem0(clk, mem_enable, writing, mem_addr, _mem_data);
    Mem #(.BASE(4), .SIZE(4)) testmem1(clk, mem_enable, writing, mem_addr, _mem_data);

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

    reg reg_rw = 0;
    reg[3:0] reg_index;
    reg[31:0] reg_data;
    reg[31:0] reg_val;
    wire[31:0] _reg_data = reg_rw ? reg_data : 'bz;
    wire[23:0] pc;
    Reg regs(clk, reg_rw, reg_index, _reg_data, pc);
    initial begin
        #1;

        reg_index = 2;
        reg_data = 3;
        reg_rw = 1;
        #10 reg_rw = 0;

        #10;

        reg_index = 5;
        reg_data = 6;
        reg_rw = 1;
        #10 reg_rw = 0;

        #10;

        reg_index = 2;
        reg_val = _reg_data;

        #10;

        reg_index = 5;
        reg_val = _reg_data;

    end

    initial begin
        $dumpfile("Test.vcd");
        $dumpvars(0,Test);
        $display("hallo, Welt");
        #100 $finish;
    end
endmodule


