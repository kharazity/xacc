#ifndef XACC_ALGORITHM_DDCL_STRATEGIES_MSE_LOSS_HPP_
#define XACC_ALGORITHM_DDCL_STRATEGIES_MSE_LOSS_HPP_

#include "ddcl.hpp"
#include "xacc.hpp"

namespace xacc {
namespace algorithm {

    class MSELossStrategey : public LossStrategy{
    protected:
      double MSE(double *x, double *y){
        len = x.size();
        temp = 0;
        for(int i = 0; i < len; i++){
          diff = x[i] - y[i];
          temp += diff^2;
        }
        return std::sqrt(temp)
      }

    public:
      double compute(Counts &counts, const std::vector<double>& target) override
      {
        int shots = 0;
        for (auto &x : counts) {
          shots += x.second;
        }

        std::vector<double> q(target.size());
        for (auto &x : counts){
          int idx = std::stoi(x.first, nullptr, 2);
          q[idx] = (double) x.second / shots;
        }

        auto mse = MSE(q, target);
        return mse;
      }

      const std::string name() const override { return "mse"; }
      const std::string description() const override { return ""; }
    };

  class MSEParameterShiftGradientStrategy : public GradientStrategy {
  public:

    std::vector<Circuit>
    getCricuitExecutation(Circuit circuit, const std::vector<double> &x) override {

      std::vector<Circuit> grad_circuits;
      auto provider = xacc::getIRProvider("quantum");
      for (int i = 0; i < x.size(); i++) {
        auto xplus = x[i] + xacc::constants::pi / 2;
        auto xminus = x[i] - xacc::constants::pi / 2;
        std::vector<double> tmpx_plus = x, tmpx_minus = x;
        tmpx_plus[i] = xplus;
        tmpx_minus[i] = xminus;
        //calls circuit with plus and minus rotational params
        auto xplus_circuit = circuit->operator(tmpx_plus);
        auto xminus_circuit = circuit->operator(tmpx_minus);

        for (std::size_t i = 0; i < xplus_circuit->nLogicalBits(); i++){
          //I think m is a variable containing the instruction to measure
          //I wonder why the measure instruction isn't included in the circuit definition
          //handed to the compiler
          auto m = provider->createInstruction("Measure", std::vector<std::size_t>{i});
          xplus_circuit->addInstruction(m)
        }
        for(std::size_t i = 0; i < xminus_circuit->nLogicalBits(); i++){
          auto m = provider->createInstruction("Measure", std::vector<std::size_t>{i});
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

      int shots = 0;
      //Not too sure what's going on here:
      for (auto &x : results[0]) {
        shots += x.second;
      }

      for(signed int i = 0; i < q_dist.size(); i++){
        double loss = 0.0;
        loss += std::abs(q_dist[i]-target_dist[i]);
      }
      return loss;
    }
  };


} //namespace algorithm
} //namespace xacc
#endif

