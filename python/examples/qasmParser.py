import re
qasm_file = "bas2x2_depth2_CL.qasm"
i = 0;
x = []
print(qasm_file)
file_name = "preprocessed_" + qasm_file
with open(qasm_file, 'r') as file:
    processed_file = open(file_name, "w+")
    for line in file.readlines():
        if line.startswith("u"):
            temp = line
            val = re.findall("\d+\.\d+", temp)
            #print(val[0])
            x.append(float(val[0]))
            temp = temp.replace(val[0], "x[%s]"%(i))
            #print(str(val[0]))
            #print(temp)
            i += 1
            processed_file.write(temp)

        elif line.startswith("measure"):
            continue
        else:
            processed_file.write(line)
    file.close()

print(x)

