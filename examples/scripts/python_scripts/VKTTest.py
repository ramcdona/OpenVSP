import openvsp as vsp
import math
import Constants as const
from pathlib import Path
from bokeh.plotting import figure, show
from bokeh.io import export_png
from bohek_helper import make_table
import pickle

class VKTTest:
    '''!Class for running and collecting data from the
        Von Karman-Trefftz studies
    '''
    def __init__(self):
        self.m_AlphaNpts = 9
        self.m_Cl_alpha_expected = 2.743 # rad
        self.m_Cm_alpha_expected = -3.10 # rad
        
        self.alpha_0 = -20.0 # deg
        self.alpha_f = 20.0 # deg
        
        self.m_Cl_alpha_error = [0.0]*self.m_AlphaNpts
        self.m_Cl_alpha_res = [0.0]*self.m_AlphaNpts
        self.m_Cm_alpha_error = [0.0]*self.m_AlphaNpts
        self.m_Cm_alpha_res = [0.0]*self.m_AlphaNpts
        self.m_AlphaSweepVec = [0.0]*self.m_AlphaNpts
        
        self.Cl_res = [0.0]*self.m_AlphaNpts
        self.Cm_res = [0.0]*self.m_AlphaNpts
        self.Cl_approx_vec = [0.0]*self.m_AlphaNpts
        self.Cm_approx_vec = [0.0]*self.m_AlphaNpts

        

#========================================= Von Karman-Trefftz Functions =================================#
#==================== Generates the relavent parameteres. Runs the ____________      =============#
#==================== ___________________ studies. Generates the ___ tables and      =============#
#==================== _____ charts charts to include in the markdown file.           =============#
#=================================================================================================#

#========== Wrapper function for ________________________________ Code ===========================#
    def VKTStudy(self):
        self.GenerateVKTWings()
        self.TestVKTWings()
        self.GenerateVKTCharts()
#===================== Sweapt UWTess Generation Functions =====================
    def GenerateVKTWings(self):
        #INSERT lines 5159 - 5188 from v&v script
        #==== Add Wing Geometry ====#
        wing_id = vsp.AddGeom( 'WING', '' )

        #==== Set Wing Section Controls to AR, Root Chord, and Tail Chord ====#
        vsp.SetDriverGroup( wing_id, 1, vsp.AR_WSECT_DRIVER, vsp.ROOTC_WSECT_DRIVER, vsp.TIPC_WSECT_DRIVER )
        
        vsp.Update()

        #==== Set Airfoil to Von Karman-Trefftz Wing Parms ====#
        vsp.SetParmVal( wing_id, 'ThickChord', 'XSecCurve_0', 0.12 )
        vsp.SetParmVal( wing_id, 'ThickChord', 'XSecCurve_1', 0.12 )
        vsp.SetParmVal( wing_id, 'Sweep_Location', 'XSec_1', 0 )
        vsp.SetParmVal( wing_id, 'Sweep', 'XSec_1', 53.54 )
        vsp.SetParmVal( wing_id, 'Root_Chord', 'XSec_1', 1.5 )
        vsp.SetParmVal( wing_id, 'Tip_Chord', 'XSec_1', 0.5 )
        vsp.SetParmVal( wing_id, 'Aspect', 'XSec_1', math.sqrt(2) )

        vsp.Update()

        #==== Setup export filenames for VSPAERO sweep ====#
        fname = 'vkt_files/vsp_files/vkt.vsp3'

        #==== Save Vehicle to File ====#
        print('-->Saving vehicle file to: ' + fname + '\n' )
        vsp.WriteVSPFile( fname, vsp.SET_ALL )
        print( 'COMPLETE\n' )

        vsp.ClearVSPModel()

