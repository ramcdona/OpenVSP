import openvsp as vsp
import math
import Constants as const
from pathlib import Path
from bokeh.models.ranges import Range1d 
from bokeh.models import LinearAxis
from bokeh.plotting import figure, show
from bokeh.io import export_png
from bohek_helper import make_table
import pickle

scriptpath = str(Path(__file__).parent.resolve())


class SupersonicDeltaWingTest:
    '''!Class for running and collecting data from the
        SupersonicDeltaWing studies
    '''
    def __init__(self):
        self.m_Sweep = [45,65]
        # Note m_SuperMachVec[3] = 2.366 will cause VSPAERO to crash for the 65° swept delta wing
        self.m_SuperMachVec = [1.1347,1.4662,1.8939,2.3662,2.8611,3.3686,3.8838,4.4040] 
        
        self.Cl_alpha_tan_sweep = [[0.0]*len(self.m_SuperMachVec) for i in range(len(self.m_Sweep))]
        self.M_sweep_fun = [[0.0]*len(self.m_SuperMachVec) for i in range(len(self.m_Sweep))]
        
        self.m_exp = [0.192469, 0.312413, 0.253835, 0.298466, 0.354254, 0.538354, 0.496513, 0.415621, 0.468619, 0.64993, 0.714086, 0.725244, 0.803347, 0.88424, 0.956764, 1.0265, 1.07392, 1.18271, 1.38633, 1.43654, 1.91632]
        self.Cl_alpha_tan_sweep_exp = [5.81731, 6.45192, 5.71635, 5.64423, 5.47115, 5.65865, 5.45673, 5.25481, 5.06731, 5.02404, 4.99519, 4.57692, 4.17308, 4.38942, 3.71154, 3.61058, 3.35096, 3.13462, 2.37019, 2.61538, 2.02404]
        

#========================================= Supersonic Delta Wing Functions =================================#
#==================== Generates the relavent parameteres. Runs the ____________      =============#
#==================== ___________________ studies. Generates the ___ tables and      =============#
#==================== _____ charts charts to include in the markdown file.           =============#
#=================================================================================================#

#========== Wrapper function for ________________________________ Code ===========================#
    def SupersonicDeltaWingStudy(self):
        self.GenerateSupersonicDeltaWing()
        self.TestSupersonicDeltaWing()
        self.GenerateSupersonicDeltaWingCharts()
        
            
#===================== Sweapt UWTess Generation Functions =====================
    def GenerateSupersonicDeltaWing(self):
        
        #==== Add Wing Geometry ====#
        wing_id = vsp.AddGeom( 'WING', '' )
        
        #==== Set Wing Section ====#
        vsp.SetDriverGroup( wing_id, 1, vsp.SPAN_WSECT_DRIVER, vsp.ROOTC_WSECT_DRIVER, vsp.TIPC_WSECT_DRIVER )
        
        vsp.Update()
        
        #==== Set Airfoil to NACA 0012 Airfoil and Set Common Parms====//
        vsp.SetParmVal( wing_id, 'ThickChord', 'XSecCurve_0', 0.04 )
        vsp.SetParmVal( wing_id, 'ThickChord', 'XSecCurve_1', 0.06 )
        vsp.SetParmVal( wing_id, 'Sweep_Location', 'XSec_1', 0 )
        vsp.SetParmVal( wing_id, 'Sec_Sweep_Location', 'XSec_1', 1 )
        vsp.SetParmVal( wing_id, 'Tip_Chord', 'XSec_1', 1 )
        vsp.SetParmVal( wing_id, 'Span', 'XSec_1', 10.00004 )
        vsp.SetParmVal( wing_id, 'SectTess_U', 'XSec_1', 30 )
        vsp.SetParmVal( wing_id, 'TECluster', 'WingGeom', 1.0 )
        vsp.SetParmVal( wing_id, 'LECluster', 'WingGeom', 1.0 )

        vsp.Update()

        for s in self.m_Sweep:
            vsp.SetParmVal( wing_id, 'Sweep', 'XSec_1', s )
            if s == 45:
                vsp.SetParmVal(wing_id,'Root_Chord', 'XSec_1',11)
            elif s == 65:
                vsp.SetParmVal(wing_id,'Root_Chord', 'XSec_1',22.39583)
            vsp.Update()    
            
            for m in self.m_SuperMachVec:    
                #==== Setup export filenames ====#
                fname = scriptpath + '/supersonic_files/vsp_files/Supersonic_Delta_Wing_Sweep' + str(s) + '_Mach' + str(m) + '.vsp3'

                #==== Save Vehicle to File ====#
                message = '-->Saving vehicle file to: ' + fname + '\n'
                print(message )
                vsp.WriteVSPFile( fname, vsp.SET_ALL )
                print( 'COMPLETE\n' )
        vsp.ClearVSPModel()

