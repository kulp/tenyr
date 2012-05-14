--
-- Text Screen Video Controller.
-- Pixel resolution is 640x480/60Hz, 8 colors (3-bit DAC).
--
-- 2007 Javier Valcarce Garc�a, javier.valcarce@gmail.com
-- $Id$

library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;


entity vga80x40_test is
  port (
    reset    : in  std_logic;
    clk50MHz : in  std_logic;
    R        : out std_logic;
    G        : out std_logic;
    B        : out std_logic;
    hsync    : out std_logic;
    vsync    : out std_logic
    );
end vga80x40_test;


architecture behavioral of vga80x40_test is

	component vga80x40
    port (
      reset       : in  std_logic;
      clk25MHz    : in  std_logic;
      R           : out std_logic;
      G           : out std_logic;
      B           : out std_logic;
      TEXT_A           : out std_logic_vector(11 downto 0);
      TEXT_D           : in  std_logic_vector(07 downto 0);
		FONT_A           : out std_logic_vector(11 downto 0);
      FONT_D           : in  std_logic_vector(07 downto 0);
      hsync       : out std_logic;
      vsync       : out std_logic;
      ocrx    : in  std_logic_vector(7 downto 0);
      ocry    : in  std_logic_vector(7 downto 0);
      octl    : in  std_logic_vector(7 downto 0)
      );   
  end component;
  
  component textram
    port (
      clka  : in  std_logic;
      dina  : in  std_logic_vector(07 downto 0);
      addra : in  std_logic_vector(11 downto 0);
      wea   : in  std_logic_vector(00 downto 0);
      douta : out std_logic_vector(07 downto 0);
      clkb  : in  std_logic;
      dinb  : in  std_logic_vector(07 downto 0);
      addrb : in  std_logic_vector(11 downto 0);
      web   : in  std_logic_vector(00 downto 0);
      doutb : out std_logic_vector(07 downto 0));

  end component;

	component fontrom
    port (
    clka: IN std_logic;
    addra: IN std_logic_VECTOR(11 downto 0);
    douta: OUT std_logic_VECTOR(7 downto 0));
	end component;

  signal clk25MHz    : std_logic;
  signal crx_oreg_ce : std_logic;
  signal cry_oreg_ce : std_logic;
  signal ctl_oreg_ce : std_logic;
  signal crx_oreg    : std_logic_vector(7 downto 0);
  signal cry_oreg    : std_logic_vector(7 downto 0);
  signal ctl_oreg    : std_logic_vector(7 downto 0);


  -- Text Buffer RAM Memory Signals, Port B (to CPU core)
  signal ram_diA : std_logic_vector(07 downto 0);
  signal ram_doA : std_logic_vector(07 downto 0);
  signal ram_adA : std_logic_vector(11 downto 0);
  signal ram_weA : std_logic_vector(00 downto 0);

  -- Text Buffer RAM Memory Signals, Port B (to VGA core)
  signal ram_diB : std_logic_vector(07 downto 0);
  signal ram_doB : std_logic_vector(07 downto 0);
  signal ram_adB : std_logic_vector(11 downto 0);
  signal ram_weB : std_logic_vector(00 downto 0);
  
  
  -- Font Buffer RAM Memory Signals
  signal rom_adB : std_logic_vector(11 downto 0);
  signal rom_doB : std_logic_vector(07 downto 0);
  
begin

  --Clock divider /2. Pixel clock is 25MHz
  clk25MHz <= '0' when reset = '1' else
              not clk25MHz when rising_edge(clk50MHz);
  
  U_VGA : vga80x40 port map (
    reset       => reset,
    clk25MHz    => clk25MHz,
    R           => R,
    G           => G,
    B           => B,
    hsync       => hsync,
    vsync       => vsync,
    TEXT_A      => ram_adB,
    TEXT_D      => ram_doB,
    FONT_A      => rom_adB,
    FONT_D      => rom_doB,
    ocrx    => crx_oreg,
    ocry    => cry_oreg,
    octl    => ctl_oreg);

  U_TEXT: textram port map (
    clka  => clk25MHz,
    dina  => ram_diA,
    addra => ram_adA,
    wea   => ram_weA,
    douta => ram_doA,
    clkb  => clk25MHz,
    dinb  => ram_diB,
    addrb => ram_adB,
    web   => ram_weB,
    doutb => ram_doB
    );
  U_FONT: fontrom port map (
    clka => CLK25mhZ,
    addra => rom_adB,
    douta => rom_doB);
	 
  ram_weA <= "0";
  ram_weB <= "0";
  ram_diA <= (others => '0');
  ram_adA <= (others => '0');
  ram_diB <= (others => '0');

  crx_oreg    <= std_logic_vector(TO_UNSIGNED(40, 8));
  cry_oreg    <= std_logic_vector(TO_UNSIGNED(20, 8));
  ctl_oreg    <= "11110010";
  crx_oreg_ce <= '1';
  cry_oreg_ce <= '1';
  ctl_oreg_ce <= '1';
  
end behavioral;
