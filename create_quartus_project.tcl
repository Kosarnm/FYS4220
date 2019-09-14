#Create project
project_new fys4220  -revision i2c_master_ctrl -overwrite

set_global_assignment -name FAMILY "Cyclone V"
set_global_assignment -name DEVICE  5CSEMA5F31C6
source fys4220_de1_soc_board.tcl


set_global_assignment -name PROJECT_OUTPUT_DIRECTORY output_files
set_global_assignment -name SDC_FILE fys4220_de1_soc_board.sdc


#Include vhd files
set_global_assignment -name VHDL_FILE hdl/i2c_master.vhd
set_global_assignment -name VHDL_FILE hdl/i2c_master_ctrl.vhd

####
export_assignments

# The two next lines will compile project, comment out if you
# would like to compile the code using the Quartus graphical user interface
#load_package flow
#execute_flow -compile

#project_close