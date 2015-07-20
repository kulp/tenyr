`define CLOCKPERIOD 100
// TODO use proper reset vectors
`define RESETVECTOR 'h1000

`define VIDEO_ADDR 'h10000

`define TRAP_ADDR       32'hffffffff
`define VECTOR_ADDR     32'hffffffc0
`define ISTACK_BOTTOM   32'hffffffa0
`define TRAMP_BOTTOM    32'hfffff800

// Check for contained-ness of a BaseBits-wide addr in a AddrBits-wide port addr
`define IN_RANGE(AddrBits,BaseBits,Base,Addr) \
        (Addr[AddrBits-1:BaseBits+1] == \
         Base[AddrBits-1:BaseBits+1])

/* vi: set ts=4 sw=4 et syntax=verilog: */

