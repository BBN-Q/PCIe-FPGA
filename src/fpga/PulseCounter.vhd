-- Copyright 2014 Raytheon BBN Technologies
-- Original Author: Colm Ryan (cryan@bbn.com)

library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
use IEEE.NUMERIC_STD.ALL;

entity PulseCounter is
  port (
  	reset : in std_logic;
  	clk : in std_logic;
  	clkCount : in std_logic_vector(31 downto 0) ;
  	pulseLine : in std_logic;
  	pulseCount : out std_logic_vector(31 downto 0) 
  ) ;
end entity ; -- PulseCounter

architecture arch of PulseCounter is

signal curCount : unsigned(31 downto 0) := (others => '0');
signal resetPulseCt : boolean := false;

begin

--Latch the latest pulse count every clkCount 
countLatch : process( reset, clk )
variable ct : unsigned(31 downto 0);
begin
	if reset = '1' then
		ct := (others => '0');
		pulseCount <= (others => '0');
	elsif rising_edge(clk) then
		if ( ct = 0) then
			ct := unsigned(clkCount);
			pulseCount <= std_logic_vector(curCount);
			resetPulseCt <= true;
		else
			ct := ct -1;
			resetPulseCt <= false;
		end if;
	end if;
end process ; -- countLatch

--Count rising edges of pulses coming in
pulseCounter : process( reset, resetPulseCt, pulseLine )
begin
	if ((reset = '1') or resetPulseCt) then
		curCount <= (others => '0');
	elsif rising_edge(pulseLine) then
		curCount <= curCount + 1;
	end if;
end process ; -- pulseCounter

end architecture ; -- arch