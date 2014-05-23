-- Copyright 2014 Raytheon BBN Technologies
-- Original Author: Colm Ryan (cryan@bbn.com)

-- See https://www.doulos.com/knowhow/fpga/fastcounter/

library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
use IEEE.NUMERIC_STD.ALL;

entity Flancter is
  port (
	sysclk : in std_logic;
	pulse : in std_logic;
	clr : in std_logic;
	enable : in std_logic;
	flag : out std_logic
  ) ;
end entity ; -- Flancter

architecture arch of Flancter is

signal setFlop, clrFlop : std_logic := '0';
signal flagSync : std_logic := '0';

begin


--Process to capture fast trigger pulse
capture : process( pulse )
begin
	if rising_edge(pulse) then 
		if enable = '1' then
			--flops get set opposite
			setFlop <= not clrFlop;
		end if ;
	end if;
end process ; -- capture

--Process to reset
clrProc : process( sysclk )
begin
	if rising_edge(sysclk) then
		if clr = '1' then
			clrFlop <= setFlop;
		end if ;
	end if ;
end process ; -- clr

--Double register the flag to syncronize onto sysclk domain
flagSyncPro : process( sysclk )
begin
	if rising_edge(sysclk) then
		if clr = '1' then
			flagSync <= '0';
			flag <= '0';
		else
			--looks for when flops are different
			flagSync <= setFlop xor clrFlop;
			flag <= flagSync;
		end if;
	end if ;
end process ; -- flagSync

end architecture ; -- arch