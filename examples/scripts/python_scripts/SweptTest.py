import openvsp as vsp
import math
import Constants as const
from bokeh.plotting import figure,output_file, show
from bokeh.io import export_png

class SweptTest:
    '''!Class for running and collecting data from the
        swept wing studies
    '''
    
    def __init__(self):
        #Which studies to run
        uw = True
        ar = True
        
        self.m_halfAR = [0] * 6
        self.m_halfAR[0] = 2.5
        self.m_halfAR[1] = 5.006975
        self.m_halfAR[2] = 7.5
        self.m_halfAR[3] = 12.5
        self.m_halfAR[4] = 20.000
        self.m_halfAR[5] = 30
        
        self.m_Tess_U = [0] * 4
        self.m_Tess_U[0] = 5
        self.m_Tess_U[1] = 12
        self.m_Tess_U[2] = 20
        self.m_Tess_U[3] = 41

        self.m_Tess_W = [0] * 4
        self.m_Tess_W[0] = 9
        self.m_Tess_W[1] = 18
        self.m_Tess_W[2] = 29
        self.m_Tess_W[3] = 51
        
        self.m_Sweep = [0] * 5
        self.m_Sweep[0] = 0
        self.m_Sweep[1] = 10.005
        self.m_Sweep[2] = 20
        self.m_Sweep[3] = 30.005
        self.m_Sweep[4] = 40
        
        self.Error_Cla = [[0.0]*len(self.m_Tess_W) for i in range(len(self.m_Tess_U))] #array<array<double>> index 0: UTess, index 1: WTess
        
        pass
    def SweptWingStudy(self):
        if(self.uw):
            self._someuwStudy()
        if(self.ar):
            self._somearStudy()

#========================================= SweptUWTess Functions =================================#
#==================== Generates the relavent parameteres. Runs the ____________      =============#
#==================== ___________________ studies. Generates the ___ tables and      =============#
#==================== _____ charts charts to include in the markdown file.           =============#
#=================================================================================================#

#========== Wrapper function for ________________________________ Code ===========================#
    def _someuwStudy(self):
        self.GenerateSweptUWTessWings()
        self.TestSweptUWTessWings()
        self.GenerateSweptUWTessCharts()
#===================== Sweapt UWTess Generation Functions =====================
    def GenerateSweptUWTessWings(self):
        #INSERT lines 4612 - 4658 from v&v script
                # Add Wing Geometry
        wing_id = vsp.AddGeom( "WING", "" )
        
        # Set Wing Section
        vsp.SetDriverGroup( wing_id, 1, vsp.AR_WSECT_DRIVER, vsp.ROOTC_WSECT_DRIVER, vsp.TIPC_WSECT_DRIVER )
        
        vsp.Update()
        
        # Set NACA 0012 Airfoil and Common Parms 
        vsp.SetParmVal( wing_id, "ThickChord", "XSecCurve_0", 0.12 )
        vsp.SetParmVal( wing_id, "ThickChord", "XSecCurve_1", 0.12 )
        vsp.SetParmVal( wing_id, "Root_Chord", "XSec_1", 1.0 )
        vsp.SetParmVal( wing_id, "Tip_Chord", "XSec_1", 1.0 )
        
        vsp.SetParmVal( wing_id, "TECluster", "WingGeom", 1.0 )
        vsp.SetParmVal( wing_id, "LECluster", "WingGeom", 0.2 )
        
        x = 1 # AR
        s = 3 # Sweep
        
        vsp.SetParmVal( wing_id, "Aspect", "XSec_1", self.m_halfAR[x] ); # Constant AR
        vsp.SetParmVal( wing_id, "Sweep", "XSec_1", self.m_Sweep[s] ); # Constant Sweep
        vsp.SetParmVal( wing_id, "OutCluster", "XSec_1", 1.0 ); # Constant Tip Clustering
        
        vsp.Update()
        
        for u in range(len(self.m_Tess_U)):
            for w in range(len(self.m_Tess_W)):
                vsp.SetParmVal( wing_id, "SectTess_U", "XSec_1", self.m_Tess_U[u] ) # Constant U Tess
                vsp.SetParmVal( wing_id, "Tess_W", "Shape", self.m_Tess_W[w] ) # Constant W Tess
                
                vsp.Update()

                # Setup export filenames for AR Study
                fname = "Swept_U" + self.m_Tess_U[u] + "_W" + self.m_Tess_W[w] + ".vsp3"

                # Save Vehicle to File
                message = "-->Saving vehicle file to: " + fname + "\n"
                print( message )
                vsp.WriteVSPFile( fname, vsp.SET_ALL )
                print( "COMPLETE\n" )
            
        
        vsp.ClearVSPModel()
        pass