#========== Run the actual ____________ Studies ==============================#
    def TestVKTWings(self):
        #Insert lines 5159-5377 from v&v script
        print( '-> Begin VKT Sweep Test:\n' )
        
        
        d_alpha = self.alpha_f - self.alpha_0 # deg
        
        
        #==== Open and test generated wing ====#
        fname = 'vkt_files/vsp_files/vkt.vsp3'
        fname_res = 'vkt_files/vsp_files/vkt_res.csv'

        print( 'Reading in file: ')
        print( fname )
        vsp.ReadVSPFile( fname ) # Sets VSP3 file name

        #==== Analysis: VSPAero Sweep ====#
        print( const.m_VSPSweepAnalysis )

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

        #==== Analysis: VSPAero Sweep ====#
        # Set defaults
        vsp.SetAnalysisInputDefaults(const.m_VSPSweepAnalysis)
        print(const.m_VSPSweepAnalysis)

        # Reference geometry set
        vsp.SetIntAnalysisInput(const.m_VSPSweepAnalysis, 'GeomSet', const.m_GeomVec, 0)
        vsp.SetIntAnalysisInput(const.m_VSPSweepAnalysis, 'RefFlag', const.m_RefFlagVec, 0)
        vsp.SetIntAnalysisInput(const.m_VSPSweepAnalysis, 'Symmetry', const.m_SymFlagVec, 0)

        wid = vsp.FindGeomsWithName( 'WingGeom' )
        vsp.SetStringAnalysisInput(const.m_VSPSweepAnalysis, 'WingID', wid, 0)
        
        vsp.SetIntAnalysisInput(const.m_VSPSweepAnalysis, 'WakeNumIter', const.m_WakeIterVec, 0)

        # Freestream Parameters
        AlphaStart = [self.alpha_0]
        AlphaEnd = [self.alpha_f]
        AlphaNpts = [self.m_AlphaNpts]
        vsp.SetDoubleAnalysisInput(const.m_VSPSweepAnalysis, 'AlphaStart', AlphaStart, 0)
        vsp.SetDoubleAnalysisInput(const.m_VSPSweepAnalysis, 'AlphaEnd', AlphaEnd, 0)
        vsp.SetIntAnalysisInput(const.m_VSPSweepAnalysis, 'AlphaNpts', AlphaNpts, 0)
        MachNpts = [1] # Start and end at 0.1
        vsp.SetDoubleAnalysisInput(const.m_VSPSweepAnalysis, 'MachStart', const.m_MachVec, 0)
        vsp.SetDoubleAnalysisInput(const.m_VSPSweepAnalysis, 'MachEnd', const.m_MachVec, 0)
        vsp.SetDoubleAnalysisInput(const.m_VSPSweepAnalysis, 'MachNpts', MachNpts, 0)

        vsp.Update()

        # list inputs, type, and current values
        vsp.PrintAnalysisInputs( const.m_VSPSweepAnalysis )
        print( '' )

        # Execute
        print( '\tExecuting...' )
        rid = vsp.ExecAnalysis( const.m_VSPSweepAnalysis )
        print( 'COMPLETE' )

        # Get & Display Results
        vsp.PrintResults( rid )
        vsp.WriteResultsCSVFile( rid, fname_res )

        # Get Result ID Vec (History and Load ResultIDs)
        rid_vec = vsp.GetStringResults( rid, 'ResultsVec' )
        if ( len(rid_vec) <= 1 ):
            vsp.ClearVSPModel()
            return
            
        # Get Result from Final Wake Iteration
        for i in range(self.m_AlphaNpts) :
        
            alpha_vec = vsp.GetDoubleResults( rid_vec[i], 'Alpha' )
            self.m_AlphaSweepVec[i] = alpha_vec[len(alpha_vec) - 1]
            
            cl_vec = vsp.GetDoubleResults( rid_vec[i], 'CL' )
            self.Cl_res[i] = cl_vec[len(cl_vec) - 1]
            
            cmy_vec = vsp.GetDoubleResults( rid_vec[i], 'CMy' )
            self.Cm_res[i] = cmy_vec[len(cmy_vec) - 1]
            
            self.Cl_approx_vec[i] = self.m_Cl_alpha_expected * math.sin( math.radians( self.m_AlphaSweepVec[i]))
            self.Cm_approx_vec[i] = self.m_Cm_alpha_expected * math.sin( math.radians( self.m_AlphaSweepVec[i]))
        
        
        for i in range(self.m_AlphaNpts):
        
            if ( i == 0 ):
            
                self.m_Cl_alpha_res[i] = math.degrees((self.Cl_res[i+1] - self.Cl_res[i])/(self.m_AlphaSweepVec[i+1] - self.m_AlphaSweepVec[i]))
                self.m_Cm_alpha_res[i] = math.degrees((self.Cm_res[i+1] - self.Cm_res[i])/(self.m_AlphaSweepVec[i+1] - self.m_AlphaSweepVec[i]))
            
            elif ( i == self.m_AlphaNpts - 1 ):
            
                self.m_Cl_alpha_res[i] = math.degrees((self.Cl_res[i] - self.Cl_res[i-1])/(self.m_AlphaSweepVec[i] - self.m_AlphaSweepVec[i-1]))
                self.m_Cm_alpha_res[i] = math.degrees((self.Cm_res[i] - self.Cm_res[i-1])/(self.m_AlphaSweepVec[i] - self.m_AlphaSweepVec[i-1]))
            
            else :# Central differencing
            
                self.m_Cl_alpha_res[i] = math.degrees((self.Cl_res[i+1] - self.Cl_res[i-1])/(self.m_AlphaSweepVec[i+1] - self.m_AlphaSweepVec[i-1]))
                self.m_Cm_alpha_res[i] = math.degrees((self.Cm_res[i+1] - self.Cm_res[i-1])/(self.m_AlphaSweepVec[i+1] - self.m_AlphaSweepVec[i-1]))
            
            self.m_Cl_alpha_error[i] = (abs((self.m_Cl_alpha_res[i] - self.m_Cl_alpha_expected)/self.m_Cl_alpha_expected))*100
            self.m_Cm_alpha_error[i] = (abs((self.m_Cm_alpha_res[i] - self.m_Cm_alpha_expected)/self.m_Cm_alpha_expected))*100
        

        self.Cl_alpha_res_avg = math.degrees((self.Cl_res[self.m_AlphaNpts - 1] - self.Cl_res[0])/d_alpha) #rad
        self.Cm_alpha_res_avg = math.degrees((self.Cm_res[self.m_AlphaNpts - 1] - self.Cm_res[0])/d_alpha) #rad

        self.m_VKT_Sweep_Cl_alpha_Err = (abs((self.Cl_alpha_res_avg - self.m_Cl_alpha_expected)/self.m_Cl_alpha_expected))*100
        self.m_VKT_Sweep_Cm_alpha_Err = (abs((self.Cm_alpha_res_avg - self.m_Cm_alpha_expected)/self.m_Cm_alpha_expected))*100
        
        vsp.ClearVSPModel()
