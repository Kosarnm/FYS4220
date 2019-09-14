library IEEE;
use IEEE.std_logic_1164.all;
use IEEE.numeric_std.all;


entity system_top is

	port(
			clk : in std_logic;
			rst_n : in std_logic;
			sw : in std_logic_vector(9 downto 0);
			keys : in std_logic_vector(3 downto 1);
			led : out std_logic_vector(9 downto 0);
			alert : in std_logic;
			sda : inout std_logic;
			scl : inout std_logic
			);
		


end entity system_top;

architecture system_top_arch of system_top is

	signal keys_r, keys_rr 		: std_logic_vector(2 downto 0);
	signal alert_r, alert_rr 	: std_logic;
	
	signal interrupt_port 		: std_logic_vector(3 downto 0);
	
	
	
	component nios2_system is
		port (
			clk_clk                  : in  std_logic                    := 'X';             -- clk
			interrupt_pio_ext_export : in  std_logic_vector(3 downto 0) := (others => 'X'); -- export
			ledpio_ext_export        : out std_logic_vector(9 downto 0);                    -- export
			reset_reset_n            : in  std_logic                    := 'X';             -- reset_n
			swpio_ext_export         : in  std_logic_vector(9 downto 0) := (others => 'X');  -- export
			i2c_wrapper_sda_ext_export : inout std_logic                    := 'X';             -- export
			i2c_wrapper_scl_ext_export : inout std_logic                    := 'X'              -- export			
		);
	end component nios2_system;

	begin
		
		interrupt_port(2 downto 0) <= keys_rr;
		interrupt_port(3) 			<= alert_rr;
		
		
		u0 : component nios2_system
			port map (
				clk_clk 												=> clk,
				reset_reset_n 										=> rst_n,
				ledpio_ext_export									=> led,
				swpio_ext_export									=> sw,
				interrupt_pio_ext_export						=> interrupt_port,
				i2c_wrapper_sda_ext_export 					=> sda,
				i2c_wrapper_scl_ext_export 					=> scl
				
				);
	

p_sync: process(clk)

	begin

		if(rising_edge(clk)) then
			
			keys_r 	<= keys(3 downto 1);
			keys_rr 	<= keys_r;
			
			alert_r 	<= alert;
			alert_rr	<= alert_r;
			
		end if;

end process;



end architecture;