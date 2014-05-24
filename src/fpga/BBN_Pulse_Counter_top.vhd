-- Copyright 2014 Raytheon BBN Technologies
-- Original Author: Colm Ryan (cryan@bbn.com)

library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
use IEEE.NUMERIC_STD.ALL;
use IEEE.STD_LOGIC_MISC.ALL;

library streaming_sgdma;
use streaming_sgdma.streaming_sgdma;

entity BBN_Pulse_Counter_top is
	port (
		resetn : in std_logic; --push button reset USER_PB0
		clk_bot : in std_logic; -- 100MHz CLKIN_BOT_P
		leds : out std_logic_vector(3 downto 0); --leds for status	

		--PCIe signals
		pcie_rstn : in std_logic;
		refclk : in std_logic;
		rx_in0 : in std_logic;
		rx_in1 : in std_logic;
		rx_in2 : in std_logic;
		rx_in3 : in std_logic;
		tx_out0 : out std_logic;
		tx_out1 : out std_logic;
		tx_out2 : out std_logic;
		tx_out3 : out std_logic;

		--HSMC lines for pulse counting
		pulseInputs : in std_logic_vector(15 downto 0) 
	) ;

end entity ; -- BBN_Pulse_Counter_top

architecture arch of BBN_Pulse_Counter_top is


signal clk_50MHz, clk_100MHz, clk_125MHz, clk_200MHz : std_logic := '0';
signal pllLocked : std_logic := '0';

signal reconfig_fromgxb : std_logic_vector(16 downto 0) := (others => '0');
signal reconfig_togxb : std_logic_vector(3 downto 0) := (others => '0');
signal reconfigBusy : std_logic := '0';


--Streaming data
signal data_in_data               : std_logic_vector(15 downto 0) := (others => '0'); 
signal data_in_valid              : std_logic                     := '0';             
signal data_in_ready              : std_logic                     := '0';                    

--CSR signals
signal csrReadData, csrWriteData : std_logic_vector(31 downto 0) := (others => '0');
signal csrAddr : unsigned(4 downto 0) := (others => '0') ;
signal csrWE : std_logic := '0';

signal controlWord, clkRate : std_logic_vector(31 downto 0) ;

--Pulse counts
type Uint32Array_t is array(0 to 15) of unsigned(31 downto 0) ;
signal pulseCounts : Uint32Array_t := (others => (others => '0'));

signal statusRegs : Uint32Array_t := (others => (others => '0'));


signal fakePulses : std_logic_vector(15 downto 0) ;
type fakePulseArray_t is array(0 to 31) of std_logic_vector(15 downto 0);
constant fakePulseArray : fakePulseArray_t := (x"0001", x"0002", x"0004", x"0008", x"0010", x"0020", x"0040", x"0080", 
												x"0100", x"0200", x"0400", x"0800", x"1000", x"2000", x"4000", x"8000",
												x"0101", x"0202", x"0404", x"0808", x"1010", x"2020", x"4040", x"8080",
												x"0108", x"0204", x"0402", x"0801", x"1080", x"2040", x"4020", x"8010");


signal pulseFlags : std_logic_vector(15 downto 0) := (others => '0');

signal enable, clear : std_logic := '0';

begin

statusRegs(8) <= x"BAADF00F";
statusRegs(9) <= x"BAADF11F";
statusRegs(10) <= x"BAADF22F";
statusRegs(11) <= x"BAADF33F";
statusRegs(12) <= x"BAADF44F";

enable <= controlWord(0);

--Transform 100MHz input clock into needed frequencies
myPLL : entity work.MyALTPLL
	port map (
		areset => not resetn,
		inclk0 => clk_bot,
		c0 => clk_100MHz,
		c1 => clk_125MHz,
		c2 => clk_50MHz,
		c3 => clk_200MHz,
		locked => pllLocked);

--Reset reconfiguration: not really sure what this does
MyALTGX_RECONFIG_inst : entity work.MyALTGX_RECONFIG 
	port map (
		offset_cancellation_reset  => not pllLocked,
		reconfig_clk	 => clk_50MHz,
		reconfig_fromgxb	 => reconfig_fromgxb,
		busy	 => reconfigBusy,
		reconfig_togxb	 => reconfig_togxb);