#======== Use Bokeh to Create tables and Graphs for the _________ Studies =#
    def GenerateVKTCharts(self):
        title = 'VKT Geometry Setup'
        header = ['Airfoil', 'AR', 'Root Chord', 'Tip Chord', 'Λ (°)', 'Λ Location', 'Span Tess (U)','Chord Tess (W)','Tip Clustering']
        data = [['NACA0012'], ['2√2'], ['1.5'],['0.5'],['53.54'],['0.0'],['6'],['33'],['1.0']]
        data_table = make_table(header,data)
        export_png(data_table,filename='vkt_files/vkt_img/vkt/geometrysetup.png')
        
        title = 'VKT VSPAERO Setup'
        header = ['Analysis', 'Method', 'α (°)', 'β (°)', 'M', 'Wake Iterations']
        data = [['Sweep'], ['VLM'], [str(self.alpha_0)+' to '+str(self.alpha_f)+', npts: '+str(self.m_AlphaNpts)],['0.0'],['0.1'],['3']]
        data_table = make_table(header,data)
        export_png(data_table,filename='vkt_files/vkt_img/vkt/vspaerosetup.png')
        
        p = figure(width=const.bokehwidth,height=const.bokehheight, title='VKT VLM: Cl vs Alpha',x_axis_label='Alpha (°)', y_axis_label='Cl')
        p.line(self.m_AlphaSweepVec,self.Cl_res, legend_label='VSPAERO',color=const.bokehcolors[0],line_width=const.bokehlinewidth)
        p.circle(self.m_AlphaSweepVec,self.Cl_res, color=const.bokehcolors[0],size=const.bokehsize)
        p.line(self.m_AlphaSweepVec,self.Cl_approx_vec, legend_label='Expected',color=const.bokehcolors[-1],line_width=const.bokehlinewidth)
        p.add_layout(p.legend[0],'right')
        #p.y_range.start=0
        export_png(p,filename='vkt_files/vkt_img/vkt/vktrawcl.png')
        
        p = figure(width=const.bokehwidth,height=const.bokehheight, title='VKT VLM Cl_alpha Alpha Sensitivity',x_axis_label='Alpha (°)', y_axis_label=r'Cl_alpha % Difference')
        p.line(self.m_AlphaSweepVec,self.m_Cl_alpha_error, legend_label=r'% Difference',color=const.bokehcolors[0],line_width=const.bokehlinewidth)
        p.circle(self.m_AlphaSweepVec,self.m_Cl_alpha_error, color=const.bokehcolors[0],size=const.bokehsize)
        p.add_layout(p.legend[0],'right')
        p.y_range.start=0
        export_png(p,filename='vkt_files/vkt_img/vkt/vktpercentcl.png')
        
        p = figure(width=const.bokehwidth,height=const.bokehheight, title='VKT VLM: Cm vs Alpha',x_axis_label='Alpha (°)', y_axis_label='Cm')
        p.line(self.m_AlphaSweepVec,self.Cm_res, legend_label='VSPAERO',color=const.bokehcolors[0],line_width=const.bokehlinewidth)
        p.circle(self.m_AlphaSweepVec,self.Cm_res, color=const.bokehcolors[0],size=const.bokehsize)
        p.line(self.m_AlphaSweepVec,self.Cm_approx_vec, legend_label='Expected',color=const.bokehcolors[-1],line_width=const.bokehlinewidth)
        p.add_layout(p.legend[0],'right')
        #p.y_range.start=0
        export_png(p,filename='vkt_files/vkt_img/vkt/vktrawcm.png')
        
        p = figure(width=const.bokehwidth,height=const.bokehheight, title='VKT VLM Cm_alpha Alpha Sensitivity',x_axis_label='Alpha (°)', y_axis_label=r'Cm_alpha % Difference')
        p.line(self.m_AlphaSweepVec,self.m_Cm_alpha_error, legend_label=r'% Difference',color=const.bokehcolors[0],line_width=const.bokehlinewidth)
        p.circle(self.m_AlphaSweepVec,self.m_Cm_alpha_error, color=const.bokehcolors[0],size=const.bokehsize)
        p.add_layout(p.legend[0],'right')
        p.y_range.start=0
        export_png(p,filename='vkt_files/vkt_img/vkt/vktpercentcm.png')
        
        
        title = 'VKT Results'
        header = ['α (°)', 'CLα Expected (rad)', 'CLα Result (rad)', 'CLα % Difference', 'CMα Expected (rad)', 'CMα Result (rad)', 'CMα % Difference']
        data = [self.m_AlphaSweepVec, [self.m_Cl_alpha_expected]*self.m_AlphaNpts, self.m_Cl_alpha_res,self.m_Cl_alpha_error,[self.m_Cm_alpha_expected]*self.m_AlphaNpts, self.m_Cm_alpha_res,self.m_Cm_alpha_error]
        data_table = make_table(header,data)
        export_png(data_table,filename='vkt_files/vkt_img/vkt/results.png')

        
    
