import xacc
import os
import argparse
import glob
import numpy as np
import re
from targets import target_dict
from parseQASM import parseQASM
from data_process import processFile
import matplotlib.pyplot as plt


def getListOfDirs(dir_name):
    file_list = os.listdir(dir_name)
    all_dirs = list()
    for file in file_list:
        full_path = os.path.join(dir_name, file)
        if os.path.isdir(full_path):
            all_dirs.append(full_path)
            all_dirs = all_dirs + getListOfDirs(full_path)
    print(all_dirs)
    return all_dirs




def runCircuit(qasm_file, num_bits, num_shots = 2048, target = target_dict, loss ='mmd', initial_parameters = [], step_size = .1, max_iter = 40):
    qbits = xacc.qalloc(num_bits)
    qpu = xacc.getAccelerator('local-ibm', {'shots': num_shots})
    qasm_txt = open(qasm_file, 'r').read()
    crct = qasm_file.replace('.qasm', '')
    xacc.qasm('''
    .compiler xasm
    .circuit {}
    .parameters x
    .qbit q
    {}'''.format(crct, qasm_txt))
    f = xacc.getCompiled(crct)
    optimizer_dict =  {'mlpack-step-size': step_size, 'mlpack-max-iter': max_iter,  'initial-parameters': initial_parameters}
    optimizer = xacc.getOptimizer('mlpack', optimizer_dict)
    targets = target[num_bits]

    base_path = os.getcwd()
    print("base_path = ", base_path)
    for target in targets.values():
        ab_file = qasm_file.replace('.qasm','.ab')
        loss_dir = base_path + "/%s"%(loss)
        if os.path.exists(loss_dir):
            os.chdir(loss_dir)
        else:
            os.mkdir(loss_dir)
            os.chdir(loss_dir)
        target_dir = loss_dir + "/%s"%(target)
        if os.path.exists(target_dir):
            os.chdir(target_dir)
        else:
            os.mkdir(target_dir)
            os.chdir(target_dir)
        data_dir = target_dir + "/data"
        print("Calculating for target: ", target)
        strategy = loss+'-parameter-shift'
        ddcl_dict = {'ansatz': f,
                          'accelerator': qpu,
                          'target_dist': target,
                          'persist-buffer': True,
                          'optimizer': optimizer,
                          'loss': loss,
                          'gradient': strategy }

        ddcl = xacc.getAlgorithm('ddcl', ddcl_dict)
        ddcl.execute(qbits)

        if os.path.exists(data_dir):
            os.chdir(data_dir)
        else:
            os.mkdir(data_dir)
            os.chdir(data_dir)

        ab_file = data_dir + "/" + qasm_file.replace(".qasm", ".ab")
        print("ab_file = ", ab_file)
        IO = open(ab_file, 'w')
        IO.write(str(qbits))
        IO.close()

        print("FILE written to: ", os.path.abspath(ab_file))

        processFile(ab_file, loss)

        print("CD = ", os.getcwd())

        optimizer_dict['maxiter'] = 1
        optimizer = xacc.getOptimizer('mlpack', optimizer_dict)
        ddcl = xacc.getAlgorithm('ddcl', ddcl_dict)

        print("GENERATING PLOTS")
        generatePerturbationPlots(ab_file, ddcl, qbits)
        os.chdir(target_dir+"/../")
    del f


