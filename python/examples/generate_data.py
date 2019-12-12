import xacc
import os
import argparse
import glob
import numpy as np
import re
import csv
from targets import target_dict
from parseQASM import parseQASM
from data_process import processFile
import matplotlib.pyplot as plt
import multiprocessing
from multiprocessing import pool
from generate_data_noisy import runNoisyCircuit, generateNoisyPlots
import json



def getListOfDirs(dir_name):
    #print("dir_name = ", dir_name)
    file_list = os.listdir(dir_name)
    all_dirs = list()
    for file in file_list:
        full_path = os.path.join(dir_name, file)
        if os.path.isdir(full_path):
            all_dirs.append(full_path)
            all_dirs = all_dirs + getListOfDirs(full_path)
    #print(all_dirs)
    return all_dirs


def runCircuit(qasm_file, num_bits, num_shots = 8192, target = target_dict, loss ='mmd', initial_parameters = [], step_size = .1, max_iter = 40, noisy = False, backend = "ibmq_johannesburg"):
    #print("qasm_file = ", qasm_file)
    #Non-noisy accelerator:las
    if args.v:
        xacc.set_verbose(True)

    #initialize simulated qpu/qbits
    qpu = xacc.getAccelerator('local-ibm', {'shots':num_shots})
    qbits = xacc.qalloc(num_bits)

    #generate the circuit to be optimized
    qasm_txt = open(qasm_file, 'r').read()
    crct = qasm_file.replace('.qasm', '')
    xacc.qasm('''
    .compiler xasm
    .circuit {}
    .parameters x
    .qbit q
    {}'''.format(crct, qasm_txt))
    f = xacc.getCompiled(crct) #ansatz

    targets = target[num_bits]
    base_path = os.getcwd()
    for target in targets.values():
        #I think this might be better
        optimizer_dict =  {'mlpack-step-size': step_size, 'mlpack-max-iter': max_iter, \
                       'initial-parameters': initial_parameters}
        optimizer = xacc.getOptimizer('mlpack', optimizer_dict)
        ab_file = qasm_file.replace('.qasm','.ab')
        noisy_ab_file = "noisy_"+ab_file
        loss_dir = base_path + "/%s"%(loss)
        #generate appropriate sub-directories
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
        if os.path.exists(data_dir):
            os.chdir(data_dir)
        else:
            os.mkdir(data_dir)
            os.chdir(data_dir)
        print("Calculating for target: ", target)
        strategy = loss+'-parameter-shift'
        ddcl_dict = {'ansatz': f,
                     'accelerator': qpu,
                     'target_dist': target,
                     'optimizer': optimizer,
                     'loss': loss,
                     'gradient': strategy}

        #full path to (data) ab_file
        ab_file = data_dir + "/" + qasm_file.replace(".qasm", ".ab")

        ddcl_noisy = None
        if noisy:
            print("NOISY PROCESSING")
            noisy_ddcl_dict = ddcl_dict.copy()
            noisy_ddcl_dict['accelerator'] = ''
            aer_dict = {'shots': num_shots,\
                        'backend': backend,\
                        'gate_error': True,\
                        'readout_error': True,\
                        'thermal_relaxation': False}
            noisy_ab_file = os.path.dirname(ab_file)+"/" +noisy_ab_file
            #generates abfile for noisy processing later
            runNoisyCircuit(num_bits, noisy_ddcl_dict, aer_dict, noisy_ab_file)

        ddcl = xacc.getAlgorithm('ddcl', ddcl_dict)
        ddcl.execute(qbits)
        IO = open(ab_file, 'w')
        IO.write(str(qbits))
        processFile(ab_file, loss)

        optimizer_dict['mlpack-max-iter'] = 1
        optimizer = xacc.getOptimizer('mlpack', optimizer_dict)

        print("GENERATING PLOTS")
        if args.plot:
            generatePerturbationPlots(ab_file, ddcl_dict, qbits, noisy, num_bits, aer_dict = aer_dict)
            os.chdir(target_dir+"/../")
    del f