#========== Run the actual ____________ Studies ==============================#
    def TestSupersonicDeltaWing(self):

        print( '-> Begin SupersonicDeltaWing Test:\n' )
        
        
        num_sweep = len(self.m_Sweep)
        num_mach = len(self.m_SuperMachVec)
            
            
        for s in range(num_sweep):
            for m in range(num_mach):
                          
                #==== Open and test generated wings ====#
                fname = scriptpath + '/supersonic_files/vsp_files/Supersonic_Delta_Wing_Sweep' + str(self.m_Sweep[s]) + '_Mach' + str(self.m_SuperMachVec[m]) + '.vsp3'
                fname_res = '/supersonic_files/vsp_files/Supersonic_Delta_Wing_Sweep' + str(s) + '_Mach' + str(m) + '_res.csv'

                print( 'Reading in file: ')
                print( fname )
                vsp.ReadVSPFile( fname ) # Sets VSP3 file name

                #==== Analysis: VSPAeroSinglePoint ====#
                print( const.m_VSPSingleAnalysis )

                #==== Analysis: VSPAero Compute Geometry to Create Vortex Lattice DegenGeom File ====#
                print( const.m_CompGeomAnalysis )

                # Set defaults
                vsp.SetAnalysisInputDefaults( const.m_CompGeomAnalysis )
                
                vsp.SetIntAnalysisInput(const.m_CompGeomAnalysis, 'Symmetry', const.m_SymFlagVec, 0)
                
                # list inputs, type, and current values
                vsp.PrintAnalysisInputs( const.m_CompGeomAnalysis )

                # Execute
                print( '\tExecuting...' )
                compgeom_resid = vsp.ExecAnalysis( const.m_CompGeomAnalysis )
                print( 'COMPLETE' )

                # Get & Display Results
                vsp.PrintResults( compgeom_resid )

                #==== Analysis: VSPAero Single Point ====#
                # Set defaults
                vsp.SetAnalysisInputDefaults(const.m_VSPSingleAnalysis)
                print(const.m_VSPSingleAnalysis)

                # Reference geometry set
                vsp.SetIntAnalysisInput(const.m_VSPSingleAnalysis, 'GeomSet', const.m_GeomVec, 0)
                vsp.SetIntAnalysisInput(const.m_VSPSingleAnalysis, 'RefFlag', const.m_GeomVec, 0)
                vsp.SetIntAnalysisInput(const.m_VSPSingleAnalysis, 'Symmetry', const.m_GeomVec, 0)

                wid = vsp.FindGeomsWithName('WingGeom')
                vsp.SetStringAnalysisInput(const.m_VSPSingleAnalysis, 'WingID', wid, 0)
                
                # Freestream Parameters
                Alpha = [5.0]
                Mach = [self.m_SuperMachVec[m]]
                vsp.SetDoubleAnalysisInput(const.m_VSPSingleAnalysis, 'Alpha', Alpha, 0)
                vsp.SetDoubleAnalysisInput(const.m_VSPSingleAnalysis, 'Mach', Mach, 0)
                vsp.SetIntAnalysisInput(const.m_VSPSingleAnalysis, 'WakeNumIter', const.m_WakeIterVec, 0)

                vsp.Update()

                # list inputs, type, and current values
                vsp.PrintAnalysisInputs( const.m_VSPSingleAnalysis )
                print( '' )

                # Execute
                print( '\tExecuting...' )
                rid = vsp.ExecAnalysis( const.m_VSPSingleAnalysis )
                print( 'COMPLETE' )

                # Get & Display Results
                vsp.PrintResults( rid )
                vsp.WriteResultsCSVFile( rid, fname_res )
                
                rid_vec = vsp.GetStringResults(rid, 'ResultsVec')
                if len(rid_vec) > 0:
                    cl_vec = vsp.GetDoubleResults(rid_vec[0], 'CL')
                    Cl_res = cl_vec[-1]
                    Cl_alpha_vsp = Cl_res / math.radians(Alpha[0]) 
                    
                    self.Cl_alpha_tan_sweep[s][m] = Cl_alpha_vsp * math.tan(math.radians(self.m_Sweep[s]))
                    self.M_sweep_fun[s][m] = math.sqrt((math.pow(self.m_SuperMachVec[m],2)) - 1) / math.tan(math.radians(self.m_Sweep[s]))
                vsp.ClearVSPModel()    
                    
                    
