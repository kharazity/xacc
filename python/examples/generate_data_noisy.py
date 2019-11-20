import xacc
from data_process import processFile
import numpy as np

def runNoisyCircuit(num_bits, noisy_ddcl_dict,  aer_dict, noisy_ab_file):
    noisy_qpu = xacc.getAccelerator('aer', aer_dict)
    noisy_qbit = xacc.qalloc(num_bits)
    noisy_ddcl_dict['accelerator'] = noisy_qpu
    ddcl_noisy = xacc.getAlgorithm('ddcl', noisy_ddcl_dict)
    ddcl_noisy.execute(noisy_qbit)

    IO = open(noisy_ab_file, 'w')
    IO.write(str(noisy_qbit))
    IO.close()
    loss = noisy_ddcl_dict['loss']
    processFile(noisy_ab_file, loss)


def generateNoisyPlots(noisy_last_param, aer_dict, noisy_ddcl_dict, num_bits):
    vals = np.linspace(-np.pi, np.pi, num = 200)
    noisy_qpu = xacc.getAccelerator('aer', aer_dict)
    noisy_ddcl_dict['accelerator'] = noisy_qpu
    noisy_ddcl = xacc.getAlgorithm('ddcl', noisy_ddcl_dict)
    noisy_qbits = xacc.qalloc(num_bits)
    noisy_loss = []
    for i in range (len(noisy_last_param)):
        print("noisy %s"%(i))
        loss_dict_noisy = {}
        for x in vals:
            last_param_noisy_ = noisy_last_param.copy()
            last_param_noisy_[i] += x
            result = noisy_ddcl.execute(noisy_qbits, last_param_noisy_)
            loss_dict_noisy[last_param_noisy_[i]] = result[0]
        noisy_loss.append(loss_dict_noisy)

    return noisy_loss


def loadJSON(filepath)
