`timescale 1ns/10ps

`define CLOCKPERIOD 10
`define RAMDELAY (1 * `CLOCKPERIOD)
// TODO use proper reset vectors
`define RESETVECTOR 'h0000
//`define SETUPTIME (`CLOCKPERIOD / 2 - 1)
`define SETUPTIME 0
`define SETUP #(`SETUPTIME)
`define DECODETIME `SETUP
`define EXECTIME #0

