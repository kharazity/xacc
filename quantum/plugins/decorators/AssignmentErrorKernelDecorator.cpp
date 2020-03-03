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
 *   Tyler Kharazi - initial implementation
 *******************************************************************************/
#include "AssignmentErrorKernelDecorator.hpp"
#include "InstructionIterator.hpp"
#include "Utils.hpp"
#include "xacc.hpp"
#include <fstream>
#include <set>
#include <Eigen/Dense>

namespace xacc {
namespace quantum {void AssignmentErrorKernelDecorator::initialize(
    const HeterogeneousMap &params) {

  if (params.keyExists<bool>("gen-kernel")) {
    gen_kernel = params.get<bool>("gen-kernel");
    std::cout<<"gen_kernel: "<<gen_kernel<<std::endl;
  }

  if (params.keyExists<bool>("multiplex")){
    multiplex = params.get<bool>("multiplex");
    //this is essentially to tell me how to deal with layouts. If multiplex,
    //then we would split layout into two smaller layouts
    //and we would generate two error kernels.
  }

  if (params.keyExists<std::vector<std::size_t>>("layout")){
    layout = params.get<std::vector<std::size_t>>("layout");
    //std::cout<<"layout recieved"<<std::endl;
    std::cout<<"running bits on physical bits: ";
    for(auto &x: layout){
      std::cout<<x<<" ";
    }
    std::cout<<std::endl;
  }

  if (params.keyExists<bool>("gen-mult-kernels")){
    gen_mult_kernels = true;
    std::cout<<"generating kernels on all composite instructions (programs) passed to execute"<<std::endl;
  }

  if (params.keyExists<std::vector<int>>("layout")){
    auto tmp = params.get<std::vector<int>>("layout");
    for (auto& a : tmp) layout.push_back(a);
    //std::cout<<"layout recieved"<<std::endl;
    std::cout<<"running bits on physical bits: ";
    for(auto &x: layout){
      std::cout<<x<<" ";
    }
    std::cout<<std::endl;
  }
} // initialize

void AssignmentErrorKernelDecorator::execute(
    std::shared_ptr<AcceleratorBuffer> buffer,
    const std::shared_ptr<CompositeInstruction> function) {
  int num_bits = buffer->size();
  int size = std::pow(2, num_bits);
  if (!layout.empty()) {
     function->mapBits(layout);
  }

  decoratedAccelerator->execute(buffer, function);

  // get the raw state and num shots
  Eigen::VectorXd init_state(size);
  init_state.setZero();
  int i = 0;
  int shots = 0;
  for (auto &x : buffer->getMeasurementCounts()) {
    shots += x.second;
    init_state(i) = double(x.second);
    i++;
  }
  init_state = (double)1/shots*init_state;


  if (gen_kernel) {
    if (decoratedAccelerator) {
      generateKernel(buffer);
    }

  } else {
    // generating the list of permutations is O(2^num_bits), we want to minimize
    // the number of times we have to call it.
    if (permutations.empty()) {
      permutations = generatePermutations(num_bits);
    }
  }

  Eigen::VectorXd EM_state = errorKernel.colPivHouseholderQr().solve(init_state);
  // checking for negative values and performing a "clip and renorm"
  for (int i = 0; i < EM_state.size(); i++) {
    if (EM_state(i) < 0.0) {
      int count = floor(shots * EM_state(i) + 0.5);
      std::cout << "found negative value, clipping and renorming "<<std::endl;
      std::cout << "removed " << abs(count) << " shots from total shots" << std::endl;
      shots += count;
      EM_state(i) = 0.0;
    }
  }


  std::map<std::string, double> origCounts;

  int total = 0;
  i = 0;
  for (auto &x : permutations) {
    origCounts[x] = (double)buffer->getMeasurementCounts()[x];
    int count = floor(shots * EM_state(i) + 0.5);
    total += count;
    buffer->appendMeasurement(x, count);
    i++;
  }
  buffer->addExtraInfo("unmitigated-counts", origCounts);

  this->layout = {};
  return;
} // execute

void AssignmentErrorKernelDecorator::execute(
    const std::shared_ptr<AcceleratorBuffer> buffer,
    const std::vector<std::shared_ptr<CompositeInstruction>> functions) {

  int num_bits = buffer->size();
  int size = std::pow(2, num_bits);

  if (permutations.empty()) {
    permutations = generatePermutations(num_bits);
  }

  //execute on specified accelerator
  decoratedAccelerator->execute(buffer, functions);

  //compute number of shots
  auto buffers = buffer->getChildren();
  int shots = 0;
  for (auto &x : buffers[0]->getMeasurementCounts()) {
    shots += x.second;
  }



  std::vector<Eigen::VectorXd> init_states;
  std::vector<Eigen::VectorXd> EM_states;
  int i = 0;
  for (auto b : buffers) {
    //generating kernel for this buffer child
    if(gen_kernel){
      std::cout<<"generating kernel for sub buffer: "<<b->name()<<" "<<std::endl;
      generateKernel(b);
    }

    //std::cout<<"i: "<<i<<std::endl;
    Eigen::VectorXd temp(size);
    int j = 0;
    for (auto &x : permutations) {
      //std::cout<<"x: "<<x<<std::endl;
      temp(j) =(double)b->computeMeasurementProbability(x);
      j++;
    }
    init_states.push_back(temp);
    //std::cout<<"init states: "<<std::endl<<temp<<std::endl;

    //std::cout<<"EM'd state: "<<std::endl;
    EM_states.push_back(errorKernel * temp);
    //std::cout<<std::endl;
    //std::cout<<EM_states[i]<<std::endl;

  }

  for (int i = 0; i < EM_states.size(); i++) {
    for (int j = 0; j < EM_states[i].size(); j++) {
      if (EM_states[i](j) < 0.0) {
        int count = floor(shots * EM_states[i](j) + 0.5);
        std::cout<<EM_states[i](j)<<std::endl;
        std::cout << "found negative value, clipping and renorming "<<std::endl;
        std::cout << "removed " << abs(count) << " shots from total shots" << std::endl;
        shots += count;
        EM_states[i](j) = 0.0;
      }
    }
  }


  std::vector<std::map<std::string, double>> origCounts(buffer->nChildren());

  int total = 0;
  i = 0;
  for (auto &b : buffers) {
    int j = 0;
    for (auto &x : permutations) {
      origCounts[i][x] = (double)b->getMeasurementCounts()[x];
      int count = floor(shots * EM_states[i](j) + 0.5);
      j++;
      total += count;
      b->appendMeasurement(x, count);
      b->addExtraInfo("unmitigated-counts", origCounts[i]);
    }
    i++;
  }
} // execute (vectorized)

} // namespace quantum
} // namespace xacc
