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
#include "MultiplexingDecorator.hpp"
#include "InstructionIterator.hpp"
#include "Utils.hpp"
#include "xacc.hpp"
#include <fstream>
#include <set>
#include <Eigen/Dense>

namespace xacc{
namespace quantum{
  void MultiplexingDecorator::initialize(const HeterogeneousMap& params){
    if(params.keyExists<std::shared_ptr<std::vector<int>>>("layout")){
      layout = params.get<std::shared_ptr<std::vector<int>>>("layout");
    }
    else{
      xacc::error("No layout given, must give layout");
    }
    return;
  }

  void MultiplexingDecorator::execute(const std::shared_ptr<AcceleratorBuffer> buffer,
                                      const std::shared_ptr<CompositeInstruction> function){
    std::shared_ptr<CompositeInstruction> multiplex_func;
    return;
  }

  void MultiplexingDecorator::execute(const std::shared_ptr<AcceleratorBuffer> buffer,
                                      const std::vector<std::shared_ptr<CompositeInstruction>> functions){
    std::shared_ptr<AcceleratorBuffer> MultiBuffer = xacc::qalloc(2*(buffer->size()));


    std::shared_ptr<CompositeInstruction> Multiplexed;
    for(auto &function : functions){
      for(auto &x:function->getInstructions()){
        std::cout<<x<<std::endl;
        Multiplexed->addInstruction(x);
      }
    }


    return;
  }

}//quantum
}//namespace
