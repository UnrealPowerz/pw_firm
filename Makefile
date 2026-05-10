# Pokewalker build system — dual-mode (host/container)
# Detects Docker environment via /opt/H8

CORES = $(shell nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)

ifneq ($(wildcard /opt/H8),)
# ===================================================================
# CONTAINER MODE — full compile/assemble/link pipeline
# ===================================================================

CC      = h8-6_2_2-ch38
AS      = h8-6_2_2-asm38
OC      = h8300-elf-objcopy
OD      = h8300-elf-objdump

INCLUDES = -include=src,include,build/gen,/opt/H8/6_2_2/include
# Optimization for size and speed to fit in ROM
CFLAGS   = -cpu=300HN -stack=medium -lang=c -outcode=sjis -nolist -chgincpath -nologo -Code=Asmcode -optimize=1 -cmncode $(INCLUDES)

ASFLAGS  = -cpu=300HN -nologo


C_SOURCES := $(shell find src -name "*.c")
S_SOURCES :=

define to_asm
build/obj_$(subst /,_,$(1:%.c=%.s))
endef

define to_obj
build/obj_$(subst /,_,$(1:%.c=%.o))
endef

C_OBJS := $(foreach src,$(C_SOURCES),$(call to_obj,$(src)))
S_OBJS := $(patsubst src/%.s,build/obj_%.o,$(S_SOURCES))
OBJS   := $(S_OBJS) $(C_OBJS)

.PHONY: all clean headers

all: headers $(OBJS) build/linked.mot build/linked.bin build/linked.dis

headers:
	python3 scripts/generate_headers.py

# Step 1: C → flattened assembly
define make-c-to-asm-rule
$(call to_asm,$1): $1
	@mkdir -p build
	$(CC) $(CFLAGS) $(CFLAGS_$(notdir $1)) -object="$$@" $$<
endef

$(foreach src,$(C_SOURCES),$(eval $(call make-c-to-asm-rule,$(src))))

# Step 2: assembly → object
define make-asm-to-obj-rule
$(call to_obj,$1): $(call to_asm,$1)
	@mkdir -p build
	$(AS) $(ASFLAGS) -object="$$@" "$$<"
endef

$(foreach src,$(C_SOURCES),$(eval $(call make-asm-to-obj-rule,$(src))))

# Assemble hand-written .s files
build/obj_%.o: src/%.s
	@mkdir -p build
	$(AS) $(ASFLAGS) -object="$@" "$<"

# Generate build/link.sub with CRLF line endings (Wine/Windows linker)
build/link.sub: $(OBJS)
	@echo "Generating build/link.sub..."
	@rm -f build/link.sub.tmp
	@for obj in $(OBJS); do \
		win_obj=$$(echo $$obj | tr '/' '\\'); \
		printf -- "-input=Z:\\work\\%s\r\n" "$$win_obj" >> build/link.sub.tmp; \
	done
	@printf -- "-library=Z:\\\\work\\\\build\\\\lib3hn.lib\r\n" >> build/link.sub.tmp
	@printf -- "-rom=D=R\r\n" >> build/link.sub.tmp
	@printf -- "-optimize=symbol_delete,short_format\r\n" >> build/link.sub.tmp
	@printf -- "-start=PIntPRG,P,PP,C,D\$$DSEC,D\$$BSEC,C\$$DSEC,C\$$BSEC,D/5E\r\n" >> build/link.sub.tmp
	@printf -- "-start=CP/BB0E\r\n" >> build/link.sub.tmp

	@printf -- "-start=B,R,S/F780\r\n"           >> build/link.sub.tmp
	@printf -- "-form=stype\r\n"             >> build/link.sub.tmp
	@printf -- "-list=build\\linked.map\r\n" >> build/link.sub.tmp
	@printf -- "-output=build\\linked.mot\r\n" >> build/link.sub.tmp
	@printf -- "-exit\r\n"                   >> build/link.sub.tmp
	@mv build/link.sub.tmp build/link.sub

build/lib3hn.lib:
	@mkdir -p build
	wine cmd /c Z:\\work\\run_lbg38.bat

build/linked.mot: build/link.sub build/lib3hn.lib $(OBJS)
	wine cmd /c "set PATH=Z:\\opt\\H8\\6_1_3\\bin;Z:\\opt\\H8\\6_2_2\\bin;%%PATH%% && optlnk.exe -subcommand=Z:\\work\\build\\link.sub"

build/linked.bin: build/linked.mot
	$(OC) -I srec -O binary build/linked.mot build/linked.bin

build/linked.dis: build/linked.bin
	$(OD) -m h8300 -D -b binary --adjust-vma=0x0 build/linked.bin > build/linked.dis

clean:
	find build -mindepth 1 ! -name 'lib3hn.lib' -delete 2>/dev/null; true

else
# ===================================================================
# HOST MODE — launches Docker for full build; ./h8cc for single files
# ===================================================================

.PHONY: all clean headers asm sdk-headers

# Default: generate headers then run full build inside Docker
all: headers
	@mkdir -p build
	docker run --rm --network=none -v .:/work h8 bash -c "make -j$(CORES)"

headers:
	python3 scripts/generate_headers.py

asm:
	@test -n "$(SRC)" || (echo "Error: SRC is not set. Usage: make asm SRC=src/path/to/file.c" && false)
	@mkdir -p $(dir build/asm/$(patsubst src/%.c,%.s,$(SRC)))
	./h8cc -include=src,include,build/gen,/opt/H8/6_2_2/include -object="build/asm/$(patsubst src/%.c,%.s,$(SRC))" $(SRC)

sdk-headers:
	@mkdir -p h8inc
	docker run --rm h8 tar -cf - -C /opt/H8/6_2_2/include . | tar -xf - -C h8inc/
	@echo "H8 SDK headers extracted to h8inc/"

clean:
	find build -mindepth 1 ! -name 'lib3hn.lib' -delete 2>/dev/null; true

endif
