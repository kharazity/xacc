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
#include "xacc.hpp"
#include <gtest/gtest.h>
#include <memory>
#include "xacc_service.hpp"

using namespace xacc;


TEST(AssignmentErrorKernelDecoratorTest, checkBasic){


  xacc::set_verbose(true);
  if (xacc::hasAccelerator("aer")){
    auto accelerator = xacc::getAccelerator("aer", {std::make_pair("shots", 2048),
                                                    std::make_pair("backend", "ibmq_johannesburg"),
                                                    std::make_pair("readout_error", true),
                                                    std::make_pair("gate_error", true),
                                                    std::make_pair("thermal_relaxation", true)});

    int num_qubits = 2;

    auto compiler = xacc::getService<xacc::Compiler>("xasm");
    xacc::qasm(R"(
.compiler xasm
.circuit bell
.qbit q
H(q[0]);
CX(q[0], q[1]);
Measure(q[0]);
Measure(q[1]);
)");
    auto bell = xacc::getCompiled("bell");
    auto decBuffer = xacc::qalloc(num_qubits);
    auto decorator = xacc::getService<AcceleratorDecorator>("assignment-error-kernel");
    decorator->initialize({std::make_pair("gen-kernel", true)});
    decorator->setDecorated(accelerator);
    decorator->execute(decBuffer, bell);
    std::cout<<"DECORATED ACCELERATOR"<<std::endl;
    decBuffer->print();
  }
  else{
    std::cout<<"you do not have aer accelerator installed, please install qiskit if you wish to run this test"<<std::endl;
  }

  }

int main(int argc, char **argv){
  int ret = 0;
  xacc::Initialize();
  xacc::external::load_external_language_plugins();
  ::testing::InitGoogleTest(&argc, argv);
  ret = RUN_ALL_TESTS();
  xacc::external::unload_external_language_plugins();
  xacc::Finalize();
  return ret;
}
