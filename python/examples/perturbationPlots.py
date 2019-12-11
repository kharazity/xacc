import xacc
import numpy as np
import matplotlib.pyplot as plt
from parseQASM import parseQASM


def generatePerturbationPlots(ab_file, ddcl, qbits):
    buffer = xacc.loadBuffer(open(ab_file,'r').read())
    parameters = buffer.getAllUnique('parameters')
    last_param = parameters[-1]
    loss = []
    vals = np.linspace(-np.pi, np.pi, num = 150)
    for i in range(len(last_param)):
        loss_dict = {}
        count = 0
        for x in vals:
            last_param_ = last_param
            last_param_[i] += x
            loss_dict[count] = ddcl.execute(qbits, last_param_)[0]
            count +=1
        loss.append(loss_dict)
        print(len(loss_dict))
    fig, ax = plt.subplots()
    print("len(loss) = ", len(loss))
    count = 0
    for val in loss:
        lists = sorted(val.items())
        x, y = zip(*lists)
        plt.plot(vals,y)
        plt.title("Direction %s"%(count))
        plt.show()
        filename = ab_file.replace(".ab", "_perturbed_dir%s.pdf"%(count))
        plt.savefig(filename, dpi = 400)
        count+=1




qbits = xacc.qalloc(3)
qpu = xacc.getAccelerator('local-ibm', {'shots': 2048})
start_params, filename, nbits = parseQASM("3qubit_depth1.qasm")
qasm_txt = open(filename, 'r').read()

target = [0.25,0.0,0.25,0.0,0.25,0.0,0.25,0.0]
crct = filename.replace('.qasm', '')
xacc.qasm('''
.compiler xasm
.circuit {}
.parameters x
.qbit q
{}'''.format(crct, qasm_txt))
f = xacc.getCompiled(crct)


optimizer_dict =  {'stepSize': .05, 'maxiter': 1,  'initial-parameters': start_params}
loss = 'js'
strategy = 'js-parameter-shift'

optimizer =xacc.getOptimizer('mlpack', optimizer_dict)
ddcl_dict = {'ansatz': f,
             'accelerator': qpu,
             'target_dist': target,
             'persist-buffer': True,
             'optimizer': optimizer,
             'loss': loss,
             'gradient': strategy }


ddcl = xacc.getAlgorithm('ddcl', ddcl_dict)


ab_file = "xacc_3qubit_depth1_js.ab"
generatePerturbationPlots(ab_file, ddcl ,qbits)
