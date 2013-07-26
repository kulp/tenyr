`define CLOCKPERIOD 100
`define RAMDELAY (1 * `CLOCKPERIOD)
// TODO use proper reset vectors
`define RESETVECTOR 'h1000

`define HALT_EXTERNAL 0
`define HALT_TENYR 1 // index of Tenyr module halt line
`define HALT_EXEC 2 // index of exec module halt line
`define HALT_LAST `HALT_EXEC
`define HALTBUSWIDTH `HALT_LAST + 1 // the number of devices supplying halt signals
`define HALTTYPE [`HALTBUSWIDTH-1:0]

`define VIDEO_ADDR 'h10000

`define INSN_NOOP 32'b0

`define TRAP_ADDR       32'hffffffff
`define VECTOR_ADDR     32'hffffffc0
`define ISTACK_BOTTOM   32'hffffffa0
`define TRAMP_BOTTOM    32'hfffff800

`define IN_RANGE(AddrBits,BaseBits,Base,Addr) \
        (Addr[AddrBits-1:BaseBits] == Base[AddrBits-1:BaseBits])

/* vi: set ts=4 sw=4 et syntax=verilog: */

