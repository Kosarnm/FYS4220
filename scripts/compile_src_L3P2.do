vlib work

quietly set prj_path ".."


vcom -2008 ../hdl/i2c_master.vhd
vcom -2008 ../hdl/i2c_master_ctrl.vhd

do compile_uvvm_util.do

vcom -2008 ../hdl/tb/tmp175_simmodel.vhd
vcom -2008 ../hdl/tb/i2c_master_pkg.vhd
vcom -2008 ../hdl/tb/i2c_master_ctrl_tb.vhd

vsim i2c_master_ctrl_tb
run -all