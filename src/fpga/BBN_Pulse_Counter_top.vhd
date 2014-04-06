library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
use IEEE.NUMERIC_STD.ALL;

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


signal clk_50MHz, clk_100MHz, clk_125MHz : std_logic := '0';
signal pllLocked : std_logic := '0';

signal reconfig_fromgxb : std_logic_vector(16 downto 0) := (others => '0');
signal reconfig_togxb : std_logic_vector(3 downto 0) := (others => '0');
signal reconfigBusy : std_logic := '0';


--Streaming data
signal data_in_data               : std_logic_vector(31 downto 0) := (others => '0'); 
signal data_in_valid              : std_logic                     := '0';             
signal data_in_ready              : std_logic                     := '0';                    

--CSR signals
signal csrReadData, csrWriteData : std_logic_vector(31 downto 0) := (others => '0');
signal csrAddr : unsigned(4 downto 0) := (others => '0') ;
signal csrWE : std_logic := '0';

signal controlWord, ledRate, clkCount : std_logic_vector(31 downto 0) ;

--Pulse counts
type PulseCountArray_t is array(0 to 15) of unsigned(31 downto 0) ;
signal pulseCounts : PulseCountArray_t := (others => (others => '0'));
signal fakePulses : std_logic_vector(3 downto 0) ;

begin

--Transform 100MHz input clock into needed frequencies
myPLL : entity work.MyALTPLL
	port map (
		areset => not resetn,
		inclk0 => clk_bot,
		c0 => clk_100MHz,
		c1 => clk_125MHz,
		c2 => clk_50MHz,
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
		data_in_clk_clk           => clk_100MHz, 
        data_in_clk_reset_reset_n => resetn,
        data_in_data              => data_in_data,
        data_in_valid             => data_in_valid,
        data_in_ready             => data_in_ready,

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
 		ledRate <= (others => '0');
 	elsif rising_edge(clk_100Mhz) then

 		csrAddr <= csrAddr + 1;

 		if (csrAddr < 15) or (csrAddr = 31) then 
	 		csrWE <= '0';
	 	else
	 		csrWE <= '1';
			csrWriteData <= std_logic_vector(pulseCounts(to_integer(csrAddr - 15)));
	 	end if;
 	
 		if (csrAddr = b"00001") then
 			controlWord <= csrReadData;
 		end if;
 		if (csrAddr = b"00010") then
 			ledRate <= csrReadData;
 		end if;
 		if (csrAddr = b"00011") then
 			clkCount <= csrReadData;
 		end if;
	 		
	end if;

end process ; -- csr

--Count pulses coming in 
genCounters: for ct in 0 to 3 generate
	counter : entity work.PulseCounter
		port map(
			reset => not resetn,
			clk => clk_100MHz,
			clkCount => clkCount,
			pulseLine => fakePulses(ct),
			unsigned(pulseCount) => pulseCounts(ct)
		);
end generate genCounters;


genCountersBis: for ct in 4 to 7 generate
	counterBis : entity work.PulseCounter
		port map(
			reset => not resetn,
			clk => clk_100MHz,
			clkCount => clkCount,
			pulseLine => pulseInputs(ct),
			unsigned(pulseCount) => pulseCounts(ct)
		);
end generate genCountersBis;


--Mock up data streaming in as a counter for now
fakeData : process( resetn, clk_100MHz )
variable counter : unsigned(31 downto 0) := (others => '0');
begin
	data_in_data <= std_logic_vector(counter);
	data_in_valid <= '1';
	if resetn = '0' then
		counter := (others => '0');
	elsif rising_edge(clk_100MHz) and data_in_ready = '1' then
		counter := counter + 1;
	end if;
end process ; -- fakeData

pulseCounts(8) <= x"BAADF00F";
pulseCounts(9) <= x"BAADF11F";
pulseCounts(10) <= x"BAADF22F";
pulseCounts(11) <= x"BAADF33F";
pulseCounts(12) <= x"BAADF44F";

--For now put a slow counter out on the LEDS so we know the board alive

countpro : process( resetn, clk_100MHz )

variable counter : unsigned(31 downto 0) := (others => '0');
variable slowCounter : unsigned(3 downto 0) := (others => '0');

begin
	leds <= not std_logic_vector(slowCounter);
	fakePulses <= std_logic_vector(slowCounter);
	if resetn = '0' then
		counter := (others => '0');
		slowCounter := (others => '0');
	elsif rising_edge(clk_100MHz) then
		if (counter < unsigned(ledRate)) then
			counter := counter + 1;
		else
			counter := (others => '0');
		end if;
		if counter = 0 then
			slowCounter := slowCounter + 1;
		end if;
	end if;
end process ; -- counter





end architecture ; -- arch