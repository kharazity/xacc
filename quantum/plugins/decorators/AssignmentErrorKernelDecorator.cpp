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
    if(params.keyExists<std::shared_ptr<bool>>("gen-kernel")){
        gen_kernel = true;
      }
  }
  void AssignmentErrorKernelDecorator::execute(std::shared_ptr<AcceleratorBuffer> buffer,
                                             const std::shared_ptr<CompositeInstruction> function){
    //need to generate error kernel on same backend as buffer;
    //Then I need to actually execute the buffer with that function to get the counts. (there is no preprocessing)
    //I also need to figure out how to get a bool passed that will allow me to control when I compute the matrix, and if the matrix is computed, how do I save it for the next iteration?
    //buffer->appendMeasurement()(actual number)
    //buffer->addExtraInfo()
    int num_bits = (int)buffer -> size();
    if(gen_kernel){
      std::cout<<"put the rest of it here" << std::endl;
      //auto counts = buffer -> getMeasurementCounts();
      std::cout << "num_bits = " << num_bits << std::endl;
      if(decoratedAccelerator){
        decoratedAccelerator->execute(buffer, function);
        std::cout<<buffer->getMeasurementCounts()<<std::endl;
        std::vector<std::string> bitstring(pow(2,num_bits));
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

            counter++;
          }
          while(next_permutation(curr.begin(), curr.end()));

        }
        //using bitstring to generate circuits
        std::shared_ptr<AcceleratorBuffer> tmpBuffer = xacc::qalloc(buffer->size());
        std::vector<std::shared_ptr<CompositeInstruction>> circuits;
        auto provider = xacc::getIRProvider("quantum");
        for(int i = 0; i < std::pow(2,num_bits); i++){
          auto circuit = provider->createComposite(bitstring[i]);
          int j = num_bits-1;
          for(char c : bitstring[i]){
            if(c == '1'){
              std::cout<<"1 found at position: "<<j<<std::endl;
              auto x = provider->createInstruction("X", j);
              circuit->addInstruction(x);
            }
            j--;
          }
          auto m0  = provider->createInstruction("Measure", 0);
          auto m1 = provider->createInstruction("Measure", 1);
          circuit->addInstruction(m0);
          circuit->addInstruction(m1);
          std::cout<<"circuit: " << bitstring[i] << std::endl;
          circuits.push_back(circuit);
        }
        decoratedAccelerator->execute(tmpBuffer, circuits);
        std::cout<<"here"<<std::endl;
        auto buffers = tmpBuffer->getChildren();

        int shots = 0;
        //compute num shots;
        for(auto &x : buffers[0]->getMeasurementCounts()){
          shots += x.second;
        }
        std::cout<<"num_shots = " << shots <<std::endl;
        //std::vector<auto> results;
        for (auto &b: buffers){
          //std::cout<< b->getMeasurementCounts() << std::endl;
          //results.push_back(b->getMeasurementCounts());
          //std::cout<<results <<std::endl;
        }
        //needs to be computed, or found in buffer
        //convert results vector<vector> into Eigen Matrix object
        Eigen::MatrixXd K = Eigen::MatrixXd::Zero(std::pow(2,num_bits), std::pow(2,num_bits));
        for(int i = 0; i < std::pow(2,num_bits); i++){
          for(int j = 0; j < std::pow(2,num_bits); j++){
            std::cout<<"put results here";
          }
        }
        //after kernel is generate set
        //gen_kernel = false;
        //take the Eigen Matrix and invert it:
        //Eigen::MatrixXd Kinv = K.inverse();
        //state = Kinv*state;
        //update state in buffer
      }
    }
    return;
  }//execute


} //namespace quantum
} //namespace xacc
