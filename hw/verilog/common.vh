`timescale 1ms/10us

`define CLOCKPERIOD 10
`define RAMDELAY (1 * `CLOCKPERIOD)
// TODO use proper reset vectors
`define RESETVECTOR 'h1000
`define SETUPTIME 2
`define SETUP #(`SETUPTIME)
`define DECODETIME `SETUP
`define EXECTIME `SETUP


