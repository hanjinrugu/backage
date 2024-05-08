/*
 * Copyright (C) 2022, Advanced Micro Devices, Inc.
 * Written by Fred Konrad
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#define SC_INCLUDE_DYNAMIC_PROCESSES

#include <stdint.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>

#include "systemc.h"
#include "tlm_utils/simple_initiator_socket.h"
#include "tlm_utils/simple_target_socket.h"
#include "tlm_utils/tlm_quantumkeeper.h"

#include "tlm-modules/pcie-controller.h"
#include "multi-qemu-pcie.h"
#include "soc/pci/xilinx/qdma.h"
#include "memory.h"

using namespace sc_core;
using namespace sc_dt;
using namespace std;

#include "trace.h"
#include "iconnect.h"
#include "debugdev.h"

#include "remote-port-tlm.h"
#include "remote-port-tlm-pci-ep.h"

#define PCI_VENDOR_ID_XILINX		(0x10ee)
#define PCI_DEVICE_ID_XILINX_EF100	(0xd004)
#define PCI_SUBSYSTEM_ID_XILINX_TEST	(0x000A)

#define PCI_CLASS_BASE_NETWORK_CONTROLLER     (0x02)

#ifndef PCI_EXP_LNKCAP_ASPM_L0S
#define PCI_EXP_LNKCAP_ASPM_L0S 0x00000400 /* ASPM L0s Support */
#endif

#define KiB (1024)
#define RAM_SIZE (4 * KiB)

#define NR_MMIO_BAR  6
#define NR_IRQ       NR_QDMA_IRQ



PhysFuncConfig getPhysFuncConfig()
{
	PhysFuncConfig cfg;
	PMCapability pmCap;
	PCIExpressCapability pcieCap;
	MSIXCapability msixCap;
	uint32_t bar_flags = PCI_BASE_ADDRESS_MEM_TYPE_64;
	uint32_t msixTableSz = NR_IRQ;
	uint32_t tableOffset = 0x100 | 4; // Table offset: 0, BIR: 4
	uint32_t pba = 0x140000 | 4; // BIR: 4
	uint32_t maxLinkWidth;

	cfg.SetPCIVendorID(PCI_VENDOR_ID_XILINX);
	cfg.SetPCIDeviceID(0x903F);

	cfg.SetPCIBAR0(256 * KiB, bar_flags);
	cfg.AddPCICapability(pmCap);

	maxLinkWidth = 1 << 4;
	pcieCap.SetDeviceCapabilities(PCI_EXP_DEVCAP_RBER);
	pcieCap.SetLinkCapabilities(PCI_EXP_LNKCAP_SLS_2_5GB | maxLinkWidth
				    | PCI_EXP_LNKCAP_ASPM_L0S);
	pcieCap.SetLinkStatus(PCI_EXP_LNKSTA_CLS_2_5GB | PCI_EXP_LNKSTA_NLW_X1);
	cfg.AddPCICapability(pcieCap);

	msixCap.SetMessageControl(msixTableSz-1);
	msixCap.SetTableOffsetBIR(tableOffset);
	msixCap.SetPendingBitArray(pba);
	cfg.AddPCICapability(msixCap);

	return cfg;
}
SC_MODULE(Top)
{
public:
	SC_HAS_PROCESS(Top);
	remoteport_tlm_pci_ep rp_pci_ep;
	pcie_root_port rootport;
	PCIeController pcie_ctlr;
	remoteport_tlm_pci_ep rp_pci_ep1;
	sc_signal<bool> rst;

	Top(sc_module_name name, const char *sk_descr, sc_time quantum, const char *sk_descr1) :
		sc_module(name),
		pcie_ctlr("pcie-ctlr", getPhysFuncConfig()),
		rp_pci_ep("rp-pci-ep", 0, 1, 0, sk_descr),
		rootport("rootport"),
		//pcie_ctlr1("pcie-ctlr1", getPhysFuncConfig()),
		rp_pci_ep1("rp-pci-ep1", 0, 1, 0, sk_descr1),		
        rst("rst")
	{
		m_qk.set_global_quantum(quantum);

		// Setup TLP sockets (host.rootport <-> pcie-ctlr)
		rootport.init_socket.bind(pcie_ctlr.tgt_socket);
		pcie_ctlr.init_socket.bind(rootport.tgt_socket);
		//rootport.multi_init_socket.bind(rootport1.multi_tgt_socket);
		//rootport1.multi_init_socket.bind(rootport.multi_tgt_socket);
		rp_pci_ep.rst(rst);
		rp_pci_ep.bind(rootport);

		//rootport.init_socket.bind(pcie_ctlr.tgt_socket);
		//pcie_ctlr.init_socket.bind(rootport.tgt_socket);

		
		rp_pci_ep1.rst(rst);
		rp_pci_ep1.bind(rootport);
		
		
		SC_THREAD(pull_reset);
	}

	void pull_reset(void) {
		rst.write(true);
		wait(1, SC_US);
		rst.write(false);
	}

private:
	tlm_utils::tlm_quantumkeeper m_qk;
};

void usage(void)
{
	cout << "tlm socket-path sync-quantum-ns" << endl;
}

int sc_main(int argc, char* argv[])
{
	Top *top;
	uint64_t sync_quantum;
	sc_trace_file *trace_fp = NULL;

	if (argc < 3) {
		sync_quantum = 10000;
	} else {
		sync_quantum = strtoull(argv[2], NULL, 10);
	}

	sc_set_time_resolution(1, SC_PS);

	top = new Top("top", argv[1],sc_time((double) sync_quantum, SC_NS),argv[3]);

	if (argc < 3) {
		sc_start(1, SC_PS);
		sc_stop();
		usage();
		exit(EXIT_FAILURE);
	}

	trace_fp = sc_create_vcd_trace_file("trace");
	if (trace_fp) {
		trace(trace_fp, *top, top->name());
	}

	sc_start();
	if (trace_fp) {
		sc_close_vcd_trace_file(trace_fp);
	}
	return 0;
}

