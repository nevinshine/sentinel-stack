# Sentinel-Stack Root Makefile

BIN_DIR := $(PWD)/bin

.PHONY: all clean telos-runtime hyperion-xdp telos-lang sentinel-cc

all: $(BIN_DIR) telos-runtime hyperion-xdp telos-lang sentinel-cc
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
	cp telos-runtime/bin/telos_daemon $(BIN_DIR)/

hyperion-xdp: $(BIN_DIR)
	@echo "Building Hyperion XDP..."
	$(MAKE) -C hyperion-xdp build
	cp hyperion-xdp/bin/hyperion_ctrl $(BIN_DIR)/

telos-lang: $(BIN_DIR)
	@echo "Building Telos Lang..."
	cd telos-lang/telosc && cargo build --release
	cp telos-lang/telosc/target/release/telosc $(BIN_DIR)/ || cp telos-lang/telosc/target/release/telos_lang $(BIN_DIR)/

sentinel-cc: $(BIN_DIR)
	@echo "Building Sentinel-CC..."
	$(MAKE) -C sentinel-cc all
	cp sentinel-cc/loader $(BIN_DIR)/sentinel-loader || true
	cp sentinel-cc/scc $(BIN_DIR)/ || true
	cp sentinel-cc/sentinel-dump $(BIN_DIR)/ || true
	cp sentinel-cc/sentinel-sign $(BIN_DIR)/ || true
	cp sentinel-cc/sentinel-tui $(BIN_DIR)/ || true

clean:
	rm -rf $(BIN_DIR)
	rm -rf release_payload
	rm -f sentinel-release.tar.gz
	$(MAKE) -C telos-runtime clean
	$(MAKE) -C hyperion-xdp clean
	$(MAKE) -C sentinel-cc clean
	cd telos-lang/telosc && cargo clean

.PHONY: release
release: clean
	@echo "Building Sentinel Stack Release Payload..."
	mkdir -p release_payload/x86_64 release_payload/arm64 release_payload/src/mei release_payload/src/vmi
	
	@echo "Cross-compiling for x86_64..."
	GOARCH=amd64 $(MAKE) -C telos-runtime all
	GOARCH=amd64 $(MAKE) -C hyperion-xdp build
	cp telos-runtime/bin/telos_daemon release_payload/x86_64/
	cp hyperion-xdp/bin/hyperion_ctrl release_payload/x86_64/
	cp telos-runtime/bin/sentinelctl release_payload/x86_64/
	
	@echo "Cross-compiling for arm64..."
	GOARCH=arm64 $(MAKE) -C telos-runtime all
	GOARCH=arm64 $(MAKE) -C hyperion-xdp build
	cp telos-runtime/bin/telos_daemon release_payload/arm64/
	cp hyperion-xdp/bin/hyperion_ctrl release_payload/arm64/
	cp telos-runtime/bin/sentinelctl release_payload/arm64/
	
	@echo "Packaging raw C source components..."
	cp -r sentinel-smm/src/mei/* release_payload/src/mei/
	cp -r sentinel-vmi/* release_payload/src/vmi/
	
	@echo "Packaging Red Team Exploit Suite..."
	mkdir -p release_payload/red_team
	make -C tests/red_team
	cp tests/red_team/sentinel_strike release_payload/red_team/
	cp tests/red_team/sentinel_strike_ring0 release_payload/red_team/
	
	@echo "Creating deployment tarball..."
	tar -czf sentinel-release.tar.gz -C release_payload .
	@echo "✓ sentinel-release.tar.gz created."

.PHONY: lab-up
lab-up:
	@echo "Spinning up Vagrant Digital Twin..."
	vagrant up

.PHONY: lab-deploy
lab-deploy: release
	@echo "Deploying to Vagrant Digital Twin via Ansible..."
	ansible-playbook -i ansible/vagrant.yml ansible/deploy.yml
