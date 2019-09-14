library IEEE;
use IEEE.std_logic_1164.all;
use IEEE.numeric_std.all;

entity i2c_master is
	generic(
		GC_SYSTEM_CLK : integer := 50_000_000;
		GC_I2C_CLK : integer := 2_00_000
	);
	
	port(
		clk : in std_logic;
		rst_n : in std_logic;
		valid : in std_logic;
		addr : in std_logic_vector(6 downto 0);
		rnw : in std_logic;
		data_wr : in std_logic_vector(7 downto 0);
		data_rd: out std_logic_vector(7 downto 0);
		busy: out std_logic;
		ack_error: out std_logic;
		sda: inout std_logic;
		scl: inout std_logic
	);
end entity i2c_master;

architecture i2c_master_arch of i2c_master is

constant C_SCL_PERIOD : integer := GC_SYSTEM_CLK/GC_I2C_CLK;
constant C_SCL_HALF_PERIOD : integer := C_SCL_PERIOD/2;
constant C_STATE_TRIGGER : integer := C_SCL_PERIOD/4;
constant C_SCL_TRIGGER : integer := C_SCL_PERIOD*3/4;

--signal state : state_type;
signal state_ena : std_logic := '0';
signal scl_high_ena : std_logic := '0';
signal scl_clk : std_logic := '0';
signal scl_oe : std_logic := '1';
signal ack_error_i : std_logic := '0';
signal sda_i : std_logic :=  '1';
signal addr_rnw_i : std_logic_vector(7 downto 0);
signal data_tx : std_logic_vector(7 downto 0);
signal data_rx : std_logic_vector(7 downto 0);


signal bit_cnt : integer range 0 to 7;

type state_type is (sIDLE, sSTART, sADDR, sACK1, sWRITE, sREAD, sACK2, sMACK, sSTOP);
signal state : state_type;


-- rnw_i is the least significant bit of the internal signal addr_rnw_i
alias rnw_i : std_logic is addr_rnw_i(0);

begin

	scl <= '0' when ((scl_oe = '1') and (scl_clk = '0')) else 'Z';
	sda <= 'Z' when (sda_i = '1') else '0';
	
p_sclk: process(clk)
	
	variable cnt : integer := 0;
	begin
			if rising_edge(clk) then
				cnt := cnt + 1;
				if(cnt = C_SCL_HALF_PERIOD) then
					cnt := 0;
					scl_clk <= not scl_clk;
				end if;
			end if;
	end process;

	
	p_ctrl: process(clk)
	
	variable cnt : integer := 0;
	begin
			if rising_edge(clk) then
				cnt := cnt + 1;
				
				if(cnt = C_SCL_TRIGGER) then
					scl_high_ena <= '1';
				end if;
				
				if((scl_high_ena = '1') and (cnt = C_SCL_TRIGGER + 1) ) then
					scl_high_ena <= '0';
				end if;

				
				if(cnt = C_STATE_TRIGGER) then
					state_ena <= '1';
				end if;
				
				if((state_ena = '1') and (cnt = C_STATE_TRIGGER + 1) ) then
					state_ena <= '0';
				end if;
				
				
				
				
				
				
				if (cnt = C_SCL_PERIOD) then
					cnt := 0;
				end if;
				
				
				
				
				
				
			end if;
	end process;

	p_state: process(clk)

	
		begin
		
			if (rising_edge(clk) ) then
			
				--asynchronously concatenating address with rnw_i
				data_tx <= data_wr;
				addr_rnw_i(7 downto 1) <= addr;
				addr_rnw_i(0) <= rnw;
				ack_error <= ack_error_i;		
			
				case state is
				
				when sIDLE => 
					busy <= '0';
					sda_i <= '1';
					scl_oe <= '0';
					if ((state_ena = '1') and (valid = '1')) then
						scl_oe <= '1'; -- reenabling the scl line to Z output
						state <= sSTART;
					end if;
					
				when sSTART => 
					busy <= '1';
					
					if(scl_high_ena = '1') then
							sda_i <= '0';
					end if;
				
					if (state_ena = '1') then
						bit_cnt <= 7;
						state <= sADDR;
					end if;
					
				when sADDR => 
					busy <= '1';
					sda_i <= addr_rnw_i(bit_cnt);
					
					if(state_ena = '1') then
						if ( (bit_cnt = 0)) then
							bit_cnt <= 7;
							state <= sACK1;
						elsif ( (bit_cnt /= 0)) then
							-- state <= sADDR;
							bit_cnt <= bit_cnt - 1 ;
						end if;
					end if;
				
				when sACK1 => 
					busy <= '1';
					sda_i <= '1';
				
					if(state_ena = '1') then
						if ((rnw_i = '0')) then
							state <= sWRITE;
						elsif ( (rnw_i = '1')) then
							state <= sREAD;
						end if;
				
						if(sda_i /= '0') then
							ack_error_i <= '1';
						end if;
					end if;
				
				when sWRITE => 
					busy <= '1';
					sda_i <= data_tx(bit_cnt);
				
					if(state_ena = '1') then
						if ((bit_cnt = 0)) then
							bit_cnt <= 7;
							state <= sACK2;
						elsif ((bit_cnt /= 0)) then
							bit_cnt <= bit_cnt - 1 ;
						end if;
					end if;
		
				when sREAD => 
					busy <= '1';
					sda_i <= '1';
		
					if(scl_high_ena = '1') then
						data_rx(bit_cnt) <= sda;
					end if;
					
					if(state_ena = '1') then
						if ( (bit_cnt = 0)) then
							bit_cnt <= 7;
							data_rd <= data_rx;
							state <= sMACK;
						elsif ((bit_cnt /= 0)) then
							bit_cnt <= bit_cnt - 1 ;
						end if;	
					end if;
					

				when sACK2 => 
					busy <= '0';
					sda_i <= '1';
				
					if(state_ena = '1') then
						if(scl_high_ena = '1') then
							ack_error <= '1';
						end if;
						
						if ( (valid = '1') and (rnw = '0')) then
							data_tx <= data_wr;
							state <= sWRITE;
						elsif ( (valid = '1') and (rnw = '1')) then
							state <= sSTART;
						elsif (valid = '0') then
							sda_i <= '0';
							state <= sSTOP;
						end if;	
					end if;
				
				
				when sMACK =>
					busy <= '0';

					if(state_ena = '1') then	
						if (valid = '0') then
							sda_i <= '0';
							state <= sSTOP;
						elsif(valid = '1') then
							if(rnw = '1') then
								state <= sREAD;
							else
								state <= sSTART;
							end if;
						end if;	
					else
						if (valid = '1') then
							sda_i <= '0';
						else
							sda_i <= '1'; -- default value of sda_i is set to '1'
						end if;
					end if;
					
			
				when sSTOP => 
					busy <= '1';
					
					if (state_ena = '1') then
						state <= sIDLE;
					end if;
					
					if(scl_high_ena = '1') then
						sda_i <= '1'; -- default value of sda_i is set to '1'sda_i <= '1'; -- default value of sda_i is set to '1'
					end if;
					
				when others => 
					state <= state;
				
				end case;
				
			end if;
			
		end process;

end architecture i2c_master_arch;




