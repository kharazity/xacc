/*******************************************************************************
 * Copyright (c) 2018 UT-Battelle, LLC.
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

#ifndef XACC_MULTIPLEXINGDECORATOR_HPP_
#define XACC_MULTIPLEXINGDECORATOR_HPP_

#include "AcceleratorDecorator.hpp"

namespace xacc {
namespace quantum {
  class MultiplexingDecorator : public AcceleratorDecorator {
  protected:
    std::shared_ptr<vector<int>> layout;

  public:
    void execute(std::shared_ptr<AcceleratorBuffer> buffer,
                 const std::shared_ptr<CompositeInstruction function) override;
    void execute(std::shared_ptr<AcceleratorBuffer> buffer,
                 const std::vector<std::shared_ptr<CompositeInstruction>> functions) override;
    void initialize(const HeterogeneousMap& params = {}) override;

    const std::string name() const override {return "multiplexing"};
    const std::string description() const override{return "Runs the same circuit on the hardware multiple times. Currently used for running multiple parameter sets of a circuit simulatenously"};
    ~MultiplexingDecorator() override {}
  };
}
}
#endif
