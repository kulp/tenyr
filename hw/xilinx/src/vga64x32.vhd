-- Hi Emacs, this is -*- mode: vhdl; -*-
----------------------------------------------------------------------------------------------------
--
-- Monocrome Text Mode Video Controller VHDL Macro
-- 64x32 characters. Pixel resolution is 640x480/60Hz
--
-- Copyright (c) 2007 Javier Valcarce García, javier.valcarce@gmail.com
-- Copyright (c) 2017 Darren Kulp, darren@kulp.ch
-- $Id$
--
----------------------------------------------------------------------------------------------------
-- This program is free software: you can redistribute it and/or modify
-- it under the terms of the GNU Lesser General Public License as published by
-- the Free Software Foundation, either version 3 of the License, or
-- (at your option) any later version.

-- This program is distributed in the hope that it will be useful,
-- but WITHOUT ANY WARRANTY; without even the implied warranty of
-- MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
-- GNU Lesser General Public License for more details.
--
-- You should have received a copy of the GNU Lesser General Public License
-- along with this program.  If not, see <http://www.gnu.org/licenses/>.
----------------------------------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;


entity vga64x32 is
  port (
    reset       : in  std_logic;
    clk25MHz    : in  std_logic;
    TEXT_A      : out std_logic_vector(10 downto 0); -- text buffer
    TEXT_D      : in  std_logic_vector(07 downto 0);
    FONT_A      : out std_logic_vector(11 downto 0); -- font buffer
    FONT_D      : in  std_logic_vector(09 downto 0);
    --
    octl        : in  std_logic_vector(07 downto 0);
    --
    R           : out std_logic;
    G           : out std_logic;
    B           : out std_logic;
    hsync       : out std_logic;
    vsync       : out std_logic
    );
end vga64x32;



architecture rtl of vga64x32 is

  signal R_int : std_logic;
  signal G_int : std_logic;
  signal B_int : std_logic;
  signal hsync_int : std_logic;
  signal vsync_int : std_logic;

  signal blank : std_logic;
  signal hctr  : integer range 793 downto 0;
  signal vctr  : integer range 524 downto 0;
  -- character/pixel position on the screen
  signal scry  : integer range 031 downto 0;  -- chr row   < 32 (5 bits)
  signal scrx  : integer range 063 downto 0;  -- chr col   < 64 (6 bits)
  signal chry  : integer range 014 downto 0;  -- chr high  < 15 (4 bits)
  signal chrx  : integer range 009 downto 0;  -- chr width < 10 (4 bits)

  signal losr_ce : std_logic;
  signal losr_ld : std_logic;
  signal losr_do : std_logic;
  signal y       : std_logic;  -- character luminance pixel value (0 or 1)

  -- control io register
  signal ctl       : std_logic_vector(7 downto 0);
  signal vga_en    : std_logic;
  signal ctl_r     : std_logic;
  signal ctl_g     : std_logic;
  signal ctl_b     : std_logic;

  component ctrm
    generic (
      M : integer := 08);
    port (
      reset : in  std_logic;            -- asyncronous reset
      clk   : in  std_logic;
      ce    : in  std_logic;            -- enable counting
      rs    : in  std_logic;            -- syncronous reset
      do    : out integer range (M-1) downto 0
      );
  end component;

  component losr
    generic (
      N : integer := 04);
    port (
      reset : in  std_logic;
      clk   : in  std_logic;
      load  : in  std_logic;
      ce    : in  std_logic;
      do    : out std_logic;
      di    : in  std_logic_vector(N-1 downto 0));
  end component;

begin

-------------------------------------------------------------------------------
-------------------------------------------------------------------------------
-- hsync generator, initialized with '1'
  process (reset, clk25MHz)
  begin
    if reset = '1' then
      hsync_int <= '1';
    elsif rising_edge(clk25MHz) then

      if (hctr > 663) and (hctr < 757) then
        hsync_int <= '0';
      else
        hsync_int <= '1';
      end if;

    end if;
  end process;


-------------------------------------------------------------------------------
-------------------------------------------------------------------------------
-- vsync generator, initialized with '1'
  process (reset, clk25MHz)
  begin
    if reset = '1' then
      vsync_int <= '1';
    elsif rising_edge(clk25MHz) then
      if (vctr > 499) and (vctr < 502) then
        vsync_int <= '0';
      else
        vsync_int <= '1';
      end if;
    end if;
  end process;

-------------------------------------------------------------------------------
-------------------------------------------------------------------------------
-- Blank signal, 0 = no draw, 1 = visible/draw zone

