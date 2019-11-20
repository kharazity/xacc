import xacc
import os
import glob
import csv
import matplotlib.pyplot as plt
import pandas as pd

#helper functions
def sortTuples(tup):
    return(sorted(tup, key = lambda x : x[1], reverse = True))

#wraps the text of the parameter array to be more manageable
def cleanLabelArrays(params):
    parameters = []
    for j in range(len(params)):
        index = 0
        paramstring = str(params[j])
        #print("paramstring = ", paramstring)
        end = len(paramstring)
        for i in range(end):
            if paramstring[i] == ",":
                index += 1;
                if (index%4) == 0:
                    temp = paramstring[0:i]+',\n'+paramstring[i+1:end]
                    temp1 = map(str, temp)
                    paramstring = ''.join(temp1)
                    paramstring = paramstring + "]"
        parameters.append(paramstring)

    #print("RETURNING: ", parameters)
    return parameters


def generatePlots(losses, params, file, optimizer_dict = {}, ddcl_dict = {}):
    loss = losses
    temp = [x for x in range(len(losses))]
    tup = list(zip(temp,loss))
    x = [x[0] for x in tup]
    y = [x[1] for x in tup]
    #collects top 5 y values and returns (x,y) pairs for later processing
    sortedtup = sortTuples(tup)
    top5 = sortedtup[0:5]
    top5.append(tup[-1])
    indices = [x[0] for x in top5]
    parameters = params
    #shortens parameters to only the values that we care about 
    params = [parameters[i] for i in indices]
    params.append(parameters[-1])
    fig, ax = plt.subplots(2, 1, gridspec_kw={'height_ratios': [3,1]})
    fig.subplots_adjust(hspace = 0.2,bottom = .02)
    ax[0].plot(x,y)
    ax[0].set_title(file)
    ax[0].set(xlabel = "training step", ylabel = "loss")
    plt.figtext(0,0.8, "final loss = %s" %(losses[-1]))
    labels = cleanLabelArrays(params)
    counter = 0
    if optimizer_dict:
        print("optimizer_dict was found!")
        counter = -3
        ax[0].text(0, -1, "loss = %s" %(optimizer_dict['loss']))
        ax[0].text(0, -2, "target = %s"%(optimizer_dict['target_dist']))
    if ddcl_dict:
        print("DDCL_dict was found!")
        for k in ddcl_dict.keys():
            ax[0].text(0, counter, "%s = %s"%(k, ddcl_dict[k]))
            counter -= 1

    for j in range(len(top5)):
        ax[0].annotate(j, top5[j])
        ax[1].set_xticklabels([])
        ax[1].set_yticklabels([])
        ax[1].axis('off')
        if len(parameters[0]) > 20:
            for k in range(len(top5)):
                ax[1].text(0, 1.3*(-k)-1,str(k)+" params: "+str(labels[k]))

        else:
            for k in range(len(params)):
                ax[1].text(0, .7*(-k)-1,str(k)+" params: "+str(labels[k]))
    plt.savefig(file+".pdf", bbox_inches = "tight",  dpi = 400)
    plt.close()

def processFile(file, loss):
    losses = []
    gradients = []

    print("~~~~~~~~~~~Reading from %s ~~~~~~~~~~~~"%(file))
    # Load the AcceleratorBuffer
    buffer = xacc.loadBuffer(open(file, 'r').read())

    # You can get all unique information at given key,
    # here let's get all unique parameter sets the problem
    # was run at
    parameters = buffer.getAllUnique('parameters')
    losses.append(buffer.getAllUnique('loss'))
    losses = losses[0]
    losses.sort(reverse = True)
    print(losses)
    gradients.append(buffer.getAllUnique('gradient'))
    gradients = gradients[0]
    params = parameters

    current_dir = os.path.dirname(os.path.abspath(file))
    file_losses = file.replace(".ab", "_losses_.csv")
    file_gradients = file.replace(".ab", "_gradients.csv")
    file_params = file.replace(".ab", "_params.csv")
    writeToCSV(losses, file_losses)
    writeToCSV(params, file_params)
    writeToCSV(gradients, file_gradients)
    file_training = file.replace(".ab", "_training_steps")
    generatePlots(losses, params, file_training)
    os.chdir(os.getcwd()+"/../")
    

def writeToCSV(data, file):
    csv_columns = ["step", "result"]
    with open(file, 'w') as csvfile:
        counter = 0
        for x in data:
            csvfile.write("%s, %s \n"%(counter, x))
            counter += 1
        csvfile.close()

#processFile("xacc_3qubit_depth1target0.ab")


#processFile("2qbit/target1/xacc_2qubit_depth2_mmd.ab")


