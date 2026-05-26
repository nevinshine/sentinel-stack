# Sentinel-Stack Root Makefile

BIN_DIR := $(PWD)/bin

.PHONY: all clean telos-runtime hyperion-xdp telos-lang

all: $(BIN_DIR) telos-runtime hyperion-xdp telos-lang
	@echo ""
	@echo "========================================"
	@echo " Sentinel-Stack v1.0-rc1 Build Complete "
	@echo "========================================"
	@echo "Artifacts are in $(BIN_DIR)/"
	@ls -la $(BIN_DIR)/

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

telos-runtime: $(BIN_DIR)
	@echo "Building Telos Runtime..."
	$(MAKE) -C telos-runtime all
	cp telos-runtime/bin/bpf_lsm.o $(BIN_DIR)/
	cp telos-runtime/bin/telos_daemon $(BIN_DIR)/

hyperion-xdp: $(BIN_DIR)
	@echo "Building Hyperion XDP..."
	$(MAKE) -C hyperion-xdp build
	cp hyperion-xdp/bin/hyperion_ctrl $(BIN_DIR)/

telos-lang: $(BIN_DIR)
	@echo "Building Telos Lang..."
	cd telos-lang/telosc && cargo build --release
	cp telos-lang/telosc/target/release/telosc $(BIN_DIR)/ || cp telos-lang/telosc/target/release/telos_lang $(BIN_DIR)/

clean:
	rm -rf $(BIN_DIR)
	$(MAKE) -C telos-runtime clean
	$(MAKE) -C hyperion-xdp clean
	cd telos-lang/telosc && cargo clean