-- Proboscide99 31/08/08
--  blank <= '0' when (hctr > 639) or (vctr > 479) else '1';
  blank <= '0' when (hctr < 8) or (hctr > 647) or (vctr > 479) else '1';

-------------------------------------------------------------------------------
-------------------------------------------------------------------------------
-- flip-flips for sync of R, G y B signal, initialized with '0'
  process (reset, clk25MHz)
  begin
    if reset = '1' then
      R <= '0';
      G <= '0';
      B <= '0';
    elsif rising_edge(clk25MHz) then
      R <= R_int;
      G <= G_int;
      B <= B_int;
    end if;
  end process;


-------------------------------------------------------------------------------
-------------------------------------------------------------------------------
-------------------------------------------------------------------------------
-------------------------------------------------------------------------------
  -- Control register. Individual control signal
  vga_en    <= octl(7);
  ctl_r     <= octl(2);
  ctl_g     <= octl(1);
  ctl_b     <= octl(0);

  -- counters, hctr, vctr, srcx, srcy, chrx, chry
  -- TODO: OPTIMIZE THIS
  counters : block
    signal hctr_ce : std_logic;
    signal hctr_rs : std_logic;
    signal vctr_ce : std_logic;
    signal vctr_rs : std_logic;

    signal chrx_ce : std_logic;
    signal chrx_rs : std_logic;
    signal chry_ce : std_logic;
    signal chry_rs : std_logic;
    signal scrx_ce : std_logic;
    signal scrx_rs : std_logic;
    signal scry_ce : std_logic;
    signal scry_rs : std_logic;

    signal hctr_639 : std_logic;
    signal vctr_479 : std_logic;
    signal chrx_009 : std_logic;
    signal chry_014 : std_logic;

    -- RAM read, ROM read
    signal ram_tmp : integer range 2047 downto 0;  --11 bits
    signal rom_tmp : integer range 3839 downto 0;  --12 bits

  begin

    U_HCTR : ctrm generic map (M => 794) port map (
      reset =>reset, clk=>clk25MHz, ce =>hctr_ce, rs =>hctr_rs, do => hctr);

    U_VCTR : ctrm generic map (M => 525) port map (reset, clk25MHz, vctr_ce, vctr_rs, vctr);

    hctr_ce <= '1';
    hctr_rs <= '1' when hctr = 793 else '0';
    vctr_ce <= '1' when hctr = 663 else '0';
    vctr_rs <= '1' when vctr = 524 else '0';

    U_CHRX: ctrm generic map (M => 010) port map (reset, clk25MHz, chrx_ce, chrx_rs, chrx);
    U_CHRY: ctrm generic map (M => 015) port map (reset, clk25MHz, chry_ce, chry_rs, chry);
    U_SCRX: ctrm generic map (M => 064) port map (reset, clk25MHz, scrx_ce, scrx_rs, scrx);
    U_SCRY: ctrm generic map (M => 032) port map (reset, clk25MHz, scry_ce, scry_rs, scry);

    hctr_639 <= '1' when hctr = 639 else '0';
    vctr_479 <= '1' when vctr = 479 else '0';
    chrx_009 <= '1' when chrx = 009 else '0';
    chry_014 <= '1' when chry = 014 else '0';

    chrx_rs <= chrx_009 or hctr_639;
    chry_rs <= chry_014 or vctr_479;
    scrx_rs <= hctr_639;
    scry_rs <= vctr_479;

    chrx_ce <= '1' and blank;
    scrx_ce <= chrx_009;
    chry_ce <= hctr_639 and blank;
    scry_ce <= chry_014 and hctr_639;

    ram_tmp <= scry * 64 + scrx;
    TEXT_A <= std_logic_vector(TO_UNSIGNED(ram_tmp, 11));

    rom_tmp <= TO_INTEGER(unsigned(TEXT_D)) * 15 + chry;
    FONT_A <= std_logic_vector(TO_UNSIGNED(rom_tmp, 12));

  end block;
-------------------------------------------------------------------------------
-------------------------------------------------------------------------------
-------------------------------------------------------------------------------

  U_LOSR : losr generic map (N => 10)
    port map (reset, clk25MHz, losr_ld, losr_ce, losr_do, FONT_D);

  losr_ce <= blank;
  losr_ld <= '1' when (chrx = 009) else '0';

  -- video out, vga_en control signal enable/disable vga signal
  R_int <= (ctl_r and y) and blank;
  G_int <= (ctl_g and y) and blank;
  B_int <= (ctl_b and y) and blank;

  hsync <= hsync_int and vga_en;
  vsync <= vsync_int and vga_en;
  y     <= losr_do;

end rtl;
