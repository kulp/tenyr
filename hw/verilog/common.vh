`define CLOCKPERIOD 100
// TODO use proper reset vectors
`define RESETVECTOR 32'h1000

`define VIDEO_ADDR 32'h10000

// Check for contained-ness of a BaseBits-wide addr in a AddrBits-wide port addr
`define IN_RANGE(AddrBits,BaseBits,Base,Addr) \
        (Addr[AddrBits-1:BaseBits+1] == \
         Base[AddrBits-1:BaseBits+1])

/* vi: set ts=4 sw=4 et syntax=verilog: */

