--implemented the 7seg display with a counter to test the button/key to make sure that the reg will be filled with sw values but doesn't work --- WHYYY?

library IEEE;
use IEEE.std_logic_1164.all;
use IEEE.numeric_std.all;


entity i2c_master_ctrl is
	generic(
		GC_SYSTEM_CLK : integer := 50_000_000;
		GC_I2C_CLK : integer := 2_00_000
	);
	
	port(
		clk : in std_logic;
		rst_n : in std_logic;
		sw : in std_logic_vector(9 downto 0);
		keys : in std_logic_vector(3 downto 1);
		led : out std_logic_vector(9 downto 0);
		sda: inout std_logic;
		scl: inout std_logic
	);
end entity i2c_master_ctrl;


architecture i2c_master_ctrl_arch of i2c_master_ctrl is


type state_type is (sIDLE, sSTART, sWAIT_DATA, sWAIT_STOP, sWRITE_DATA);
signal state : state_type;


alias addr_reg : std_logic_vector is sw(9 downto 8);
alias load_reg : std_logic_vector is sw(7 downto 0);



--variable byte_count : unsigned(1 downto 0); 


--the output internal signals for mapping to the i2c_master
signal i2c_data_rd 	: std_logic_vector(7 downto 0);
signal i2c_busy 		: std_logic;
signal i2c_ack_error : std_logic;



-- the input internal signals for mapping to the i2c_master
signal i2c_data_wr 	: std_logic_vector(7 downto 0);
signal i2c_addr 		: std_logic_vector(6 downto 0);
signal i2c_rnw 		: std_logic;
signal i2c_valid 		: std_logic;


type Reg is array (0 to 3) of std_logic_vector(7 downto 0);


-- Internal signal to synchronize and input signal and to perform edge detection 
signal key3_r   : std_logic; -- synchronization register 1 
signal key3_rr  : std_logic; -- synchronization register 2 
signal key3_rrr : std_logic; -- register used for edge detection 


-- Internal signal to synchronize and input signal and to perform edge detection 
signal key2_r   : std_logic; -- synchronization register 1 
signal key2_rr  : std_logic; -- synchronization register 2 
signal key2_rrr : std_logic; -- register used for edge detection 


-- Internal signal to synchronize and input signal and to perform edge detection 
signal key1_r   : std_logic; -- synchronization register 1 
signal key1_rr  : std_logic; -- synchronization register 2 
signal key1_rrr : std_logic; -- register used for edge detection 


-- Internal signal to synchronize and input signal and to perform edge detection 
signal key0_r   : std_logic; -- synchronization register 1 
signal key0_rr  : std_logic; -- synchronization register 2 
signal key0_rrr : std_logic; -- register used for edge detection



-- Internal signal to synchronize and input signal and to perform edge detection 
signal busy_r   : std_logic; -- synchronization register 1 


signal Read_reg : Reg;
signal Write_reg : Reg;


begin

	-- Include the I2C master (DUT)
	I2c_master_submodule : entity work.i2c_master
	generic map (
		GC_SYSTEM_CLK => GC_SYSTEM_CLK,
		GC_I2C_CLK    => GC_I2C_CLK)
		port map (
			clk       => clk,
			rst_n     => rst_n,
			valid     => i2c_valid, 	--mapped
			addr      => i2c_addr,   	--mapped
			rnw       => i2c_rnw,		--mapped
			data_wr   => i2c_data_wr,  --mapped
			data_rd   => i2c_data_rd,	--mapped
			busy      => i2c_busy,		--mapped
			ack_error => i2c_ack_error,--mapped
			sda       => sda,
			scl       => scl
		);

	
	p_led: process(clk)
	begin
		if (rising_edge(clk)) then
			if(keys(1) = '1') then
				led(7 downto 0) <= Read_reg(0)(7 downto 0);
			else
				led(7 downto 0) <= Read_reg(1)(7 downto 0);
			end if;
		end if;
	end process;

	
	
 -- Synchronize external asynchronous signal with the clk50 domain. 
  -- Third register is used for edge detection 
   p_sync : process(clk) 
   begin 
     if rising_edge(clk) then 
	  
	  
			key3_r   <= keys(3); 
			key3_rr  <= key3_r; 
			key3_rrr <= key3_rr; 

			key2_r   <= keys(2); 
			key2_rr  <= key2_r; 
			key2_rrr <= key2_rr; 

			
			key1_r   <= keys(1); 
			key1_rr  <= key1_r; 
			key1_rrr <= key1_rr; 			
		 
		 	busy_r   <= i2c_busy; 
		 
     end if; 
   end process; 

	
	
	
	
	p_reg_load: process(clk)
	begin
		if (rising_edge(clk) ) then
		
		
			if (key3_rr = '0' and key3_rrr = '1') then
					
				if( addr_reg = "00") then
					Write_reg(0) <= load_reg;
				elsif( addr_reg = "01") then
					Write_reg(1) <= load_reg;
				elsif( addr_reg = "10") then
					Write_reg(2) <= load_reg;
				elsif( addr_reg = "11") then
					Write_reg(3) <= load_reg;
				end if;
							
			end if;
		
		end if;	
	
	end process;
	

	p_state: process(clk)
	
	
	variable byte_count : integer:= 0;
	
	begin
	
		if (rising_edge(clk) ) then
		
			case state is
		
			
				when sIDLE => 
				
					i2c_valid <= '0';
					
					if (key2_rr = '0' and key2_rrr = '1') then
						byte_count := to_integer(unsigned(sw(1 downto 0)));
						state <= sSTART;
					end if;
			
			
				when sSTART => 
				
					i2c_data_wr <= Write_reg(1);
					i2c_addr	<= Write_reg(0)(7 downto 1);
					i2c_rnw 		<= Write_reg(0)(0);
					i2c_valid 	<= '1';
				
					if (busy_r = '0' and i2c_busy = '1') then  --rising edge of busy
						state <= sWAIT_DATA;
					end if;
					
				when sWAIT_DATA => 
					
					if (busy_r = '1' and i2c_busy = '0') then  --falling edge of busy
						if(i2c_rnw = '1') then -- read condition
							
							Read_reg( (byte_count - 1)) <= i2c_data_rd; --
							
							byte_count := byte_count - 1 ;
	
	
	
							if(byte_count = 0) then
								state <= sWAIT_STOP;
							end if;
	
	
						else -- write condition

							if(byte_count = 0) then
								state <= sWAIT_STOP;
							else
								state <= sWRITE_DATA;
							end if;
						end if;
					end if;
					

				
				when sWAIT_STOP => 
				

					i2c_valid <= '0';
					
					if (busy_r = '1' and i2c_busy = '0') then  --falling edge of busy
						state <= sIDLE;
					end if;
				
				when sWRITE_DATA => 
				
	
					if (byte_count - 1 = 1) then
						i2c_data_wr <= Write_reg(3);
					elsif (byte_count-1 = 0) then
						i2c_data_wr <= Write_reg(2);
					end if;
					
					if (busy_r = '0' and i2c_busy = '1') then  --rising edge of busy
						byte_count := byte_count - 1;
						state <= sWAIT_DATA;
					end if;
					
					
				when others => 
					state <= state;
						
			end case;
		
		end if;
	
	end process;

	

end architecture i2c_master_ctrl_arch;