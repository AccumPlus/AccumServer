module				:= $(shell basename `pwd`)

compile_flags		:= -std=c++11 -fPIC

obj_dir				:= ../../$(obj_dir)$(module)/

ifeq ($(build_type), debug)
	module			:= $(addsuffix _debug, $(module))
endif

module				:= $(addsuffix .so, $(addprefix $(obj_dir)lib, $(module)))

src_files			:= $(shell find . -maxdepth 1 -name '*.cpp')
obj_files			:= $(src_files:.cpp=.o)
obj_files			:= $(addprefix $(obj_dir), $(obj_files))

.PHONY: install

all: prepare $(module)

prepare:
	mkdir -p $(obj_dir)

$(module): $(obj_files)
	g++ -o $@ -shared $<

$(info Obj dir is $(obj_dir))

$(obj_dir)%.o:%.cpp
	g++ -fPIC -c $< -o $@

install:
	mkdir -p $(bin_dir)
	cp $(module) $(bin_dir)
