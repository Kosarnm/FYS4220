library IEEE;
use IEEE.std_logic_1164.all;
use IEEE.numeric_std.all;

entity lab1 is
  port (
	LED : out std_logic_vector(9 downto 0); -- Red LEDs
	SW : in std_logic_vector(9 downto 0);    -- Switches end entity lab1;
	HEX0 : out std_logic_vector(6 downto 0); -- 7seg display
	clk50 : in std_logic;
	reset_n : in std_logic;
	ext_ena_n : in std_logic
	);
end entity lab1;


architecture top_level of lab1 is

	--declaring the counter
	signal counter : unsigned(3 downto 0) := "0000";

	
	
	-- Internal signal to synchronize and input signal and to perform edge detection  
	signal key_r   : std_logic; -- synchronization register 1  
	signal key_rr  : std_logic; -- synchronization register 2  
	signal key_rrr : std_logic; -- register used for edge detection  

	
	
	
begin

--led display the switch value
LED <= SW;


p_sync : process(clk50) 
begin
	if rising_edge(clk50) then 
	
		key_r   <= ext_ena_n;  
 		key_rr  <= key_r;  
 		key_rrr <= key_rr;  

	
	
	end if;
end process;



process(clk50)
begin
	if(rising_edge(clk50)) then
	
		if(key_rrr = '0' and key_rr = '1') then
			counter <= counter + 1;
		end if;
		
		if reset_n = '0' then
				counter <= "0000";
		end if;
	end if;


	
end process;



with counter select
	HEX0 <= 	"1000000" when "0000", -- 0
				"1111001" when "0001", -- 1
				"0100100" when "0010", -- 2
				"0110000" when "0011", -- 3
				"0011001" when "0100", -- 4
				"0010010" when "0101", -- 5
				"0000010" when "0110", -- 6
				"1111000" when "0111", -- 7
				"0000000" when "1000", -- 8
				"0010000" when "1001", -- 9
				"0001000" when "1010", -- A
				"0000011" when "1011", -- b
				"1000110" when "1100", -- C
				"0100001" when "1101", -- d
				"0000110" when "1110", -- E
				"0001110" when "1111", -- F
				"1111111" when others;


end architecture top_level;