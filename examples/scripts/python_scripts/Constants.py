#Constants pulled from V&V Base Class in Master_VSP_VV_Script.vspscript

#TABLE CONSTANTS
STUDY_SETUP_TABLE_HEADER =["Case #","Analysis","Method","alpha (°)","beta (°)","M","Wake Iterations"]

#STRING CONSTANTS
m_VSPSweepAnalysis = "VSPAEROSweep"
m_CompGeomAnalysis = "VSPAEROComputeGeometry"
m_VSPSingleAnalysis = "VSPAEROSinglePoint"

#VECTOR CONSTANTS
m_GeomVec = [0]
m_AlphaVec = [1.0]
m_MachVec = [0.1]
m_SymFlagVec = [1]
m_RefFlagVec = [1]; # Wing Reference
m_WakeIterVec = [3]

#Bokeh graph constants
bokehcolors = ["blue","red","gold","green","purple", "skyblue","gray"] #Bokeh uses css color names https://www.w3schools.com/colors/colors_names.asp
bokehlinewidth = 3
bokehsize = 5
bokehaspectratio = 2        

#CONSTANTS
b = 0.9949874371; # M = 0.1, b = (1-M^2)^0.5
k_theo = 1.0