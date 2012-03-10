`timescale 1ms/10us

module Test();
    reg clk = 0;
    always #5 clk = !clk;

    Top top();
    reg[31:0] operand_addr, insn_addr;
    reg[31:0] operand_data, insn_data;
    wire[31:0] _operand_data = (mem_enable && writing) ? operand_data : 'bz;
    wire[31:0] _insn_data;
    always @(negedge clk) insn_data <= _insn_data;
    reg writing = 0;
    reg reading = 0;

    reg[31:0] val;

    wire mem_enable = reading ^ writing;
    Mem #(.BASE(0), .SIZE(4))
        testmem0(.clk(clk), .enable(mem_enable), .p0rw(writing),
                 .p0_addr(operand_addr), .p0_data(_operand_data),
                 .p1_addr(pc), .p1_data(_insn_data));
    Mem #(.BASE(4), .SIZE(4))
        testmem1(.clk(clk), .enable(mem_enable), .p0rw(writing),
                 .p0_addr(operand_addr), .p0_data(_operand_data),
                 .p1_addr(pc), .p1_data(_insn_data));
    SimSerial #(.BASE(8)) serial(.clk(clk), .enable(mem_enable), .rw(writing),
                                 .addr(operand_addr), .data(_operand_data));

    always @(negedge clk) begin
        if (reading)
            operand_data <= _operand_data;
    end

    initial #0 begin
        #1;

        operand_addr = 2;
        operand_data = 3;
        writing = 1;
        #10 writing = 0;

        #10;

        operand_addr = 5;
        operand_data = 6;
        writing = 1;
        #10 writing = 0;

        #10;

        operand_addr = 2;
        operand_data = 0;
        reading = 1;
        #10 reading = 0;
        val = operand_data;

        #10;

        operand_addr = 5;
        operand_data = 0;
        reading = 1;
        #10 reading = 0;
        val = operand_data;

        #10;

        operand_addr = 8;
        operand_data = 65;
        writing = 1;
        #10 writing = 0;

    end

    reg reg_rw = 0;
    reg[3:0] reg_indexZ,
             reg_indexX,
             reg_indexY;
    reg[31:0] reg_dataZ,
              reg_dataX,
              reg_dataY;
    reg[31:0] reg_valZ,
              reg_valX,
              reg_valY;
    wire[31:0] _reg_dataZ = reg_rw ? reg_dataZ : 'bz;
    wire[31:0] _reg_dataX;
    wire[31:0] _reg_dataY;
    wire[31:0] pc;

    //Reg regs(clk, reg_rw, reg_index, _reg_data, pc);
    Reg regs(.clk(clk),
            .rwZ(reg_rw), .indexZ(reg_indexZ), .valueZ(_reg_dataZ),
                          .indexX(reg_indexX), .valueX(_reg_dataX),
                          .indexY(reg_indexY), .valueY(_reg_dataY),
            .pc(pc));

    initial #0 begin

        @(posedge clk);

        #00 reg_indexX = 2;
        #00 reg_indexY = 5;

        #01 reg_indexZ = 2;
        #00 reg_dataZ  = 3;
        #00 reg_rw     = 1;
        #05 reg_rw     = 0;

        @(posedge clk) #4;

        #00 reg_indexZ = 5;
        #00 reg_dataZ  = 6;
        #00 reg_rw     = 1;
        #05 reg_rw     = 0;

        @(posedge clk) #4;

        #00 reg_indexZ = 2;
        #05 reg_valZ   = _reg_dataZ;

        @(posedge clk) #4;

        #00 reg_indexZ = 5;
        #05 reg_valZ   = _reg_dataZ;

        @(posedge clk) #4;

        #00 reg_indexZ = 'bz;

        @(posedge clk) #4;

        #00 reg_indexZ = 5;
        #05 reg_valZ   = _reg_dataZ;

        @(posedge clk) #4;

        #00 reg_indexZ = 2;
        #05 reg_valZ   = _reg_dataZ;

    end

    initial #0 begin
        $dumpfile("Test.vcd");
        $dumpvars(0,Test);
        $display("hallo, Welt");
        #100 $finish;
    end
endmodule


