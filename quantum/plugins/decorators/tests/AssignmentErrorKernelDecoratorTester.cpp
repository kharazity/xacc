/*******************************************************************************
 * Copyright (c) 2019 UT-Battelle, LLC.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * and Eclipse Distribution License v1.0 which accompanies this
 * distribution. The Eclipse Public License is available at
 * http://www.eclipse.org/legal/epl-v10.html and the Eclipse Distribution
 *License is available at https://eclipse.org/org/documents/edl-v10.php
 *
 * Contributors:
 *   Alexander J. McCaskey - initial API and implementation
 *******************************************************************************/

#include "AcceleratorDecorator.hpp"
#include "Instruction.hpp"
#include "xacc.hpp"
#include <gtest/gtest.h>
#include <memory>
#include "xacc_service.hpp"

using namespace xacc;


TEST(AssignmentErrorKernelDecoratorTest, checkBasic){

  xacc::Initialize();
  xacc::external::load_external_language_plugins();
  auto accelerator = xacc::getAccelerator("aer", {std::make_pair("shots",2048), std::make_pair("backend", "ibmq_johannesburg"),
                                                  std::make_pair("readout_error", true), std::make_pair("gate_error", true)});
 
  int num_qubits = 2;
  auto buffer = xacc::qalloc(num_qubits);

  auto compiler = xacc::getService<xacc::Compiler>("xasm");
  const std::string src = R"src(__qpu__ void foo(qbit q) {
       H(q[0]);
       CNOT(q[1],q[0]);
       Measure(q[0]);
       Measure(q[1]);
       })src";
  auto ir = compiler->compile(src, accelerator);
  auto f = ir->getComposite("foo");
  auto decorator = xacc::getService<AcceleratorDecorator>("assignment-error-kernel");
  decorator->initialize({std::make_pair("gen-kernel", true)});
  decorator->setDecorated(accelerator);
  decorator->execute(buffer, f);
  xacc::external::unload_external_language_plugins();
  xacc::Finalize();
}

int main(int argc, char **argv){
  int ret = 0;
  ::testing::InitGoogleTest(&argc, argv);
  ret = RUN_ALL_TESTS();
  return ret;
}