def runVKTstudy(mode = 3):
    setup_filepaths()
    currentpath = str(Path(__file__).parent.resolve())
    
    test = VKTTest()
    if (mode == 1 or mode == 2):
        with open(currentpath+'/vkt_files/vkttest.pckl','rb') as picklefile:    
            test = pickle.load(picklefile)
    if (mode == 1): 
        test.GenerateVKTCharts()
    if (mode == 3):
        test.VKTStudy()
        with open(currentpath+'/vkt_files/vkttest.pckl','wb') as picklefile:
            pickle.dump(test,picklefile)
    if (mode == 2):
        test.TestVKTWings()
        test.GenerateVKTCharts()
        with open(currentpath+'/vkt_files/vkttest.pckl','wb') as picklefile:
            pickle.dump(test,picklefile)        
            
def setup_filepaths():
    scriptpath = Path(__file__).parent.resolve()
    testnames = ['vkt_files/']
    subnames = [['vkt_img/','vsp_files/']]
    subsubnames = [[['vkt'],['']]]
    for i in range(len(testnames)):
        for j in range(len(subnames[i])):
            for k in range(len(subsubnames[i][j])):
                dirname = Path.joinpath(scriptpath, testnames[i]+subnames[i][j]+subsubnames[i][j][k])
                dirname.mkdir(parents=True, exist_ok=True)
                
if __name__ == '__main__':
    runVKTstudy(mode = 3)    