qsys : entity streaming_sgdma.streaming_sgdma
	port map(
		--Reset from PCI bus
		pcie_hard_ip_0_pcie_rstn_export => pcie_rstn,

		--Clocks
		pcie_hard_ip_0_cal_blk_clk_clk => clk_50MHz,
		pcie_hard_ip_0_refclk_export => refclk,
		pcie_hard_ip_0_fixedclk_clk => clk_125MHz,
		pcie_hard_ip_0_reconfig_gxbclk_clk => clk_50MHz,
	
		--Reconfig: still not sure what this does
		pcie_hard_ip_0_reconfig_busy_busy_altgxb_reconfig => reconfigBusy,
		pcie_hard_ip_0_reconfig_togxb_data => reconfig_togxb,
		pcie_hard_ip_0_reconfig_fromgxb_0_data => reconfig_fromgxb,

		--Transiever ports
		pcie_hard_ip_0_rx_in_rx_datain_0   => rx_in0,
		pcie_hard_ip_0_rx_in_rx_datain_1   => rx_in1,
		pcie_hard_ip_0_rx_in_rx_datain_2   => rx_in2,
		pcie_hard_ip_0_rx_in_rx_datain_3   => rx_in3,
		pcie_hard_ip_0_tx_out_tx_dataout_0 => tx_out0,
		pcie_hard_ip_0_tx_out_tx_dataout_1 => tx_out1,
		pcie_hard_ip_0_tx_out_tx_dataout_2 => tx_out2,
		pcie_hard_ip_0_tx_out_tx_dataout_3 => tx_out3,

		--Test ports
		pcie_hard_ip_0_test_in_test_in => ( others => '0'),
		pcie_hard_ip_0_test_out_test_out => open,

		--Streaming Avalon ports
        data_in_data              => data_in_data,
        data_in_valid             => data_in_valid,
        data_in_ready             => data_in_ready,

		reset_bridge_0_in_reset_reset => not controlWord(0),
        clock_bridge_0_in_clk_clk => clk_200MHz,

        --On chip memory as poor-man's CSR
        onchip_memory2_0_clk2_clk => clk_100MHz,
        onchip_memory2_0_s2_address => x"00" & '0' & std_logic_vector(csrAddr),
        onchip_memory2_0_s2_chipselect => '1',
        onchip_memory2_0_s2_clken => pllLocked,
        onchip_memory2_0_s2_write => csrWE,
        onchip_memory2_0_s2_readdata => csrReadData,
        onchip_memory2_0_s2_writedata => csrWriteData,
        onchip_memory2_0_s2_byteenable => (others => '1'),
        onchip_memory2_0_reset2_reset => not resetn,
        onchip_memory2_0_reset2_reset_req => not resetn
    );


--Poor man's CSR, scan through first 32 words 
csr : process( resetn, clk_100MHz )
begin
	if resetn = '0' then
 		csrWriteData <= (others => '0');
 		csrAddr <= (others => '0');
 		csrWE <= '0';
 		controlWord <= (others => '0');
 		clkRate <= (others => '0');
 	elsif rising_edge(clk_100Mhz) then

 		csrAddr <= csrAddr + 1;

 		if (csrAddr < 15) or (csrAddr = 31) then 
	 		csrWE <= '0';
	 	else
	 		csrWE <= '1';
			csrWriteData <= std_logic_vector(statusRegs(to_integer(csrAddr - 15)));
	 	end if;
 	
 		if (csrAddr = b"00001") then
 			controlWord <= csrReadData;
 		end if;
 		if (csrAddr = b"00010") then
 			clkRate <= csrReadData;
 		end if;
	 		
	end if;

end process ; -- csr

--Setup flacters for all the pulse lines
genFlancters : for ct in 0 to 15 generate
	flancterGen : entity work.Flancter
		port map(
			sysclk => clk_200MHz,
			pulse => fakePulses(ct),
			clr => clear,
			enable => enable,
			flag => pulseFlags(ct)
		);
end generate ; -- genFlancters


--State machine to catch pulses
PulseCatcher : process( clk_200MHz )
type State_t is (IDLE, LATCH, CLR);
variable state : State_t := IDLE; 
variable ct : unsigned(31 downto 0) := (others => '0');

begin
	if rising_edge(clk_200MHz) then
		statusRegs(1) <= ct;

		case( state ) is

			when IDLE =>
				data_in_data <= (others => '0');
				data_in_valid <= '0';
				clear <= '0';

				--Wait for any of the flancter flags to go high
				if or_reduce(pulseFlags) = '1' then
					state := LATCH;
				end if;

			when LATCH =>
				data_in_data <= pulseFlags;
				data_in_valid <= '1';
				state := CLR;
				ct := ct+1;

			when CLR =>
				data_in_valid <= '0';
				clear <= '1';
				state := IDLE;

			when others =>
				null;
		
		end case ;
		
	end if ;
end process ; -- PulseCatcher

--For now put a slow counter out on the LEDS so we know the board alive

countpro : process( resetn, clk_100MHz )

variable counter : unsigned(31 downto 0) := (others => '0');
variable slowCounter : unsigned(3 downto 0) := (others => '0');

begin
	leds <= not std_logic_vector(slowCounter);
	if resetn = '0' then
		counter := (others => '0');
		slowCounter := (others => '0');
	elsif rising_edge(clk_100MHz) then
		if (counter < unsigned(clkRate)) then
			counter := counter + 1;
		else
			counter := (others => '0');
		end if;
		if counter = 0 then
			slowCounter := slowCounter + 1;
		end if;
	end if;
end process ; -- counter


--For now put a slow counter out on the LEDS so we know the board alive

countpro2 : process( resetn, clk_100MHz )

variable counter : unsigned(31 downto 0) := (others => '0');
variable slowCounter : unsigned(4 downto 0) := (others => '0');

begin
	if resetn = '0' or controlWord(0) = '0' then
		counter := (others => '0');
		slowCounter := (others => '0');
		fakePulses <= (others => '0');
	elsif rising_edge(clk_100MHz) then
		if (counter < unsigned(clkRate)) then
			counter := counter + 1;
		else
			counter := (others => '0');
		end if;
		if counter = 0 then
			slowCounter := slowCounter + 1;
			fakePulses <= fakePulseArray(to_integer(slowCounter));
		else
			fakePulses <= (others => '0');
		end if;
	end if;
end process ; -- counter


end architecture ; -- arch