import xacc
import os #for finding qasm files
import glob #recursive finding of qasm_files
import argparse #command line arguments
import numpy as np #because science
import csv #save params and losses
from targets import target_dict
from parseQASM import parseQASM
from data_process import processFile
from multiprocessing import pool

GLOBAL_ROOT = os.getcwd()

parser = argparse.ArgumentParser(description = '')

num_cores = multiprocessing.cpu_count() - 1;

parser.add_argument('--perturb', help = "perturb initial parameters by random vector", action = "store_true")
parser.add_argument('--overwrite', help = "overwrite saved plots FIXME", action = "store_true")
parser.add_argument('--hardware', help = "specify which hardware to use")
parser.add_argument('--shots', help = "nshots to run on your experiment, default is 8192", action = "store")
parser.add_argument('--dir', help = "pass a qasm file or directory of qasm files to process, otherwise recursively search through all sub directories for .qasm files", action = "store")
parser.add_argument('--loss', help ="Pass either 'mmd' or 'js' for js or mmd losses respectively", action = "store")
parser.add_argument('--noisy', help = "Pass noisy if you wish to use noisy simulation", action = "store_true")
parser.add_argument('--rigetti', help = "Pass if you wish to run on Rigetti hardware FIXME")
parser.add_argument('--backend', help = "Pass string of IBM backend you wish to use. Default is 'ibmq_qasm_simulator' ", action = "store")
parser.add_argument("--plot", help = "pass plot if you want to plot the data", action = "store_true")
parser.add_argument("-v", help = "verbose output of ddcl iterations", action = "store_true")
parser.add_argument("-EM", help = "Error mitigation of noisy circuits FIXME", action = "store_true")
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

if args.EM:
    if !args.noisy:
        print("EM passed without noise simulation turned on, turning off error mitigation")
        args.EM = False;

def getCircuitData(directories):
    
    for dir in directories:
        os.chdir(dir)
        files = glob.glob("*.qasm")
        for file in files:
            if file.startswith("xacc"):
                print("ignoring file: ", file)
                continue
            else:
                data.append(parseQASM(file));

def getListOfDirs(dir_name):
    file_list = os.listdir(dir_name);
    all_dirs = list()
    for file in file_list:
        full_path = os.path.join(dir_name, file)
        if os.path.isdir(full_path):
            all_dirs.append(full_path)
            all_dirs = all_dirs + getListOfDirs(full_path)
            #maybe I should generate 3 lists of start_params, filenames, num_bits, and I could multiprocess over the list(s)
            #if so I should do it here, which would imply knowing what this does just a tad bit better
    return all_dirs


def trainCircuit():
    #should probably take in the qasm_file and booleans for running with noise/EM and should ONLY do the training portion of
    #the code
    return;

def generatePlots():

    return;

