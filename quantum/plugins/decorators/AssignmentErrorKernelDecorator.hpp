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
 *   Tyler Kharazi - initial implementation
 *******************************************************************************/
#ifndef XACC_ASSIGNMENTERRORKERNELDECORATOR_HPP_
#define XACC_ASSIGNMENTERRORKERNELDECORATOR_HPP_

#include "AcceleratorDecorator.hpp"
#include <Eigen/Dense>
#include "xacc.hpp"

namespace xacc {

namespace quantum {

class AssignmentErrorKernelDecorator : public AcceleratorDecorator {
protected:
  bool gen_kernel;
  Eigen::MatrixXd errorKernel;
  std::vector<std::string> permutations = {""};
  bool multiplex;
  std::vector<std::size_t> layout;
  bool gen_mult_kernels;

  std::string toBinary(int num, int num_bits){
    std::string s = std::bitset<64> (num).to_string();
    std::string bitstring = "";
    for (int i = 0; i < num_bits; i++){
      bitstring += s[64-num_bits+i];
    }
    //std::cout<<bitstring<<std::endl;
    return bitstring;
  }

  std::vector<std::string> generatePermutations(int num_bits) {
    int pow_bits = std::pow(2, num_bits);
    std::vector<std::string> permutations(pow_bits);
    for(int i = 0; i < pow_bits; i++){
      permutations[i] = toBinary(i, num_bits);
    }
    return permutations;
  }
  /*
  Eigen::VectorXd non_negative_least_squares(Eigen::VectorXd dist_meas){
    int size = dist_meas.size();
    std::cout<<"size = "<<size<<std::endl;
    double eps = 1e-6;
    Eigen::VectorXd x(size);
    Eigen::VectorXd w = errorKernel.transpose()*(dist_meas - errorKernel*x);

    Eigen::MatrixXd H = errorKernel.transpose()*errorKernel;
    Eigen::MatrixXd h = -1. * errorKernel.transpose()*dist_meas;
    Eigen::VectorXd quadrature = 1/2*dist_meas.transpose()*H*dist_meas + h.transpose()*dist_meas;
    std::vector<int> R;
    for(int i = 1; i <= size; i++){
      R[i] = i;
    }

    std::vector<int> P;

    while(R.size() > 0 && w.maxCoeff() > eps){
      float max = w.maxCoeff();
      int j = 0;
      for(int i = 0; i < R.size(); i++){
        if(w[i] == max){
          std::cout<<"coeff idx: " << i << std::endl;
          std::cout<<"max = : "<< w[i] <<std::endl;
          std::cout<<"what is the max: " << max << std::endl;
          j = i;
          R.erase(j);
          break;
        }
        std::cout<<"we found the max at index: "<< j <<std::endl;
        P.push_back(j);
        
      }
    }




    return quadrature;

  }
  */

  void generateKernel(std::shared_ptr<AcceleratorBuffer> buffer) {
    int num_bits = buffer->size();
    std::cout<<"num_bits: " << num_bits << std::endl;
    int pow_bits = std::pow(2, num_bits);
    this->permutations = generatePermutations(num_bits);

    // permutations contains all possible permutations of bitstrings of length num_bits,
    // there is direct mapping from permutations to circuit as follows:
    // permutations: 10 => X gate on first qubit and nothing on the zeroeth qubit and
    // measure both permutations: 11 => X gate on both qubits and measure both
    // etc...

    std::shared_ptr<AcceleratorBuffer> tmpBuffer = xacc::qalloc(buffer->size());

    // generating list of circuits to evaluate
    std::vector<std::shared_ptr<CompositeInstruction>> circuits;
    auto provider = xacc::getIRProvider("quantum");
    for (int i = 0; i < pow_bits; i++) {
      //std::cout<<"State prep: "<<permutations[i]<<std::endl;
      auto circuit = provider->createComposite(permutations[i]);
      int j = num_bits - 1;
      for (char c : permutations[i]) {
        if (c == '1') {
          auto x = provider->createInstruction("X", j);
          circuit->addInstruction(x);
        }
        j--;
      }
      for(int i = 0; i < num_bits; i++){
        circuit->addInstruction(provider->createInstruction("Measure", i));
      }
      if (!layout.empty()){
        circuit->mapBits(layout);
      }

      circuits.push_back(circuit);
      std::cout<<circuit->toString()<<std::endl;
    }
    //std::cout<<tmpBuffer->size()<<std::endl;
    decoratedAccelerator->execute(tmpBuffer, circuits);
    auto buffers = tmpBuffer->getChildren();
    int shots = 0;
    // compute num shots;
    std::cout<<tmpBuffer->nChildren()<<std::endl;
    for (auto &x : buffers[0]->getMeasurementCounts()) {
      shots += x.second;
    }
    // initializing vector of vector of counts to size 2^num_bits x 2^num_bits
    std::vector<std::vector<int>> counts(std::pow(2, num_bits),
                                         std::vector<int>(pow_bits));

    int row = 0;
    Eigen::MatrixXd K(pow_bits, pow_bits);
    K.setZero();
    for (auto &b : buffers) {
      //b->print();
      int col = 0;
      //std::cout<<b->name()<<": "<<std::endl;
      for (auto &x : permutations) {
        auto temp = b->computeMeasurementProbability(x);
        //std::cout<<"counts for " << x <<": "<< temp<<std::endl;
        K(row, col) = temp;
        col++;
      }
      std::cout<<std::endl;
      row++;
    }
    //std::cout << "MATRIX:\n" << K << "\n";
    errorKernel = K.inverse();
    //std::cout << "INVERSE:\n" << errorKernel << "\n";

    std::vector<double> vec(K.data(), K.data() + K.rows()*K.cols());
    buffer->addExtraInfo("error-kernel", vec);

    if(this->gen_mult_kernels == true){
      gen_kernel = true;
    }
    else{
      gen_kernel = false;
    }
  }

public:
  AssignmentErrorKernelDecorator() = default;
  const std::vector<std::string> configurationKeys() override {
    return {"gen-kernel"};
  }

  void initialize(const HeterogeneousMap &params = {}) override;

  void execute(std::shared_ptr<AcceleratorBuffer> buffer,
               const std::shared_ptr<CompositeInstruction> function) override;
  void execute(std::shared_ptr<AcceleratorBuffer> buffer,
               const std::vector<std::shared_ptr<CompositeInstruction>>
               functions) override;

  const std::string name() const override { return "assignment-error-kernel"; }
  const std::string description() const override { return ""; }

  ~AssignmentErrorKernelDecorator() override {}

 
};

} // namespace quantum
} // namespace xacc
#endif