def generatePerturbationPlots(ab_file, ddcl, qbits):
    print("This is where we think we're at", os.getcwd())
    #/Users/6tk/Desktop/Examples1/1qbit/xacc_1qubit_basic/mmd/target
    print(ab_file)
    buffer = xacc.loadBuffer(open(ab_file,'r').read())
    parameters = buffer.getAllUnique('parameters')
    last_param = parameters[-1]
    loss = []
    dir_name = ab_file.replace(".ab", "")
    vals = np.linspace(-np.pi, np.pi, num = 200)
    for i in range(len(last_param)):
        loss_dict = {}
        count = 0
        for x in vals:
            last_param_ = last_param
            last_param_[i] += x
            loss_dict[count] = ddcl.execute(qbits, last_param_)[0]
            count +=1
        loss.append(loss_dict)
    fig, ax = plt.subplots()
    count = 0
    for val in loss:
        lists = sorted(val.items())
        x, y = zip(*lists)
        vals = np.add(vals, x)
        plt.plot(vals,y)
        plt.title("%s Direction %s"%(ab_file, count))
        plt.xlabel("parameter")
        plt.ylabel("loss")
        current_directory = os.path.dirname(os.path.abspath(ab_file))
        print("current_directory = ", current_directory)
        paths_dir =os.path.join(current_directory, "plots")
        print("CURRENT DIR = ", current_directory)
        print("PATH = ", paths_dir)
        if os.path.exists(paths_dir):
            os.chdir(paths_dir)
            plot_dir = ab_file.replace(".ab", "_perturbed_dir%s.png"%(count))
            filename = os.path.join(os.getcwd(), plot_dir)
            plt.savefig(filename, dpi = 400)
        else:
            print(paths_dir)
            os.mkdir(paths_dir)
            os.chdir(paths_dir)
            plot_dir = ab_file.replace(".ab", "_perturbed_dir%s.png"%(count))
            filename = os.path.join(os.getcwd(), plot_dir)
            plt.savefig(filename, dpi = 400)
        plt.close()
        os.chdir(current_directory)
        count+=1





def perturbFromTrained(params):
    return





parser = argparse.ArgumentParser(description = 'options for perturbing initial parameters and overwriting files')

parser.add_argument('--perturb', help = "perturb initial parameters by random vector", action = "store_true")
parser.add_argument('--overwrite', help = "overwrite saved plots", action = "store_true")
parser.add_argument('--hardware', help = "specify which hardware to use")
parser.add_argument('--shots', help = "nshots to run on your experiment, default is 2048")
parser.add_argument('--dir', help = "pass a qasm file or directory of qasm files to process, otherwise recursively search through all sub directories for .qasm files", action = "store")
parser.add_argument('--loss', help ="Pass either 'mmd' or 'js' for js or mmd losses respectively")
args = parser.parse_args()

nshots = 2048
if args.shots:
    nshots = args.shots
perturb = 0
if args.perturb:
    perturb = 1

overwrite = 0
if args.overwrite:
    overwrite = 1

parse_dir = ""
if args.dir:
    print("INPUT DIRECTORY = ", args.dir)
    parse_dir = args.dir
    directories = getListOfDirs(parse_dir)
else:
    current_directory = os.getcwd()
    directories = getListOfDirs(current_directory)





def execute():
    GLOBAL_ROOT = os.getcwd()
    if args.dir:
        for dir in directories:
            os.chdir(dir)
            print("CURRENTLY IN DIR", args.dir)
            files = glob.glob("*.qasm")
            print("FILES = ", files)
            for file in files:
                if file.startswith("preprocessed"):
                    print("ignoring file: ", file)
                    continue
                elif file.startswith("xacc"):
                    print("ignoring file:", file)
                    continue
                else:
                    print("process file: ", file)
                    start_params, filename, nbits = parseQASM(file)

                if perturb:
                    delta = .01*np.random.normal()
                    start_params += delta
                    circuit = runCircuit(filename, nbits, initial_parameters = start_params)
                    os.chdir(GLOBAL_ROOT)
                else:
                    print("filename = ", filename)
                    print("running file %s"%(filename))
                    circuit = runCircuit(filename, nbits, initial_parameters = start_params, loss = "js")
                    os.chdir(GLOBAL_ROOT)


    else:
        for dir in directories:
            os.chdir(dir)
            print("CURRENTLY IN DIR ", dir)
            files = glob.glob("*.qasm")
            print("FILES = ", files)
            for file in files:
                if file.startswith("preprocessed"):
                    print("ignoring file: ", file)
                    continue
                #prevents neverending processing
                elif file.startswith("xacc"):
                    print("ignoring file:", file)
                    continue
                else:
                    print("process file: ", file)
                    start_params, filename, nbits = parseQASM(file)
                if perturb:
                    delta = .01*np.random.normal()
                    start_params += delta
                    circuit = runCircuit(filename, nbits, initial_parameters = start_params)
                else:
                    print("filename = ", filename)
                    print("running file %s"%(filename))
                    circuit = runCircuit(filename, nbits, initial_parameters = start_params)
                os.chdir(GLOBAL_ROOT)

        for dir in directories:
            #cleaning up directory
            os.chdir(dir)
            files = glob.glob("*.qasm")
            for file in files:
                if file.startswith("xacc"):
                    print("REMOVING FILE: ", file)
                    os.remove(file)

execute()




