TOPNAME = mycpu_top
VSRC_DIR = ./vsrc
VSRC_SUB_DIR = $(shell find -L $(VSRC_DIR)/ -type d)
VINCLUDE = $(addprefix -I, $(VSRC_SUB_DIR))
VLOG_DIR = ./vlogs
MAINNAME = mysim_main
# verilator -Mdir Name of output object directory
VXX_MDIR = ./obj_dir
CSRC_DIR = ./csrc
VXX_WNO = -Wno-caseincomplete \
		  -Wno-width \
		  -Wno-pinmissing \
		  -Wno-implicit \
		  -Wno-timescalemod
CFLAGS = -I../include -g
NPROC = $(shell nproc)
VXXFLAG += --trace --cc -D__SIM_IP__ --Mdir $(VXX_MDIR) $(VXX_WNO) -LDFLAGS "-lpthread" --relative-includes $(VINCLUDE) -CFLAGS "$(CFLAGS)" -j $(NPROC)

VXXBIN = V$(TOPNAME)
VSRC_TOP = $(VSRC_DIR)/$(TOPNAME).v
VSRC_ALL = $(shell find $(VSRC_DIR) -type f -name "*.v")
CSRC = $(shell find $(CSRC_DIR) -type f -name "*.cpp")

.PHONY: clean head sim wave var

var:

include ./scripts/config.mk

head: $(VSRC_ALL)
	verilator $(VXXFLAG) $(VSRC_TOP) 

$(VXXBIN): $(VSRC_ALL) $(CSRC)
	verilator $(VXXFLAG) --exe $(VSRC_TOP) $(CSRC) --build
 
sim: $(VXXBIN)
	$(VXX_MDIR)/V$(TOPNAME)

wave:
	gtkwave ./*.vcd signals.sav -S marker.tcl

clean:
ifndef VSRC_DIR
	@echo Please first DEFINE variable "VXX_MDIR"
else
	rm -rf $(VXX_MDIR)/*
endif
