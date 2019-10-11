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
#ifndef XACC_ALGORITHM_DDCL_STRATEGIES_MMD_LOSS_HPP_
#define XACC_ALGORITHM_DDCL_STRATEGIES_MMD_LOSS_HPP_

#include "ddcl.hpp"
#include "xacc.hpp"
#include <cassert>
#include <unsupported/Eigen/MatrixFunctions>

namespace xacc {
  namespace algorithm {

    struct MMD{
      std::vector<double> sigma_list;
      int num_bit;
      bool is_binary;
      std::vector<int> basis;
      std::vector<double> K;
    };

    class MMDLossStrategy : public LossStrategy {
    protected:
      // Helper functions

      //Assuming that the compute function should handle both pdf's and clip_and_renorm's capacities

      //confident
      std::vector<double> mix_rbf_kernel(std::vector<double> x,
                                         std::vector<double> y,
                                         std::vector<double> sigma_list,
                                         int num_bit){
        len = x.size();
        Eigen::MatrixXd dx2(len, len);

        for(int i = 0; i < num_bit; i++){
          for(int j = 0; j < len; j++ ){
            bool temp = (x[i]>>i)&1;
            bool temp0 = (y[j]>>i)&1;
            if(temp != temp0){
              dx2(i,j) = 1;
            }
            else{
              dx2(i,j) = 0;
            }
          }
        }
        return _mix_rbf_kernel_d(dx2, sigma_list);
      }

      //confident
      std::vector<double> _mix_rbf_kernel_d(Eigen::MatrixBase<derived> dx2,
                                             std::vector<double> sigma_list){
        Eigen::Matrix K(dx2._Rows(), dx2._Cols());
        for(sigma : sigma_list){
          double gamma = 1.0/(2*sigma);
          K += K.exp();
        }
        return K;
      }

      //confident
      double kernel_expect(Eigen::MatrixBase<derived> K,
                           std::vector<int> px,
                           std::vector<int>  py){
        len = px.size();
        P = Eigen::Map<Eigen::VectorXd>(px, len);
        Q = Eigen::Map<Eigen::VectorXd>(py, len);

        Eigen::VectorXd temp = K.dot(Q);
        temp = P.dot(temp);
        return temp;
      }

    public:
      std::pair<double, std::vector<double>>
      compute(Counts &counts, const std::vector<double> &target) override {
        double * pdf = pdf();

        int shots = 0;
        for (auto &x : counts) {
          shots += x.second;
        }

        // Compute the probability distribution
        std::vector<double> q(target.size()); // all zeros
        for (auto &x : counts) {
          int idx = std::stoi(x.first, nullptr, 2);
          q[idx] = (double)x.second / shots;
        }



        auto mmd = 
          return std::make_pair(mmd, q);
      }

      bool isValidGradientStrategy(const std::string &gradientStrategy) override {
        // FIXME define what grad strategies this guy works with
        return true;
      }
      const std::string name() const override { return "mmd"; }
      const std::string description() const override { return ""; }

    };





    class MMDParameterShiftGradientStrategy : public GradientStrategy {
    public:
      std::vector<Circuit>
      getCircuitExecutions(Circuit circuit, const std::vector<double> &x) override {

        std::vector<Circuit> grad_circuits;
        auto provider = xacc::getIRProvider("quantum");
        for (int i = 0; i < x.size(); i++) {
          auto xplus = x[i] + xacc::constants::pi / 2.;
          auto xminus = x[i] - xacc::constants::pi / 2.;
          std::vector<double> tmpx_plus = x, tmpx_minus = x;
          tmpx_plus[i] = xplus;
          tmpx_minus[i] = xminus;
          auto xplus_circuit = circuit->operator()(tmpx_plus);
          auto xminus_circuit = circuit->operator()(tmpx_minus);

          for (std::size_t i = 0; i < xplus_circuit->nLogicalBits(); i++) {
            auto m =
              provider->createInstruction("Measure", std::vector<std::size_t>{i});
            xplus_circuit->addInstruction(m);
          }
          for (std::size_t i = 0; i < xminus_circuit->nLogicalBits(); i++) {
            auto m =
              provider->createInstruction("Measure", std::vector<std::size_t>{i});
            xminus_circuit->addInstruction(m);
          }
          grad_circuits.push_back(xplus_circuit);
          grad_circuits.push_back(xminus_circuit);
        }

        return grad_circuits;
      }

      void compute(std::vector<double> &grad, std::vector<Counts> results,
                   const std::vector<double> &q_dist,
                   const std::vector<double> &target_dist) override {
        assert(grad.size() == 2 * results.size());

        // Get the number of shosts
        int shots = 0;
        for (auto &x : results[0]) {
          shots += x.second;
        }

        // Create q+ and q- vectors
        std::vector<std::vector<double>> qplus_theta, qminus_theta;
        for (int i = 0; i < results.size(); i += 2) {
          std::vector<double> qp(q_dist.size()), qm(q_dist.size());
          for (auto &x : results[i]) {
            int idx = std::stoi(x.first, nullptr, 2);
            qp[idx] = (double)x.second / shots;
          }
          for (auto &x : results[i + 1]) {
            int idx = std::stoi(x.first, nullptr, 2);
            qm[idx] = (double)x.second / shots;
          }

          qplus_theta.push_back(qp);
          qminus_theta.push_back(qm);
        }

        // std::cout << "qdist: " << q_dist << "\n";
        for (int i = 0; i < grad.size(); i++) {
          double sum = 0.0;
          for (int x = 0; x < q_dist.size(); x++) {
            if (std::fabs(q_dist[x]) > 1e-12) {
              sum += std::log(q_dist[x] / (0.5 * (target_dist[x] + q_dist[x]))) *
                0.5 * (qplus_theta[i][x] - qminus_theta[i][x]);
              //   std::cout << sum << "\n";
            }
          }
          sum *= 0.5;
          grad[i] = sum;
        }

        return;
      }

      const std::string name() const override { return "js-parameter-shift"; }
      const std::string description() const override { return ""; }
    };
  } // namespace algorithm
} // namespace xacc
#endif
