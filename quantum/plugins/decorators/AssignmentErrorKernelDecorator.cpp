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
#include "AssignmentErrorKernelDecorator.hpp"
#include "InstructionIterator.hpp"
#include "Utils.hpp"
#include "xacc.hpp"
#include <fstream>
#include <set>
#include <Eigen/Dense>

namespace xacc{
namespace quantum{
  void AssignmentErrorKernelDecorator::initialize(const HeterogeneousMap& params){

    if(params.keyExists<bool>("gen-kernel")){
      gen_kernel = params.get<bool>("gen-kernel");
    }
  }
  void AssignmentErrorKernelDecorator::execute(std::shared_ptr<AcceleratorBuffer> buffer,
                                             const std::shared_ptr<CompositeInstruction> function){
    int num_bits = (int)buffer->size();
    std::vector<std::string> permutations;
    if(gen_kernel){
      if(decoratedAccelerator){
        //pow(2, num_bits) gets used a lot, so I figured I should just define it here
        int pow_bits = std::pow(2, num_bits);
        //Algorithm to iterate through all possible bitstrings for a given number of bits
        std::vector<std::string> bitstring(pow_bits);
        std::string str = "";
        std::string curr;
        int counter = 0;
        int j = num_bits;
        while(j--){
          str.push_back('0');
        }
        for(int k = 0; k <= num_bits; k++){
          str[num_bits - k] = '1';
          curr = str;
          do{
            bitstring[counter] = curr;
            //std::cout<<"curr = "<<curr <<std::endl;
            counter++;
          }
          while(next_permutation(curr.begin(), curr.end()));

        }
        permutations = bitstring;
        //bitstring contains all possible bitstrings to generate circuits, there is direct
        //mapping from bitstring to circuit as follows:
        //bitstring: 10 => X gate on zeroth qubit and nothing on first qubit and measure both
        //bitstring: 11 => X gate on both qubits and measure both etc...

        std::shared_ptr<AcceleratorBuffer> tmpBuffer = xacc::qalloc(buffer->size());

        //list of circuits to evaluate
        std::vector<std::shared_ptr<CompositeInstruction>> circuits;
        auto provider = xacc::getIRProvider("quantum");
        for(int i = 0; i < pow_bits; i++){
          auto circuit = provider->createComposite(bitstring[i]);
          int j = num_bits-1;
          for(char c : bitstring[i]){
            if(c == '1'){
              //std::cout<<"1 found at position: "<<j<<std::endl;
              auto x = provider->createInstruction("X", j);
              circuit->addInstruction(x);
            }
            j--;
          }
          auto m0  = provider->createInstruction("Measure", 0);
          auto m1 = provider->createInstruction("Measure", 1);
          circuit->addInstruction(m0);
          circuit->addInstruction(m1);
          circuits.push_back(circuit);
        }
        decoratedAccelerator->execute(tmpBuffer, circuits);
        //std::cout<<"here"<<std::endl;
        auto buffers = tmpBuffer->getChildren();

        int shots = 0;
        //compute num shots;
        for(auto &x : buffers[0]->getMeasurementCounts()){
          shots += x.second;
        }
        std::cout<<"num_shots = " << shots <<std::endl;
        //initializing vector of vector of counts to size 2^num_bits x 2^num_bits
        std::vector<std::vector<int>> counts(std::pow(2, num_bits),
                                              std::vector<int>(pow_bits));

        int row = 0;
        Eigen::MatrixXd K(pow_bits, pow_bits);
        for (auto &b: buffers){
          int col = 0;
          for(auto& x : bitstring){
            auto temp = b->computeMeasurementProbability(x);
            K(row, col) = temp;
            col++;
          }
          row++;
        }
        errorKernel = K.inverse();
        std::cout<<std::endl<<errorKernel<<std::endl;
        gen_kernel = false;
      }
      
    }
    //Eigen::VectorXd EM_state = errorKernel*init_state;

    decoratedAccelerator->execute(buffer,function);
    int size = std::pow(2, num_bits);
    Eigen::VectorXd init_state(size);
    int i = 0;
    for(auto &x: permutations){
      init_state(i) =buffer->computeMeasurementProbability(x);
      i++;
    }
    std::cout<<"init_state: "<<std::endl;
    std::cout<<init_state<<std::endl;
    auto EM_state = errorKernel*init_state;
    std::cout<<"Error Mitigated:"<<std::endl;
    std::cout<<std::endl<<EM_state<<std::endl;
    return;
  }//execute

  void AssignmentErrorKernelDecorator::execute(const std::shared_ptr<AcceleratorBuffer> buffer,
               const std::vector<std::shared_ptr<CompositeInstruction>> functions){

    return;
  }//execute (vectorized)

} //namespace quantum
} //namespace xacc