#======== Use Bokeh to Create tables and Graphs for the _________ Studies =#
    def GenerateSupersonicDeltaWingCharts(self):
        title = 'Supersonic Delta Wing Geometry Setup'
        header = ['Case #','Root Airfoil','Tip Airfoil','Span','Root Chord','Λ (°)', 'Λ Location','Span Tess (U)', 'Chord Tess (W)']
        data = [['1','2'], ['NACA004']*2, ['NACA006']*2,['20']*2,['11.0']*2,['45','65'],['0.0']*2,['30']*2,['33']*2]
        data_table = make_table(header,data)
        export_png(data_table,filename=scriptpath + '/supersonic_files/supersonic_img/supersonic/geometrysetup.png')
        
        title = 'Supersonic Delta Wing VSPAERO Setup'
        header = ['Analysis', 'Method', 'α (°)', 'β (°)', 'M', 'Wake Iterations']
        data = [['Single Point'], ['VLM'], ['5.0'],['0.0'],['1.135,1.366,1.894,2.386,2.861,3.369,3.884,4.404'],['3']]
        data_table = make_table(header,data)
        export_png(data_table,filename=scriptpath + '/supersonic_files/supersonic_img/supersonic/vspaerosetup.png')
        
        p = figure(width=const.bokehwidth,height=const.bokehheight, title='Supersonic Delta Wing: Cl_alpha*tan(sweep) = f(m)',x_axis_label='m', y_axis_label='Cl_alpha*tan(sweep)')
        for a in range(len(self.m_Sweep)):
            p.circle(self.M_sweep_fun[a],self.Cl_alpha_tan_sweep[a], legend_label='VSPAERO '+str(self.m_Sweep[a])+'° Sweep', color=const.bokehcolors[a],size=const.bokehsize)
            p.line(self.M_sweep_fun[a],self.Cl_alpha_tan_sweep[a], legend_label='VSPAERO '+str(self.m_Sweep[a])+'° Sweep',color=const.bokehcolors[a],line_width=const.bokehlinewidth)
            print(self.Cl_alpha_tan_sweep)
            print('/////////////////////////////')


        p.circle(self.m_exp,self.Cl_alpha_tan_sweep_exp, legend_label='Experimental Data',color=const.bokehcolors[-1],size=const.bokehsize)
        p.add_layout(p.legend[0],'right')
        p.y_range.start=0
        export_png(p,filename=scriptpath + '/supersonic_files/supersonic_img/supersonic/only.png')


    
        
    
def runSupersonicDeltaWingstudy(mode = 3):
    setup_filepaths()

    
    test = SupersonicDeltaWingTest()
    if (mode == 1 or mode == 2):
        with open(scriptpath+'/supersonic_files/supersonictest.pckl','rb') as picklefile:    
            test = pickle.load(picklefile)
    if (mode == 1): 
        test.GenerateSupersonicDeltaWingCharts()
    if (mode == 3):
        test.SupersonicDeltaWingStudy()
        with open(scriptpath+'/supersonic_files/supersonictest.pckl','wb') as picklefile:
            pickle.dump(test,picklefile)
    if (mode == 2):
        test.TestSupersonicDeltaWing()
        test.GenerateSupersonicDeltaWingCharts()
        with open(scriptpath+'/supersonic_files/supersonictest.pckl','wb') as picklefile:
            pickle.dump(test,picklefile)        
            
            
def setup_filepaths():
    scriptpathlib = Path(__file__).parent.resolve()
    testnames = ['supersonic_files/']
    subnames = [['supersonic_img/','vsp_files/']]
    subsubnames = [[['supersonic'],['']]]
    for i in range(len(testnames)):
        for j in range(len(subnames[i])):
            for k in range(len(subsubnames[i][j])):
                dirname = Path.joinpath(scriptpathlib, testnames[i]+subnames[i][j]+subsubnames[i][j][k])
                dirname.mkdir(parents=True, exist_ok=True)
                
if __name__ == '__main__':
    runSupersonicDeltaWingstudy(mode = 3)    
