# ===================================
# Changeable variables

project 			:= AccumServer
build_type			:= debug

bin_dir				:= bin

#version major.minor.build
inc_version			:= build

modules				:= class

# ===================================

src_dir				:= src/

lib_files			:= $(modules)

version				:= $(shell gawk 'BEGIN{FS = ".";}{printf("%d %d %d", $$1, $$2, $$3);}' $(src_dir)version)
major 				:= $(word 1, $(version))
minor				:= $(word 2, $(version))
build				:= $(word 3, $(version))
version				:= $(shell if [ "build" = "build" ]; then build=2; minor=0; major=0; else if [ "build" = "minor" ]; then build=0; minor=1; major=0; else if [ "build" = "major" ]; then build=0; minor=0; major=1; fi; fi; fi; echo $$major.$$minor.$$build)

compile_flags		:= -std=c++11 -DVERSION="\"$(major).$(minor).$(build)"\"
link_flags			:= -Xlinker -rpath=$(CURDIR)/$(bin_dir)/


$(info version is $(version))

ifeq ($(build_type), debug)
	project			:= $(addsuffix _debug, $(project))
	obj_dir			:= obj/Debug/
	lib_files		:= $(addsuffix _debug, $(lib_files))
	compile_flags	:= $(addprefix -DDEBUG , $(compile_flags))
else
	obj_dir			:= obj/Release/
endif

export build_type bin_dir src_dir obj_dir

project				:= $(addprefix $(obj_dir),$(project))

src_files			:= $(shell find $(src_dir) -maxdepth 1 -name '*.cpp')
obj_files			:= $(src_files:.cpp=.o)
obj_files			:= $(subst $(src_dir),$(obj_dir), $(obj_files))

.PHONY: clean install

all: prepare build_modules $(project)

build_modules:
	for module in $(modules); \
	do \
		$(MAKE) -C $(src_dir)$$module; \
	done

install_modules:
	for module in $(modules); \
	do \
		if [ "$(build_type)" = "debug" ]; then \
			cp $(obj_dir)/$$module/*_debug.so $(bin_dir); \
		else \
			cp $(obj_dir)/$$module/*$$module.so $(bin_dir); \
		fi \
	done

prepare:
	mkdir -p $(obj_dir)
	mkdir -p $(bin_dir)

$(project): $(obj_files)
	g++ -o $@ $< $(addprefix -L,$(addprefix $(obj_dir),$(modules))) $(addprefix -l,$(lib_files)) $(link_flags)

$(obj_dir)%.o:$(src_dir)%.cpp
	g++ $(compile_flags) -o $@ -c $< -I$(src_dir) 

clean:
	rm -rf obj bin

install: all install_modules
	echo $(version) > $(src_dir)version
	cp $(project) $(bin_dir)
