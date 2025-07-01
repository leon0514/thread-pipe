# ---------------------------------------------------------------------------- #
#         Original Makefile - with a dedicated 'so' build target
# ---------------------------------------------------------------------------- #

cc        := g++
so_name   := trt_pipeline.so
pro_name  := pro
workdir   := workspace
srcdir    := src
objdir    := objs
stdcpp    := c++17

# Compilation and linking flags
cpp_compile_flags := -std=$(stdcpp) -Wall -g -fPIC -pthread -fsanitize=address -I$(srcdir)
link_flags        := -pthread -fsanitize=address
rpath_flags       := -Wl,-rpath='$$ORIGIN'

# Source file separation
cpp_srcs   := $(shell find $(srcdir) -name "*.cpp")
main_src   := $(srcdir)/main.cpp
lib_srcs   := $(filter-out $(main_src), $(cpp_srcs))

# Object file lists
main_obj   := $(main_src:$(srcdir)/%.cpp=$(objdir)/%.o)
lib_objs   := $(lib_srcs:$(srcdir)/%.cpp=$(objdir)/%.o)

# Dependency files (.mk)
cpp_mk     := $(patsubst $(srcdir)/%.cpp,$(objdir)/%.cpp.mk, $(cpp_srcs))

ifneq ($(MAKECMDGOALS), clean)
-include $(cpp_mk)
endif


# --- MODIFICATION: Add explicit targets for 'so', 'pro', and update 'all' ---
.PHONY: all so pro run clean

# Default target: builds both the library and the executable
all: so pro

# NEW: Target to build ONLY the shared library
so: $(workdir)/$(so_name)

# NEW: Target to build ONLY the executable (and its dependencies)
pro: $(workdir)/$(pro_name)

# 'run' target now clearly depends on the 'pro' target
run: pro
	@echo "--- Running Application from workspace ---"
	@cd $(workdir) && ./$(pro_name)


# --- Linking Rules ---

# Rule for the shared library (.so)
$(workdir)/$(so_name): $(lib_objs)
	@echo "Linking Shared Library: $@"
	@mkdir -p $(dir $@)
	@$(cc) -shared $^ -o $@ $(link_flags)

# Rule for the executable (pro)
$(workdir)/$(pro_name): $(main_obj) $(workdir)/$(so_name)
	@echo "Linking Executable: $@"
	@mkdir -p $(dir $@)
	@$(cc) $(main_obj) -o $@ $(link_flags) -L$(workdir) -l:$(so_name) $(rpath_flags)


# --- Compilation and Dependency Generation Rules (Unchanged) ---

$(objdir)/%.o: $(srcdir)/%.cpp
	@echo "Compiling CXX: $<"
	@mkdir -p $(dir $@)
	@$(cc) -c $< -o $@ $(cpp_compile_flags)

$(objdir)/%.cpp.mk: $(srcdir)/%.cpp
	@echo "Generating dependencies for: $<"
	@mkdir -p $(dir $@)
	@$(cc) -M $< -MF $@ -MT $(@:.cpp.mk=.cpp.o) $(cpp_compile_flags)


# --- Clean Rule (Unchanged) ---
clean:
	@echo "Cleaning up build artifacts..."
	@rm -rf $(objdir) $(workdir)