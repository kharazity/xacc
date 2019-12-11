import xacc


num_shots = 8192
num_bits = 2
qpu = xacc.getAccelerator('local-ibm', {'shots':num_shots})
aer_dict = {'shots': num_shots,\
              'backend': "ibmq_johannesburg",\
              'gate_error': True,\
              'readout_error':True,\
              'thermal_relaxation': True}
noisy_qpu = xacc.getAccelerator('aer', aer_dict)

xasm = xacc.getCompiler('xasm')

xacc.qasm('''
.compiler xasm
.circuit qbit2_hwa
.parameters x
.qbit q

U(q[0], x[0], -pi/2, pi/2);

''')

target = [.25, .75]

f = xacc.getCompiled("qbit2_hwa")


initial_params = [0];
step_size = 0.1;
max_iter = 1;
optimizer_dict =  {'mlpack-step-size': step_size, 'mlpack-max-iter': max_iter, \
                       'initial-parameters': initial_params}

optimizer = xacc.getOptimizer('mlpack', optimizer_dict)

loss = 'mmd'
strategy = 'mmd-parameter-shift'
ddcl_dict = {'ansatz': f,
             'accelerator': qpu,
             'target_dist': target,
             'optimizer': optimizer,
             'loss': loss,
             'gradient': strategy}

noisy_ddcl_dict = ddcl_dict.copy()
noisy_ddcl_dict['accelerator'] = noisy_qpu


vals = np.linspace(-np.pi, np.pi, num = 30)

ddcl = xacc.getAlgorithm('ddcl', ddcl_dict)
qbits = xacc.qalloc(1)
noisy_ddcl = xacc.getAlgorithm('ddcl', noisy_ddcl_dict)
noisy_qbits = xacc.qalloc(1)

for x in vals:
    result = ddcl.execute(qbits, [x])
    noisy_result = noisy_ddcl.execute(noisy_qbits, [x])
    print("LOSS = ", result[0])
    print("NOISY_LOSS = ", noisy_result[0])
    print()

