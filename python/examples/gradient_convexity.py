import xacc
import json
import os
from os import walk
import glob
import numpy as np
import matplotlib.pyplot as plt
base_path = "/Users/6tk/Desktop/qasm_dir/"
json_files = []
noisy_json_files = []


def calculate_grad(losses, noisy_losses):
    loss = list(losses.copy().values())
    noisy_loss = list(noisy_losses.copy().values())
    grad = []
    noisy_grad = []
    for k in range(len(loss)):
        #print(k)
        if k < 2:
            continue
        elif k > (len(loss) - 2):
            continue
        else:

            grad.append((loss[k+1]-loss[k-1])/(2*delta_x))
            noisy_grad.append((noisy_loss[k+1] - noisy_loss[k-1])/(2*delta_x))

    x_vals = list(losses.keys())[1:len(loss)-2]
    plt.plot(x_vals, grad)
    plt.show()
    noisy_x_vals = list(noisy_losses.keys())[1:len(loss)-2]
    plt.plot(noisy_x_vals, noisy_grad)
    plt.show()
        



for (dirpath, dirnames, filenames) in walk(base_path):
    for file in filenames:
        if file.endswith(".json"):
            if file.startswith("noisy"):
                noisy_json_files.append(os.path.join(dirpath, file))
            else:
                json_files.append(os.path.join(dirpath,file))

print(json_files)
print(noisy_json_files)
#parse through all directories below and grad all *.json files

if(len(json_files) != len(noisy_json_files)):
    print("json_files and noisy_json_files of different lengths, terminating")
    print("len(json_files) = ", len(json_files))
    print("len(noisy_json_files = )", len(noisy_json_files))
    print("json_files = ", json_files)
    print("noisy_json_files = ", noisy_json_files)

for i in range(len(json_files)):
    with open(json_files[i], "r") as read_file:
        losses = json.load(read_file)

    with open(noisy_json_files[i], "r") as read_file:
        noisy_losses = json.load(read_file)

    if(len(noisy_losses) != len(losses)):
        print("losses not of same lengths")
        print("len(noisy_losses) = ", len(noisy_losses))
        print("len(losses) = ", len(losses))
    else:
        delta_x = 2*np.pi/200 #from np.linspace(-pi, pi, num = 200)
        for j in range(len(noisy_losses)):

            calculate_grad(losses[j], noisy_losses[j])


