'''
This module should take in a .qasm file and return a file that is compatible with xacc
'''

import re


def parseQASM(qasm_file):
    num_params = 0
    num_bits = 0
    initial_params = []
    processed_file = "xacc_" + qasm_file
    with open(qasm_file, 'r') as file:
        xacc_file = open(processed_file, "w+")
        for line in file.readlines():
            if line.startswith('qreg'):
                #find the number of qubits
                for char in line:
                    if char.isdigit():
                        num_bits = int(char)
                        print("num_bits = ", num_bits)
                        break

            #replace u1(val, 0, 0) q[i];
            #with U()
            elif line.startswith('u1'):
                #need to grab the qubit number
                temp = line
                vals = re.findall("\d+\.\d+", line)
                m = re.search(r"\[(\d)\]", line)
                qbit = m.group(1)
                for val in vals:
                    initial_params.append(float(val))
                    xacc_file.write("U(q[%s], 0, 0, x[%s]);\n"%(qbit, num_params))
                    num_params+=1

            #replace u3(val, 0, 0)
            elif line.startswith('u3'):
                #need to grab the qubit number as well
                temp = line
                vals = re.findall("\d+\.\d+", line)
                m = re.search(r"\[(\d)\]", line)
                qbit = m.group(1)
                for val in vals:
                    initial_params.append(float(val))
                    xacc_file.write("U(q[%s], x[%s], -pi/2, pi/2);\n"%(qbit, num_params))
                    num_params+=1


            elif line.startswith('cx'):
                #grab the first and second qubits
                temp = line.split(',')
                qbits = []
                for x in temp:
                    m = re.search(r"\[(\d)\]", x)
                    qbits.append(m.group(1))
                xacc_file.write("CNOT(q[%s], q[%s]);\n"%(qbits[0], qbits[1]))

        file.close()
        xacc_file.close()
        return initial_params, processed_file, num_bits

