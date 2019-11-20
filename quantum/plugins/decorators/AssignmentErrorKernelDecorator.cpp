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
#include "xacc.hpp"
#include <fstream>


namespace xacc {
namespace quantum {
  //generate the powerset, which tells the set of all qbit orientations that need to be created for
  //circuit generation
  vector<vector<int>> generatePermutationList(int num_bits){
    vector<int> bits(num_bits) = for(int i = 0: i<num_bits: i++){bits[i] = i;};
    vector<int> permutations(pow(2,num_bits));
    while(i < pow(2,num_bits))
      {
        for(int j = 0; j < num_bits; j++)
          {
            if(i & (1 << j)){
              permutations[i] = set[j];
              i++;
            }
          }
      }
    return permutations;
  }

  Circuit generateCircuit(int num_bits, vector<vector<int>> permutations){
    //Here's what this function need to do,
    /*
      We're going to recieve a vector of ints which tells us which qubits need to have an X
      gate applied to them. We can iterate through this list and add composite instructions

     */
    provider = xacc::getIRProvider("quantum");
    vector<Circuit> EMCircuits(pow(2,num_bits));
    vector<auto> counts;
    for(auto circ: EMCircuits){
      for(auto x:permutations){
        for(int qbit: x){
          auto temp = provider->createInstruction("X", [qbit]);
        }
        circ->addInstruction(temp);
      }
      counts.push_back(circ->getMeasurements());
    }

      


    Circuit circuit = 0;
    //something.initialize(num_bits) //maybe it can take this in as a
      //parameter by reference
 
    }
  }

  //mandate in initialize that there's a key for generating kernel, set false after
  void AssignmentErrorKernelDecorator::initialize(std::shared_ptr<AcceleratorBuffer> buffer, const HeterogeneousMap &parameters){
    if(!parameters.keyExists<std::shared_ptr<Bool>>){
      std::cout<< "Not generating error kernel"
    }
    else{

      vector<vector<int>> permutations_list = generatePermutationList(num_bits);
      for(auto x: permutations_list){
        //generate the n circuits you need.
        auto circuit = generateCircuit(qbits)

        }
      }

    }
  }
  void AssignmentErrorKernelDecorator::execute(
                                               std::shared_ptr<AcceleratorBuffer> buffer,
                                               const std::shared_ptr<CompositeInstruction> function) {

    /*Here's what I need to run the Error Kernel
     I need the number of qubits
     I need to generate circuits to measure states
     I need the backend for which to run the kernel on (presumably included in the buffer)
     I need to return the pow(2,nqbits)x pow(2,nqbits) matrix M*output_state
     It should also have the option to only run the kernel build if the option is passed
     This is because:
     1) it is computational unwise to recreate the kernel for each iteration
     2) There's no need since the kernel is just to correct for readout error, which we asuume
     would be constant throughout the iteration process


     generating the  matrix:




     inv(E)*result = result' */
    if (decoratedAccelerator)
      decorattedAccelerator->execute(buffer, function);


    //First step find number of qbits:
    int num_bits = buffer -> size();

    auto state = buffer -> getMeasurmentCounts();
    std::cout<<"state = " << state << std::endl;
    //ibm = xacc.getaccelerator('ibm')
    //em_ibm = getacceleratordecorator('kmatrix', ibm, {'generate-k':True})


    vector<int> permutations = generatePermutationList(num_bits);

    HeterogeneousMap properties;
    if (decoratedAccelerator) {
      properties = decoratedAccelerator->getProperties();
    }

  }




  }


}