#========== Run the actual ____________ Studies ==============================#
    def TestSweptUWTessWings(self):
        #Insert lines 4955-5056 from v&v script
        print( "-> Begin Swept Wing UW Tesselation Test:\n" )
        
        x = 1 # AR
        s = 2 # Sweep
        
        
        
        # Calculate Experimental and Theoretical Values
        self.C_bot_two = 1 + (pow((math.tan(math.radians(self.m_Sweep[s]))),2)/pow(const.b,2))
        self.C_top = 2*math.pi*2*self.m_halfAR[x]
        self.C_bot_one_theo = (pow((2*self.m_halfAR[x]),2)*pow(b,2))/pow(const.k_theo,2)
        self.C_bot_theo = (2 + (math.sqrt((self.C_bot_one_theo*self.C_bot_two)+4)))
        self.C_l_alpha_exper_theo = self.C_top/self.C_bot_theo
        self.Cl_alpha_theo = math.degrees(self.C_l_alpha_exper_theo) # rad --> deg
        self.Lift_angle_theo = 1/self.Cl_alpha_theo # Cl to lift angle
        
        
        
        for u in range(len(self.m_Tess_U)): 
            for w in range(len(self.m_Tess_W)):
            
                # Open the file
                fname = "Swept_U" + int(self.m_Tess_U[u]) + "_W" + int(self.m_Tess_W[w]) + ".vsp3"
                fname_res = "Swept_U" + int(self.m_Tess_U[u]) + "_W" + int(self.m_Tess_W[w]) + "_res.csv"

                print( "Reading in file: " )
                print( fname )
                vsp.ReadVSPFile( fname ) # Sets VSP3 file name
                
                #==== Analysis: VSPAeroSinglePoint ====#
                print( const.m_VSPSingleAnalysis )

                #==== Analysis: VSPAero Compute Geometry to Create Vortex Lattice DegenGeom File ====#
                print( const.m_CompGeomAnalysis )

                # Set defaults
                vsp.SetAnalysisInputDefaults( const.m_CompGeomAnalysis )

                # list inputs, type, and current values
                vsp.PrintAnalysisInputs( const.m_CompGeomAnalysis )

                # Execute
                print( "\tExecuting..." )
                compgeom_resid = vsp.ExecAnalysis( const.m_CompGeomAnalysis )
                print( "COMPLETE" )

                # Get & Display Results
                vsp.PrintResults( compgeom_resid )

                #==== Analysis: VSPAero Single Point ====#
                # Set defaults
                vsp.SetAnalysisInputDefaults(const.m_VSPSingleAnalysis)
                print(const.m_VSPSingleAnalysis)

                # Reference geometry set
                vsp.SetIntAnalysisInput( const.m_VSPSingleAnalysis, "GeomSet", const.m_GeomVec, 0 )
                vsp.SetIntAnalysisInput(const.m_VSPSingleAnalysis, "RefFlag", const.m_RefFlagVec, 0)
                vsp.SetIntAnalysisInput(const.m_VSPSingleAnalysis, "Symmetry", const.m_SymFlagVec, 0)

                wid = vsp.FindGeomsWithName( "WingGeom" )
                vsp.SetStringAnalysisInput(const.m_VSPSingleAnalysis, "WingID", wid, 0)

                # Freestream Parameters
                vsp.SetDoubleAnalysisInput(const.m_VSPSingleAnalysis, "Alpha", const.m_AlphaVec, 0)
                vsp.SetDoubleAnalysisInput(const.m_VSPSingleAnalysis, "Mach", const.m_MachVec, 0)
                vsp.SetIntAnalysisInput(const.m_VSPSingleAnalysis, "WakeNumIter", const.m_WakeIterVec, 0)

                vsp.Update()

                # list inputs, type, and current values
                vsp.PrintAnalysisInputs( const.m_VSPSingleAnalysis )
                print( "" )

                # Execute
                print( "\tExecuting..." )
                rid = vsp.ExecAnalysis( const.m_VSPSingleAnalysis )
                print( "COMPLETE" )

                # Get & Display Results
                vsp.PrintResults( rid )
                vsp.WriteResultsCSVFile( rid, fname_res )

                # Get Result ID Vec (History and Load ResultIDs)
                rid_vec = vsp.GetStringResults( rid, "ResultsVec" )
                if len(rid_vec) > 0 :
                
                    # Get History Results (rid_vec[0]) from Final Wake Iteration in History Result
                    cl_vec = vsp.GetDoubleResults( rid_vec[0], "CL" )
                    Cl_res = cl_vec[int(cl_vec.length()) - 1]

                    # Calculate Error
                    C_l_alpha_vsp = Cl_res # alpha = 1.0 (deg)
                    self.Error_Cla[u][w] = (abs((C_l_alpha_vsp - self.Cl_alpha_theo)/self.Cl_alpha_theo))*100
                
                
                vsp.ClearVSPModel()
        pass
#======== Use Bokeh to Create tables and Graphs for the _________ Studies =#
    def GenerateSweptUWTessCharts(self):
        pass

#========================================= SweptARSweep Functions =================================#
#==================== Generates the relavent parameteres. Runs the ____________      =============#
#==================== ___________________ studies. Generates the ___ tables and      =============#
#==================== _____ charts charts to include in the markdown file.           =============#
#=================================================================================================#

#========== Wrapper function for ________________________________ Code ===========================#
    def _somearStudy(self):
        self.GenerateSweptARSweepWings()
        self.TestSweptARSweepWings()
        self.GenerateSweptARSweep()
#===================== Sweapt ARSweep Generation Functions =====================
    def GenerateSweptARSweepWings(self):
        #Insert lines 4561-4607 from v&v script
        pass

#========== Run the actual ____________ Studies ==============================#
    def TestSweptARSweepWings(self):
        #Insert lines 4663-4848 from v&v script
        pass
#======== Use Bokeh to Create tables and Graphs for the _________ Studies =#
    def GenerateSweptARSweep(self):
        pass