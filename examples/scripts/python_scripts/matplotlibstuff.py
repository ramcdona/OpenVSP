import numpy as np
import matplotlib.pyplot as plt
from pathlib import Path
scriptpath = str(Path(__file__).parent.resolve())



def main():
    m_exp = [0.192469, 0.312413, 0.253835, 0.298466, 0.354254, 0.538354, 0.496513, 0.415621, 0.468619, 0.64993, 0.714086, 0.725244, 0.803347, 0.88424, 0.956764, 1.0265, 1.07392, 1.18271, 1.38633, 1.43654, 1.91632]
    Cl_alpha_tan_sweep_exp = [5.81731, 6.45192, 5.71635, 5.64423, 5.47115, 5.65865, 5.45673, 5.25481, 5.06731, 5.02404, 4.99519, 4.57692, 4.17308, 4.38942, 3.71154, 3.61058, 3.35096, 3.13462, 2.37019, 2.61538, 2.02404]
    m = np.array(m_exp)
    n = np.array(Cl_alpha_tan_sweep_exp)
    
    fig, ax = plt.subplots()
    ax.plot(m,n+1, 'o-',color='green',label='something vsp')
    ax.set_title('Openvsp plot')
    ax.plot(m_exp,Cl_alpha_tan_sweep_exp,color='red', label='something analysis')
    ax.set_xlabel('label for x')
    ax.set_ylabel('label for y')
    #ax.legend(bbox_to_anchor=(1.05,1),loc='center left')
    ax.legend(bbox_to_anchor=(.5,-.1),loc='upper center', ncols=10)
    fig.savefig(scriptpath+'/matplotlibfigtest.svg', bbox_inches='tight')

if __name__ == "__main__":
    main()