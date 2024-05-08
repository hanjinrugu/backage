/*
 * Copyright (c) 2019 Xilinx Inc.
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
 */

#include <sstream>
#include <string>
#include <vector>
#include <array>

#define SC_INCLUDE_DYNAMIC_PROCESSES

#include "systemc"
using namespace sc_core;
using namespace sc_dt;
using namespace std;

#include "tlm.h"
#include "tlm_utils/simple_initiator_socket.h"
#include "tlm_utils/simple_target_socket.h"

#include "traffic-generators/tg-tlm.h"
#include "checkers/pc-ace.h"
#include "checkers/config-ace.h"
#include "test-modules/memory.h"
#include "test-modules/signals-ace.h"
#include "siggen-ace.h"

#define AXI_ADDR_WIDTH 64
#define AXI_DATA_WIDTH 64

typedef ACESignals<
	AXI_ADDR_WIDTH,	// ADDR_WIDTH
	AXI_DATA_WIDTH	// DATA_WIDTH
> ACESignals__;

SIGGEN_TESTSUITE(TestSuite)
{
	SIGGEN_TESTSUITE_CTOR(TestSuite)
	{}

	void run_tests()
	{
		SetMessageType(AXI_RESET_ERROR);

		wait(clk.posedge_event());

		TESTCASE(test_reset);

		TESTCASE_NEG(test_reset_with_asserted_arvalid);
		TESTCASE_NEG(test_reset_with_asserted_rvalid);
		TESTCASE_NEG(test_reset_with_asserted_awvalid);
		TESTCASE_NEG(test_reset_with_asserted_wvalid);
		TESTCASE_NEG(test_reset_with_asserted_bvalid);

		TESTCASE_NEG(test_reset_with_asserted_rack);
		TESTCASE_NEG(test_reset_with_asserted_wack);

		TESTCASE_NEG(test_reset_with_asserted_acvalid);
		TESTCASE_NEG(test_reset_with_asserted_crvalid);
		TESTCASE_NEG(test_reset_with_asserted_cdvalid);
	}

	void test_reset()
	{
		// All valid signals false
		arvalid.write(false);
		rvalid.write(false);
		awvalid.write(false);
		wvalid.write(false);
		bvalid.write(false);

		resetn.write(false);
		wait(clk.posedge_event());

		resetn.write(true);
		wait(clk.posedge_event());
	}

#define GEN_RESET_WITH_ASSERTED(valid)		\
void test_reset_with_asserted_ ## valid ()	\
{						\
	arvalid.write(false);			\
	rvalid.write(false);			\
	awvalid.write(false);			\
	wvalid.write(false);			\
	bvalid.write(false);			\
						\
	valid.write(true);			\
						\
	resetn.write(false);			\
	wait(clk.posedge_event());		\
						\
	resetn.write(true);			\
	wait(clk.posedge_event());		\
	wait(clk.posedge_event());		\
}

	GEN_RESET_WITH_ASSERTED(arvalid)
	GEN_RESET_WITH_ASSERTED(rvalid)
	GEN_RESET_WITH_ASSERTED(awvalid)
	GEN_RESET_WITH_ASSERTED(wvalid)
	GEN_RESET_WITH_ASSERTED(bvalid)

	GEN_RESET_WITH_ASSERTED(rack)
	GEN_RESET_WITH_ASSERTED(wack)

	GEN_RESET_WITH_ASSERTED(acvalid)
	GEN_RESET_WITH_ASSERTED(crvalid)
	GEN_RESET_WITH_ASSERTED(cdvalid)

};

SIGGEN_RUN(TestSuite)

ACEPCConfig checker_config()
{
	ACEPCConfig cfg;

	cfg.check_ace_reset();

	return cfg;
}

int sc_main(int argc, char *argv[])
{
	ACEProtocolChecker<AXI_ADDR_WIDTH, AXI_DATA_WIDTH>
			checker("checker", checker_config());

	ACESignals__ signals("ace_signals");

	SignalGen<AXI_ADDR_WIDTH, AXI_DATA_WIDTH> siggen("sig_gen");

	sc_clock clk("clk", sc_time(20, SC_US));
	sc_signal<bool> resetn("resetn", true);

	// Connect clk
	checker.clk(clk);
	siggen.clk(clk);

	// Connect reset
	checker.resetn(resetn);
	siggen.resetn(resetn);

	// Connect signals
	signals.connect(checker);
	signals.connect(siggen);

	sc_trace_file *trace_fp = sc_create_vcd_trace_file(argv[0]);

	sc_trace(trace_fp, siggen.clk, siggen.clk.name());
	sc_trace(trace_fp, siggen.resetn, siggen.resetn.name());
	signals.Trace(trace_fp);

	// Run
	sc_start(100, SC_MS);

	sc_stop();

	if (trace_fp) {
		sc_close_vcd_trace_file(trace_fp);
	}

	return 0;
}