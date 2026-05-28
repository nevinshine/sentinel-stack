Vagrant.configure("2") do |config|
  # Use generic boxes that support libvirt
  
  config.vm.define "intelhost" do |intelhost|
    intelhost.vm.box = "generic/ubuntu2204"
    intelhost.vm.hostname = "intelhost"
    intelhost.vm.network "private_network", ip: "192.168.121.10"
    
    intelhost.vm.provider :libvirt do |libvirt|
      libvirt.qemu_use_session = false
      libvirt.storage_pool_name = "images"
      libvirt.memory = 4096
      libvirt.cpus = 4
    end

    # Arm the BPF LSM automatically if it's missing
    intelhost.vm.provision "shell", inline: <<-SHELL
      if ! grep -q "bpf" /sys/kernel/security/lsm; then
        echo "[*] BPF LSM not armed. Patching GRUB..."
        # Safely append to GRUB_CMDLINE_LINUX
        sed -i 's/^GRUB_CMDLINE_LINUX="\\(.*\\)"/GRUB_CMDLINE_LINUX="\\1 lsm=lockdown,capability,yama,apparmor,bpf"/' /etc/default/grub
        # Fallback if GRUB_CMDLINE_LINUX was empty or just ""
        sed -i 's/^GRUB_CMDLINE_LINUX=""/GRUB_CMDLINE_LINUX="lsm=lockdown,capability,yama,apparmor,bpf"/' /etc/default/grub
        update-grub
      else
        echo "[+] BPF LSM is already armed."
      fi
    SHELL
    
    # (Reboot manually with 'vagrant reload intelhost' to apply if this patched GRUB)
  end

  # We are commenting out the ARM64 emulation node for this specific Ring 0 test
  # since Vagrant versions < 2.3 do not support the 'box_architecture' flag natively,
  # and the Ring 0 tests are strictly targeting the x86 intelhost VM anyway.
  #
  # config.vm.define "raspberrypi" do |raspberrypi|
  #   raspberrypi.vm.box = "generic/ubuntu2204"
  #   raspberrypi.vm.box_architecture = "aarch64"
  #   raspberrypi.vm.hostname = "raspberrypi"
  #   raspberrypi.vm.network "private_network", ip: "192.168.121.20"
  #   
  #   raspberrypi.vm.provider :libvirt do |libvirt|
  #     libvirt.memory = 2048
  #     libvirt.cpus = 2
  #     # Force system emulation for ARM64 on x86 host
  #     libvirt.driver = "qemu"
  #     libvirt.machine_type = "virt"
  #     libvirt.cpu_mode = "custom"
  #     libvirt.cpu_model = "cortex-a72"
  #   end
  # end
end
