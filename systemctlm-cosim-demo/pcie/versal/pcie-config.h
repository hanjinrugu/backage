/*
 * TLM-2.0 PCIe root port.
 *
 * Copyright (c) 2022 AMD Inc.
 * Written by Francisco Iglesias.
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
 *
 * [1] PCI Express Base Specification Revision 5.0 Version 1.0
 *
 */
#ifndef SOC_PCIE_CORE_PCIE_ROOT_PORT_H__
#define SOC_PCIE_CORE_PCIE_ROOT_PORT_H__

#include <list>
#include "soc/pci/core/pci-device-base.h"
#include "tlm-bridges/tlm2tlp-bridge.h"

class pcie_root_port : public pci_device_base
{
public:
	//
	// TLP connections out from the pcie root port
	//
	tlm_utils::simple_initiator_socket<pcie_root_port> init_socket;
	tlm_utils::simple_target_socket<pcie_root_port> tgt_socket;
	long pdata[1024];
	pcie_root_port(sc_core::sc_module_name name):
		pci_device_base(name, 1, 0),

		init_socket("init-socket"),
		tgt_socket("tgt-socket"),

		cfg_init_socket("cfg-init-socket"),
		mem_init_socket("mem-init-socket"),
		io_init_socket("io-init-socket"),
		msg_init_socket("msg-init-socket"),
		brdg_dma_tgt_socket("brdg-dma-tgt-socket"),

		brdg_fwd_tgt_socket("brdg-fwd-tgt-socket"),
		brdg_fwd_init_socket("brdg-fwd-init-socket"),

		tlm2tlp_brdg("tlm2tlp-bridge")
	{
		cfg_init_socket.bind(tlm2tlp_brdg.cfg_tgt_socket);
		mem_init_socket.bind(tlm2tlp_brdg.mem_tgt_socket);
		io_init_socket.bind(tlm2tlp_brdg.io_tgt_socket);
		msg_init_socket.bind(tlm2tlp_brdg.msg_tgt_socket);

		//
		// Setup DMA fwd path (tlm2tlp_brdg -> upstream to host)
		//
		tlm2tlp_brdg.dma_init_socket.bind(brdg_dma_tgt_socket);
		brdg_dma_tgt_socket.register_b_transport(
			this, &pcie_root_port::brdg_dma_b_transport);

		//
		// Setup TLP Tx fwd path (tlm2tlp_brdg -> pcie-root-port)
		//
		tlm2tlp_brdg.init_socket.bind(brdg_fwd_tgt_socket);
		brdg_fwd_tgt_socket.register_b_transport(
			this, &pcie_root_port::brdg_fwd_tx_b_transport);

		//
		// Connect TLP Rx fwd path (pcie-root-port -> tlm2tlp_brdg)
		//
		brdg_fwd_init_socket.bind(tlm2tlp_brdg.tgt_socket);
		tgt_socket.register_b_transport(
			this, &pcie_root_port::brdg_fwd_rx_b_transport);
	}

private:
	//
	// Receive from Remote Port
	//
	void config_b_transport(tlm::tlm_generic_payload& trans,
					sc_time& delay)
	{
		cfg_init_socket->b_transport(trans, delay);
	}

	void bar_b_transport(int bar_nr, tlm::tlm_generic_payload& trans,
			     sc_time& delay)
	{
	tlm::tlm_command cmd = trans.get_command();
	sc_dt::uint64 addr = trans.get_address();
	unsigned char *data = trans.get_data_ptr();
	unsigned int len = trans.get_data_length();
	unsigned char *byt = trans.get_byte_enable_ptr();
	unsigned int wid = trans.get_streaming_width();
	long v = 0;
	long w = 0;
	if (byt != 0) {
		trans.set_response_status(tlm::TLM_BYTE_ENABLE_ERROR_RESPONSE);
		return;
	}

	if (len > 8 || wid < len) {
		trans.set_response_status(tlm::TLM_BURST_ERROR_RESPONSE);
		return;
	}

	// Pretend this is slow!
	delay += sc_time(1, SC_US);

	if (trans.get_command() == tlm::TLM_READ_COMMAND) {
		//pdata = 0x1234;
		v = pdata[(addr & 0x0f)>>2];
		memcpy(data, &v, len);

	} else if (cmd == tlm::TLM_WRITE_COMMAND) {
		memcpy(&w, data, len);
		pdata[(addr & 0x0f)>>2] = (int)w;
	}
	trans.set_response_status(tlm::TLM_OK_RESPONSE);
	}

	//
	// Forward received TLPs to the tlm2tlp_bridge
	//
	void brdg_fwd_rx_b_transport(tlm::tlm_generic_payload& trans,
					sc_time& delay)
	{
		brdg_fwd_init_socket->b_transport(trans, delay);
	}

	//
	// Forward (transmit) received TLPs from the tlm2tlp_bridge
	//
	void brdg_fwd_tx_b_transport(tlm::tlm_generic_payload& trans,
					sc_time& delay)
	{
		init_socket->b_transport(trans, delay);
	}

	//
	// Forward DMA requests received from tlm2tlp_bridge
	//
	void brdg_dma_b_transport(tlm::tlm_generic_payload& trans,
					sc_time& delay)
	{
		dma->b_transport(trans, delay);
	}
	//
	// Wrap the tlm2tlp-bridge in this class
	//
	// Internal TLM connections (input to the tlm2tlp_bridge)
	//
	tlm_utils::simple_initiator_socket<pcie_root_port> cfg_init_socket;
	tlm_utils::simple_initiator_socket<pcie_root_port> mem_init_socket;
	tlm_utils::simple_initiator_socket<pcie_root_port> io_init_socket;
	tlm_utils::simple_initiator_socket<pcie_root_port> msg_init_socket;
	tlm_utils::simple_target_socket<pcie_root_port> brdg_dma_tgt_socket;
	//
	// Internal TLP sockets forwarding to/from the tlm2tlp_bridge
	//
	tlm_utils::simple_target_socket<pcie_root_port> brdg_fwd_tgt_socket;
	tlm_utils::simple_initiator_socket<pcie_root_port> brdg_fwd_init_socket;

	tlm2tlp_bridge tlm2tlp_brdg;
};

#endif