def generatePerturbationPlots(ab_file, ddcl_dict, qbits, noisy, num_bits, aer_dict = None):
    buffer = xacc.loadBuffer(open(ab_file,'r').read())
    parameters = buffer.getAllUnique('parameters')
    last_param = parameters[-1]
    data = {}
    data[ab_file.replace(".ab","")]= last_param;
    ddcl = xacc.getAlgorithm('ddcl', ddcl_dict)
    qbits = xacc.qalloc(num_bits)
    noisy_ab_file = os.path.dirname(ab_file) +"/noisy_" + os.path.basename(ab_file)
    print("noisy_ab_file = ", noisy_ab_file)
    print("noiseless = ", last_param)
   
    if noisy:
        noisy_buffer = xacc.loadBuffer(open(noisy_ab_file, 'r').read())
        noisy_parameters = buffer.getAllUnique('parameters')
        data[noisy_ab_file.replace(".ab","")]=noisy_parameters[-1];
        print("Noisy = ", noisy_parameters[-1])
        noisy_last_param = noisy_parameters[-1]
        noisy_ddcl_dict = ddcl_dict.copy()
        noisy_ddcl_dict['accelerator'] = ''


    with open("/Users/6tk/Desktop/qasm_dir/final_params.csv", 'a') as outfile:
        w = csv.DictWriter(outfile, data.keys());
        w.writeheader()
        w.writerow(data);

    print("last parameters written to .json file")
    vals = np.linspace(-np.pi, np.pi, num = 200)

    if noisy:
        noisy_loss = generateNoisyPlots(noisy_last_param, aer_dict, noisy_ddcl_dict, num_bits)

    loss = []
    for i in range(len(last_param)):
        print(i)
        loss_dict = {}

        for x in vals:
            last_param_ = last_param.copy()
            last_param_[i] += x
            result = ddcl.execute(qbits, last_param_)
            loss_dict[last_param_[i]] = result[0]
        loss.append(loss_dict)


    json_file = ab_file.replace('.ab', '.json')
    with open(json_file, 'w') as fout:
        json.dump(loss, fout)






    #PLOTTING
    fig, ax = plt.subplots()
    count = 0
    for i in range(len(loss)):
        lists = sorted(loss[i].items())
        x, y = zip(*lists)
        if noisy:
            noisy_lists = sorted(noisy_loss[i].items())
            x_noisy, y_noisy = zip(*noisy_lists)

        plt.plot(vals,y, label = "simulated")
        if noisy:
            plt.plot(vals, y_noisy, label = "noisy")

        plt.legend()
        plt.title("%s Direction %s"%(ab_file, count))
        plt.xlabel("parameter")
        plt.ylabel("loss")
        current_directory = os.path.dirname(os.path.abspath(ab_file))
        paths_dir =os.path.join(current_directory, "plots")

        if os.path.exists(paths_dir):
            os.chdir(paths_dir)
            plot_dir = ab_file.replace(".ab", "_perturbed_dir%s.png"%(count))
            if noisy:
                plot_dir = noisy_ab_file.replace(".ab", "_noisy_perturbed_dir%s.png"%(count))

            filename = os.path.join(os.getcwd(), plot_dir)
            plt.savefig(filename, dpi = 400)
        else:
            #print(paths_dir)
            os.mkdir(paths_dir)
            os.chdir(paths_dir)
            plot_dir = ab_file.replace(".ab", "_perturbed_dir%s.png"%(count))
            if noisy:
                plot_dir = noisy_ab_file.replace(".ab", "_noisy_perturbed_dir%s.png"%(count))
            filename = os.path.join(os.getcwd(), plot_dir)
            plt.savefig(filename, dpi = 400)
        plt.close()
        os.chdir(current_directory)
        count+=1






parser = argparse.ArgumentParser(description = 'options for perturbing initial parameters and overwriting files')

parser.add_argument('--perturb', help = "perturb initial parameters by random vector", action = "store_true")
parser.add_argument('--overwrite', help = "overwrite saved plots", action = "store_true")
parser.add_argument('--hardware', help = "specify which hardware to use")
parser.add_argument('--shots', help = "nshots to run on your experiment, default is 8192", action = "store")
parser.add_argument('--dir', help = "pass a qasm file or directory of qasm files to process, otherwise recursively search through all sub directories for .qasm files", action = "store")
parser.add_argument('--loss', help ="Pass either 'mmd' or 'js' for js or mmd losses respectively", action = "store")
parser.add_argument('--noisy', help = "Pass noisy if you wish to use noisy simulation", action = "store_true")
parser.add_argument('--rigetti', help = "Pass if you wish to run on Rigetti")
parser.add_argument('--backend', help = "Pass string of IBM backend you wish to use. Default is 'ibmq_qasm_simulator' ", action = "store")

parser.add_argument("--plot", help = "pass plot if you want to plot the data", action = "store_true")

parser.add_argument("-v", help = "verbose output of ddcl iterations", action = "store_true")
args = parser.parse_args()
nshots = 8192
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
        #I think this is the best place to start multiprocessing, I can spawn a new
        #worker to work on each file for the number of logical cores the system this is
        #running on has. I could do something like:
        #also getting set up on cades would be really primo for this, since I only have four
        #cores to run on locally.
        num_cores = multiprocessing.cpu_count() - 1

        for dir in directories:
            os.chdir(dir)
            #print("CURRENTLY IN DIR", args.dir)
            files = glob.glob("*.qasm")
            #print("FILES = ", files)
            for file in files:
                if file.startswith("preprocessed"):
                    print("ignoring file: ", file)
                    continue
                elif file.startswith("xacc"):
                    print("ignoring file:", file)
                    continue
                else:
                    #print("process file: ", file)
                    start_params, filename, nbits = parseQASM(file)

                if perturb:
                    delta = .01*np.random.normal()
                    start_params += delta
                    circuit = runCircuit(filename, nbits, initial_parameters = start_params)
                    os.chdir(GLOBAL_ROOT)
                else:
                    print("filename = ", filename)
                    print("running file %s"%(filename))
                    if args.backend:
                        runCircuit(filename, nbits, initial_parameters = start_params,\
                                             loss = args.loss, noisy = args.noisy,\
                                             backend = args.backend)
                    else:
                        runCircuit(filename, nbits, initial_parameters = start_params, \
                                             loss = args.loss, noisy = args.noisy)
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
                    circuit = runCircuit(filename, nbits, initial_parameters = start_params, noisy = True)
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
