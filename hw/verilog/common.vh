`define CLOCKPERIOD 100
`define RAMDELAY (1 * `CLOCKPERIOD)
// TODO use proper reset vectors
`define RESETVECTOR 'h1000

`define HALT_EXTERNAL 0
`define HALT_EIB 1 // index of External Interrupt Block
`define HALT_LAST `HALT_EIB
`define HALTBUSWIDTH `HALT_LAST + 1 // the number of devices supplying halt signals
`define HALTTYPE [`HALTBUSWIDTH-1:0]

`define VIDEO_ADDR 'h10000

`define INSN_NOOP 32'b0

`define TRAP_ADDR       32'hffffffff
`define VECTOR_ADDR     32'hffffffc0
`define ISTACK_BOTTOM   32'hffffffa0
`define TRAMP_BOTTOM    32'hfffff800

// Need a bit on the top end to handle the case where AddrBits == BaseBits
`ifdef __ICARUS__
// Icarus appears to balk at part-select of concatenation
`define IN_RANGE(AddrBits,BaseBits,Base,Addr) \
        (Addr[AddrBits-1:BaseBits+1] == \
         Base[AddrBits-1:BaseBits+1])
`else
// ISE balks at part-select with zero width, whereas Icarus accepts it
`define IN_RANGE(AddrBits,BaseBits,Base,Addr) \
        ({1'b1,Addr}[AddrBits:BaseBits+1] == \
         {1'b1,Base}[AddrBits:BaseBits+1])
`endif

/* vi: set ts=4 sw=4 et syntax=verilog: */

