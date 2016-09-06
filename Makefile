# ===================================
# Changeable variables

project 			:= AccumServer
build_type			:= debug

bin_dir				:= bin

#version major.minor.build
inc_version			:= none

modules				:= server

# ===================================

src_dir				:= src/
obj_dir				:= obj/$(build_type)/

lib_files			:= $(modules)

version				:= $(shell gawk 'BEGIN{FS = ".";}\
					                {\
										major = $$1; \
										minor = $$2; \
										build = $$3; \
					                    if ("'$(inc_version)'" == "build") \
					                    { \
					                        build = $$3 + 1; \
					                    } \
					                    else if ("'$(inc_version)'" == "minor") \
					                    { \
					                        minor = $$2 + 1; \
					                        build = 0; \
					                    } \
					                    else if ("'$(inc_version)'" == "major") \
					                    { \
					                        major = $$1 + 1; \
					                        minor = 0; \
					                        build = 0; \
					                    } \
					                    printf("%d.%d.%d", major, minor, build); \
					                }' $(src_dir)version)

compile_flags		:= -std=c++11 -DVERSION="\"$(version)"\"
link_flags			:= -Xlinker -rpath=$(CURDIR)/$(bin_dir)/

ifeq ($(build_type), debug)
	project			:= $(addsuffix _debug, $(project))
	lib_files		:= $(addsuffix _debug, $(lib_files))
	compile_flags	:= $(addprefix -DDEBUG , $(compile_flags))
endif

export build_type bin_dir obj_dir

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
	if [ "$(inc_version)" != "none" ]; then \
		rm -f $(obj_dir)/main.o; \
	fi

$(project): $(obj_files)
	g++ -o $@ $< $(addprefix -L,$(addprefix $(obj_dir),$(modules))) $(addprefix -l,$(lib_files)) $(link_flags)

$(obj_dir)%.o:$(src_dir)%.cpp
	g++ $(compile_flags) -o $@ -c $< -I$(src_dir) 

clean:
	rm -rf obj bin

install: all install_modules
	echo $(version) > $(src_dir)version
	cp $(project) $(bin_dir)
