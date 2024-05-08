qemu-system-x86_64                                                     \
    -M q35,accel=tcg,kernel-irqchip=split -cpu qemu64,rdtscp                          \
    -m 4G -smp 4 -display none                                            \
    -kernel ${HOME}/cosim/Downloads/ubuntu-20.04-server-cloudimg-amd64-vmlinuz-generic               \
    -append "root=/dev/sda1 rootwait console=tty1 console=ttyS0 intel_iommu=on"       \
    -initrd ${HOME}/cosim/Downloads/ubuntu-20.04-server-cloudimg-amd64-initrd-generic                \
    -serial mon:stdio -device intel-iommu,intremap=on,device-iotlb=on                 \
    -drive file=/home/long/cosim/Downloads/ubuntu-20.04-server-cloudimg-amd64.img                       \
    -drive file=/home/long/cosim/Downloads/user-data.img,format=raw                                     \
    -device ioh3420,id=rootport1,slot=1                                               \
    -device remote-port-pci-adaptor,bus=rootport1,id=rp0                              \
    -machine-path /home/long/cosim/buildroot/handles                                                   \
    -device virtio-net-pci,netdev=net0 -netdev type=user,id=net0                      \
    -device remote-port-pcie-root-port,id=rprootport,slot=0,rp-adaptor0=rp,rp-chan0=0
    
./pcie/versal/multi-qemu-pcie-demo unix:${HOME}/cosim/buildroot/handles/qemu-rport-_machine_peripheral_rp0_rp 10000 unix:${HOME}/cosim/buildroot/handles/1/qemu-rport-_machine_peripheral_rp0_rp

qemu-system-x86_64                                        \
    -M q35,accel=tcg,kernel-irqchip=split -cpu qemu64,rdtscp                          \
    -m 4G -smp 4 -display none                                            \
    -kernel ${HOME}/cosim/Downloads1/ubuntu-20.04-server-cloudimg-amd64-vmlinuz-generic               \
    -append "root=/dev/sda1 rootwait console=tty1 console=ttyS0 intel_iommu=on"       \
    -initrd ${HOME}/cosim/Downloads1/ubuntu-20.04-server-cloudimg-amd64-initrd-generic                \
    -serial mon:stdio -device intel-iommu,intremap=on,device-iotlb=on                 \
    -drive file=/home/long/cosim/Downloads1/ubuntu-20.04-server-cloudimg-amd64.img                       \
    -drive file=/home/long/cosim/Downloads1/user-data.img,format=raw                                     \
    -device ioh3420,id=rootport1,slot=1                                               \
    -device remote-port-pci-adaptor,bus=rootport1,id=rp0                              \
    -machine-path /home/long/cosim/buildroot/handles/1/                                                   \
    -device virtio-net-pci,netdev=net0 -netdev type=user,id=net0                      \
    -device remote-port-pcie-root-port,id=rprootport,slot=0,rp-adaptor0=rp,rp-chan0=0
