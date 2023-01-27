import openvsp as vsp
import math
import Contraints as const
class HersheyTest:
    '''! Class for running and collecting data from 
         the Hershey studies
    '''
    def __init__(self):

        self.m_halfAR = [0]*6
        self.m_halfAR[0] = 2.5
        self.m_halfAR[1] = 5.0016
        self.m_halfAR[2] = 7.5
        self.m_halfAR[3] = 12.5
        self.m_halfAR[4] = 20.0
        self.m_halfAR[5] = 30.0

        self.m_AlphaNpts = 9
        
        self.m_Tip_Clus = [0]*3
        self.m_Tip_Clus[0] = 0.1
        self.m_Tip_Clus[1] = 0.5
        self.m_Tip_Clus[2] = 1

        self.m_Tess_U =[0]*4
        self.m_Tess_U[0] = 5
        self.m_Tess_U[1] = 12
        self.m_Tess_U[2] = 20
        self.m_Tess_U[3] = 41

        self.m_Tess_W = [0]*4
        self.m_Tess_W[0] = 9
        self.m_Tess_W[1] = 17
        self.m_Tess_W[2] = 29
        self.m_Tess_W[3] = 51
        
        self.m_WakeIter = [0]*4
        self.m_WakeIter[0] = 1
        self.m_WakeIter[1] = 2
        self.m_WakeIter[2] = 3
        self.m_WakeIter[3] = 4
        self.m_WakeIter[4] = 5
        
        self.m_AdvancedWakeVec = [0]*3
        self.m_AdvancedWakeVec[0] = 1
        self.m_AdvancedWakeVec[1] = 2
        self.m_AdvancedWakeVec[2] = 3
        
        self.AVL_file_name = vsp.GetVSPExePath() + "/airfoil/Hershey_AR10_AVL.dat"
        self.m_AR10_Y_Cl_Cd_vec = vsp.ReadAVLFSFile(self.AVL_file_name)
    
    def hersheyBarStudy(self):
        #====Generate Hershey Bar Wings====
        self.generateHersheyBarWings()

        #====Test Hershey Bar Wings====
        self.testHersheyBarWings()
    
    def generateHersheyBarWings(self):
        self.generateHersheyBarARWings()
        self.generateHersheyBarUWTessWings()
        self.generateHersheyBarTCWings()
        self.generateHersheyBarUTessWings()
        self.generateHersheyBarWTessWings()
        self.generateHersheyBarWakeWings()
        self.generateHersheyBarAdvancedWings()
    
    def testHersheyBarWings(self):
        
        self.testHersheyBarARWings()
        self.testHersheyBarUWTessWings()
        self.testHersheyBarTCWings()
        self.testHersheyBarUTessWings()
        self.testHersheyBarWTessWings()
        self.testHersheyBarWakeWings()
        self.testHersheyBarAdvancedWings()
    

    #===================== Hershey Bar Wing Generation Functions =====================
    def generateHersheyBarARWings(self):
        #==== Add Wing Geometry ====
        wing_id = vsp.AddGeom( "WING", "" ) 

        #==== Set Wing Section ====
        vsp.SetDriverGroup(wing_id, 1 ,vsp.AR_WSECT_DRIVER, vsp.ROOTC_WSECT_DRIVER, vsp.TIPC_WSECT_DRIVER)

        vsp.Update()

        #==== Set NACA 0012 Airfoil and Common Parms
        vsp.SetParmVal( wing_id, "ThickChord", "XSecCurve_0", 0.12 )
        vsp.SetParmVal( wing_id, "ThickChord", "XSecCurve_1", 0.12 )
        vsp.SetParmVal( wing_id, "Sweep", "XSec_1", 0 )
        vsp.SetParmVal( wing_id, "Root_Chord", "XSec_1", 1.0 )
        vsp.SetParmVal( wing_id, "Tip_Chord", "XSec_1", 1.0 )
        vsp.SetParmVal( wing_id, "TECluster", "WingGeom", 1.0 )
        vsp.SetParmVal( wing_id, "LECluster", "WingGeom", 0.2 )

        u = 3
        w = 3
        t = 2

        vsp.SetParmVal( wing_id, "SectTess_U", "XSec_1", self.m_Tess_U[u] ) # Constant U Tess
        vsp.SetParmVal( wing_id, "Tess_W", "Shape", self.m_Tess_W[w] ) # Constant W Tess
        vsp.SetParmVal( wing_id, "OutCluster", "XSec_1", self.m_Tip_Clus[t] ) # Constant Tip Clustering

        vsp.Update()

        for x in range(len(self.m_halfAR)):
            vsp.SetParmVal( wing_id, "Aspect", "XSec_1", self.m_halfAR[x] )

            vsp.Update()

            #==== Setup export filenames for AR Study ====
            fname = "Hershey_AR" + int(2*self.m_halfAR[x]) + ".vsp3"

            #==== Save Vehicle to File ====
            message = "-->Saving vehicle file to: " + fname + "\n"
            print( message )
            vsp.WriteVSPFile( fname, vsp.SET_ALL )
            print( "COMPLETE\n" )

        vsp.ClearVSPModel()

    def generateHersheyBarUTessWings(self):
        #==== Add Wing Geometry ====#
        wing_id = vsp.AddGeom( "WING", "" )
        
        #==== Set Wing Section ====#
        vsp.SetDriverGroup( wing_id, 1, vsp.AR_WSECT_DRIVER, vsp.ROOTC_WSECT_DRIVER, vsp.TIPC_WSECT_DRIVER )
        
        vsp.Update()
        
        #==== Set NACA 0012 Airfoil and Common Parms 
        vsp.SetParmVal( wing_id, "ThickChord", "XSecCurve_0", 0.12 )
        vsp.SetParmVal( wing_id, "ThickChord", "XSecCurve_1", 0.12 )
        vsp.SetParmVal( wing_id, "Sweep", "XSec_1", 0 )
        vsp.SetParmVal( wing_id, "Root_Chord", "XSec_1", 1.0 )
        vsp.SetParmVal( wing_id, "Tip_Chord", "XSec_1", 1.0 )
        vsp.SetParmVal( wing_id, "TECluster", "WingGeom", 1.0 )
        vsp.SetParmVal( wing_id, "LECluster", "WingGeom", 0.2 )
        
        x = 1 # AR
        w = 1 # WTess
        t = 2 # Tip Clustering
        
        vsp.SetParmVal( wing_id, "Aspect", "XSec_1", self.m_halfAR[x] ) # Constant AR
        vsp.SetParmVal( wing_id, "Tess_W", "Shape", self.m_Tess_W[w] ) # Constant W Tess
        vsp.SetParmVal( wing_id, "OutCluster", "XSec_1", self.m_Tip_Clus[t] ) # Constant Tip Clustering
        
        vsp.Update()
        
        for u in range(len(self.m_Tess_U)):
            self.SetParmVal( wing_id, "SectTess_U", "XSec_1", self.m_Tess_U[u] )

            vsp.Update()

            #==== Setup export filenames for U Tess Study ====#
            fname = "Hershey_U" + int(self.m_Tess_U[u]) + ".vsp3"

            #==== Save Vehicle to File ====#
            message = "-->Saving vehicle file to: " + fname + "\n"
            print( message )
            vsp.WriteVSPFile( fname, vsp.SET_ALL )
            print( "COMPLETE\n" )
        
        
        vsp.ClearVSPModel()

    def generateHersheyBarTCWings(self):
        #==== Add Wing Geometry ====#
        wing_id = vsp.AddGeom( "WING", "" )
        
        #==== Set Wing Section ====#
        vsp.SetDriverGroup( wing_id, 1, vsp.AR_WSECT_DRIVER, vsp.ROOTC_WSECT_DRIVER, vsp.TIPC_WSECT_DRIVER )
        
        vsp.Update()
        
        #==== Set NACA 0012 Airfoil and Common Parms 
        vsp.SetParmVal( wing_id, "ThickChord", "XSecCurve_0", 0.12 )
        vsp.SetParmVal( wing_id, "ThickChord", "XSecCurve_1", 0.12 )
        vsp.SetParmVal( wing_id, "Sweep", "XSec_1", 0 )
        vsp.SetParmVal( wing_id, "Root_Chord", "XSec_1", 1.0 )
        vsp.SetParmVal( wing_id, "Tip_Chord", "XSec_1", 1.0 )
        vsp.SetParmVal( wing_id, "TECluster", "WingGeom", 1.0 )
        vsp.SetParmVal( wing_id, "LECluster", "WingGeom", 0.2 )
        
        x = 1 # AR
        u = 1 # UTess
        w = 1 # WTess
        
        vsp.SetParmVal( wing_id, "Aspect", "XSec_1", self.m_halfAR[x] ) # Constant AR
        vsp.SetParmVal( wing_id, "SectTess_U", "XSec_1", self.m_Tess_U[u] ) # Constant U Tess
        vsp.SetParmVal( wing_id, "Tess_W", "Shape", self.m_Tess_W[w] ) # Constant W Tess
        
        vsp.Update()
        
        for t in range(len(self.m_Tip_Clus)):
        
            vsp.SetParmVal( wing_id, "OutCluster", "XSec_1", self.m_Tip_Clus[t] )

            vsp.Update()

            #==== Setup export filenames for Tip Clustering Study ====#
            fname = "Hershey_TC" + float(self.m_Tip_Clus[t]) + ".vsp3"

            #==== Save Vehicle to File ====#
            message = "-->Saving vehicle file to: " + fname + "\n"
            print( message )
            vsp.WriteVSPFile( fname, vsp.SET_ALL )
            print( "COMPLETE\n" )
        
        
        vsp.ClearVSPModel()
    
    def generateHersheyBarUWTessWings(self):
        #==== Add Wing Geometry ====#
        wing_id = vsp.AddGeom( "WING", "" )
        
        #==== Set Wing Section ====#
        vsp.SetDriverGroup( wing_id, 1, vsp.AR_WSECT_DRIVER, vsp.ROOTC_WSECT_DRIVER, vsp.TIPC_WSECT_DRIVER )
        
        vsp.Update()
        
        #==== Set NACA 0012 Airfoil and Common Parms 
        vsp.SetParmVal( wing_id, "ThickChord", "XSecCurve_0", 0.12 )
        vsp.SetParmVal( wing_id, "ThickChord", "XSecCurve_1", 0.12 )
        vsp.SetParmVal( wing_id, "Sweep", "XSec_1", 0 )
        vsp.SetParmVal( wing_id, "Root_Chord", "XSec_1", 1.0 )
        vsp.SetParmVal( wing_id, "Tip_Chord", "XSec_1", 1.0 )
        vsp.SetParmVal( wing_id, "TECluster", "WingGeom", 1.0 )
        vsp.SetParmVal( wing_id, "LECluster", "WingGeom", 0.2 )
        
        x = 1 # AR
        t = 2 # Tip Clustering
        
        vsp.SetParmVal( wing_id, "Aspect", "XSec_1", self.m_halfAR[x] ) # Constant AR
        vsp.SetParmVal( wing_id, "OutCluster", "XSec_1", self.m_Tip_Clus[t] ) # Constant Tip Clustering
        
        vsp.Update()
        
        for u in range(len(self.m_Tess_U)):
        
            for w in range(len(self.m_Tess_W)):
            
                vsp.SetParmVal( wing_id, "SectTess_U", "XSec_1", self.m_Tess_U[u] )
                vsp.SetParmVal( wing_id, "Tess_W", "Shape", self.m_Tess_W[w] )

                vsp.Update()

                #==== Setup export filenames for UW Tess Study ====#
                fname = "Hershey_U" + int(self.m_Tess_U[u]) + "_W" + int(self.m_Tess_W[w]) + ".vsp3"

                #==== Save Vehicle to File ====#
                message = "-->Saving vehicle file to: " + fname + "\n"
                print( message )
                vsp.WriteVSPFile( fname, vsp.SET_ALL )
                print( "COMPLETE\n" )
            
        
        
        vsp.ClearVSPModel()

    def generateHersheyBarWTessWings(self):
        #==== Add Wing Geometry ====#
        wing_id = vsp.AddGeom( "WING", "" )
        
        #==== Set Wing Section ====#
        vsp.SetDriverGroup( wing_id, 1, vsp.AR_WSECT_DRIVER, vsp.ROOTC_WSECT_DRIVER, vsp.TIPC_WSECT_DRIVER )
        
        vsp.Update()
        
        #==== Set NACA 0012 Airfoil and Common Parms 
        vsp.SetParmVal( wing_id, "ThickChord", "XSecCurve_0", 0.12 )
        vsp.SetParmVal( wing_id, "ThickChord", "XSecCurve_1", 0.12 )
        vsp.SetParmVal( wing_id, "Sweep", "XSec_1", 0 )
        vsp.SetParmVal( wing_id, "Root_Chord", "XSec_1", 1.0 )
        vsp.SetParmVal( wing_id, "Tip_Chord", "XSec_1", 1.0 )
        vsp.SetParmVal( wing_id, "TECluster", "WingGeom", 1.0 )
        vsp.SetParmVal( wing_id, "LECluster", "WingGeom", 0.2 )
        
        x = 1 # AR
        u = 1 # UTess
        t = 2 # Tip Clustering
        
        vsp.SetParmVal( wing_id, "Aspect", "XSec_1", self.m_halfAR[x] ) # Constant AR
        vsp.SetParmVal( wing_id, "SectTess_U", "XSec_1", self.m_Tess_U[u] ) # Constant U Tess
        vsp.SetParmVal( wing_id, "OutCluster", "XSec_1", self.m_Tip_Clus[t] ) # Constant Tip Clustering
        
        vsp.Update()
        
        for w in range(len(self.m_Tess_W)):
        
            vsp.SetParmVal( wing_id, "Tess_W", "Shape", self.m_Tess_W[w] )

            vsp.Update()

            #==== Setup export filenames for W Tess Study ====#
            fname = "Hershey_W" + int(self.m_Tess_W[w]) + ".vsp3"

            #==== Save Vehicle to File ====#
            message = "-->Saving vehicle file to: " + fname + "\n"
            print( message )
            vsp.WriteVSPFile( fname, vsp.SET_ALL )
            print( "COMPLETE\n" )
        
        
        vsp.ClearVSPModel()
    
    def generateHersheyBarWakeWings(self):
        #==== Add Wing Geometry ====#
        wing_id = vsp.AddGeom( "WING", "" )
        
        #==== Set Wing Section ====#
        vsp.SetDriverGroup( wing_id, 1, vsp.AR_WSECT_DRIVER, vsp.ROOTC_WSECT_DRIVER, vsp.TIPC_WSECT_DRIVER )
        
        vsp.Update()
        
        #==== Set NACA 0012 Airfoil and Common Parms 
        vsp.SetParmVal( wing_id, "ThickChord", "XSecCurve_0", 0.12 )
        vsp.SetParmVal( wing_id, "ThickChord", "XSecCurve_1", 0.12 )
        vsp.SetParmVal( wing_id, "Sweep", "XSec_1", 0 )
        vsp.SetParmVal( wing_id, "Root_Chord", "XSec_1", 1.0 )
        vsp.SetParmVal( wing_id, "Tip_Chord", "XSec_1", 1.0 )
        vsp.SetParmVal( wing_id, "TECluster", "WingGeom", 1.0 )
        vsp.SetParmVal( wing_id, "LECluster", "WingGeom", 0.2 )
        
        x = 1 # AR
        u = 1 # UTess
        w = 1 # WTess
        t = 2 # Tip Clustering
        
        vsp.SetParmVal( wing_id, "Aspect", "XSec_1", self.m_halfAR[x] ) # Constant AR
        vsp.SetParmVal( wing_id, "SectTess_U", "XSec_1", self.m_Tess_U[u] ) # Constant U Tess
        vsp.SetParmVal( wing_id, "Tess_W", "Shape", self.m_Tess_W[w] ) # Constant W Tess
        vsp.SetParmVal( wing_id, "OutCluster", "XSec_1", self.m_Tip_Clus[t] ) # Constant Tip Clustering
        
        vsp.Update()
        
        for i in range(len(self.m_WakeIter)):
        
            #==== Setup export filenames for Wake Iteration Study ====#
            fname = "Hershey_Wake" + int(self.m_WakeIter[i]) + ".vsp3"

            #==== Save Vehicle to File ====#
            message = "-->Saving vehicle file to: " + fname + "\n"
            print( message )
            vsp.WriteVSPFile( fname, vsp.SET_ALL )
            print( "COMPLETE\n" )
        
        
        vsp.ClearVSPModel()

    def generateHersheyBarAdvancedWings(self):
        #==== Add Wing Geometry ====#
        wing_id = vsp.AddGeom( "WING", "" )
        
        #==== Set Wing Section ====#
        vsp.SetDriverGroup( wing_id, 1, vsp.AR_WSECT_DRIVER, vsp.ROOTC_WSECT_DRIVER, vsp.TIPC_WSECT_DRIVER )
        
        vsp.Update()
        
        #==== Set NACA 0012 Airfoil and Common Parms 
        vsp.SetParmVal( wing_id, "ThickChord", "XSecCurve_0", 0.12 )
        vsp.SetParmVal( wing_id, "ThickChord", "XSecCurve_1", 0.12 )
        vsp.SetParmVal( wing_id, "Sweep", "XSec_1", 0 )
        vsp.SetParmVal( wing_id, "Root_Chord", "XSec_1", 1.0 )
        vsp.SetParmVal( wing_id, "Tip_Chord", "XSec_1", 1.0 )
        vsp.SetParmVal( wing_id, "TECluster", "WingGeom", 1.0 )
        vsp.SetParmVal( wing_id, "LECluster", "WingGeom", 0.2 )
        
        x = 2 # AR
        u = 1 # UTess
        w = 1 # WTess
        t = 2 # Tip Clustering
        
        vsp.SetParmVal( wing_id, "Aspect", "XSec_1", self.m_halfAR[x] ) # Constant AR
        vsp.SetParmVal( wing_id, "SectTess_U", "XSec_1", self.m_Tess_U[u] ) # Constant U Tess
        vsp.SetParmVal( wing_id, "Tess_W", "Shape", self.m_Tess_W[w] ) # Constant W Tess
        vsp.SetParmVal( wing_id, "OutCluster", "XSec_1", self.m_Tip_Clus[t] ) # Constant Tip Clustering
        
        vsp.Update()
        
        num_case = 4
        
        for i in range(num_case):
        
            #==== Setup export filenames for Wake Iteration Study ====#
            fname = "Hershey_Advanced_" + int(i) + ".vsp3"

            #==== Save Vehicle to File ====#
            message = "-->Saving vehicle file to: " + fname + "\n"
            print( message )
            vsp.WriteVSPFile( fname, vsp.SET_ALL )
            print( "COMPLETE\n" )
        
        
        vsp.ClearVSPModel()
    
    #===================== Hershey Bar Wing Testing Functions =====================
    def testHersheyBarARWings(self):
        print("-> Begin HersheyBar AR Study:\n")

        num_AR = len(self.m_halfAR)

        C_bot_two = 1 + (pow(math.tan(0.0),2)/pow(const.b,2))
        
        alpha_0 = -20.0
        alpha_f = 20.0
        d_alpha = alpha_f - alpha_0
        Vinf = 100
        alpha_step = d_alpha/(self.m_AlphaNpts - 1)
        alpha_mid_index = int((self.m_AlphaNpts - 1)/2.0)

        
        alpha_vlm = [[]]*(num_AR)
        Cl_vlm = [[]]*(num_AR)
        Cl_approx = [[]]*(num_AR)

        
        Cl_alpha_vlm = [0.0]*(num_AR)
        Lift_angle_vlm  = [0.0]*(num_AR)
        Lift_angle_theo = [0.0]*(num_AR)
        AR = [0.0]*(num_AR)
        C_ratio = [0.0]*(num_AR)
        Cl_alpha_theo = [0.0]*(num_AR)
        Lift_angle_pm = [0.0]*(num_AR)
        Cl_alpha_pm = [0.0]*(num_AR)
        Error_Cl_alpha_vlm = [0.0]*(self.m_AlphaNpts)
        
        for x in range(len(self.m_halfAR)):
        
            #==== Open and test generated wings ====#
            fname = "Hershey_AR" + int(2*self.m_halfAR[x]) + ".vsp3"
            fname_res_vlm = "Hershey_AR" + int(2*self.m_halfAR[x]) + "_vlm_res.csv"
            fname_res_pm = "Hershey_AR" + int(2*self.m_halfAR[x]) + "_pm_res.csv"

            print("Reading in file: ", False )
            print( fname )
            vsp.ReadVSPFile( fname ) # Sets VSP3 file name
            
            #==== Analysis: VSPAero VLM Sweep ====#
            print( const.m_VSPSweepAnalysis )

            #==== Analysis: VSPAero Compute Geometry to Create Vortex Lattice DegenGeom File ====#
            print( const.m_CompGeomAnalysis )

            # Set defaults
            vsp.SetAnalysisInputDefaults( const.m_CompGeomAnalysis )
            
            vsp.SetIntAnalysisInput(const.m_CompGeomAnalysis, "Symmetry", const.m_SymFlagVec, 0)

            # list inputs, type, and current values
            vsp.printAnalysisInputs( const.m_CompGeomAnalysis )

            # Execute
            print( "\tExecuting..." )
            compgeom_resid = vsp.ExecAnalysis( const.m_CompGeomAnalysis )
            print( "COMPLETE" )

            # Get & Display Results
            vsp.PrintResults( compgeom_resid )

            #==== Analysis: VSPAero VLM Sweep ====#
            # Set defaults
            vsp.SetAnalysisInputDefaults(const.m_VSPSweepAnalysis)

            # Reference geometry set
            vsp.SetIntAnalysisInput(const.m_VSPSweepAnalysis, "GeomSet", const.m_GeomVec, 0)
            vsp.SetIntAnalysisInput(const.m_VSPSweepAnalysis, "RefFlag", const.m_RefFlagVec, 0)

            wid = vsp.FindGeomsWithName( "WingGeom" )
            vsp.SetStringAnalysisInput(const.m_VSPSweepAnalysis, "WingID", wid, 0)
            vsp.SetIntAnalysisInput(const.m_VSPSweepAnalysis, "Symmetry", const.m_SymFlagVec, 0)
            vsp.SetIntAnalysisInput(const.m_VSPSweepAnalysis, "WakeNumIter", const.m_WakeIterVec, 0)

            # Freestream Parameters
            AlphaStart = [alpha_0] #array<double> AlphaStart
            AlphaEnd = [alpha_f] #array<double> AlhpaEnd
            AlphaNpts = [self.m_AlphaNpts] #array<int> AlphaNpts
            
            vsp.SetDoubleAnalysisInput(const.m_VSPSweepAnalysis, "AlphaStart", AlphaStart, 0)
            vsp.SetDoubleAnalysisInput(const.m_VSPSweepAnalysis, "AlphaEnd", AlphaEnd, 0)
            vsp.SetIntAnalysisInput(const.m_VSPSweepAnalysis, "AlphaNpts", AlphaNpts, 0)
            
            MachNpts = [1] # Start and end at 0.1 #array<double>
            vsp.SetDoubleAnalysisInput(const.m_VSPSweepAnalysis, "MachStart", const.m_MachVec, 0)
            vsp.SetDoubleAnalysisInput(const.m_VSPSweepAnalysis, "MachEnd", const.m_MachVec, 0)
            vsp.SetDoubleAnalysisInput(const.m_VSPSweepAnalysis, "MachNpts", MachNpts, 0)

            vsp.Update()

            # list inputs, type, and current values
            vsp.PrintAnalysisInputs( const.m_VSPSweepAnalysis )
            print( "" )

            # Execute
            print( "\tExecuting..." )
            rid = vsp.ExecAnalysis( const.m_VSPSweepAnalysis )
            print( "COMPLETE" )

            # Get & Display Results
            vsp.PrintResults( rid )
            vsp.WriteResultsCSVFile( rid, fname_res_vlm )
            
            # Get Result ID Vec (History and Load ResultIDs)
            rid_vec = vsp.GetStringResults( rid, "ResultsVec" )
            
            # Calculate Experimental and Theoretical Values
            # Fluid -Dynamic Lift pg 3-2
            # Method 1 of USAF DATCOM Section 1, page 1-7, and also NACA TN-3911)
            AR[x] = 2*self.m_halfAR[x]
            C_top = 2*math.pi*AR[x]
            C_bot_one_theo = (pow(AR[x],2)*pow(const.b,2))/pow(const.k_theo,2)
            C_bot_theo = (2 + (math.sqrt((C_bot_one_theo*C_bot_two)+4)))
            Cl_alpha_theo[x] = (C_top/C_bot_theo)*(math.pi/180) # deg
            Lift_angle_theo[x] = 1/(Cl_alpha_theo[x]) # Cl to lift angle (deg)
            C_ratio[x] = 1/AR[x] # AR to chord ratio
            
            if ( len(rid_vec) >= 1 ):
                alpha_res = [0.0]*( self.m_AlphaNpts )
                Cl_res = [0.0]*( self.m_AlphaNpts )
                Cl_approx_vec = [0.0]*( self.m_AlphaNpts )
                
                # Get Result from Final Wake Iteration
                for i in range(self.m_AlphaNpts):
                
                    alpha_vec = vsp.GetDoubleResults( rid_vec[i], "Alpha" )
                    alpha_res[i] = alpha_vec[int(len(alpha_vec)) - 1]
                    
                    cl_vec = vsp.GetDoubleResults( rid_vec[i], "CL" )
                    Cl_res[i] = cl_vec[int(len(cl_vec)) - 1]
                    
                    Cl_approx_vec[i] = 2 * math.pi * math.sin( math.radians( alpha_res[i] ) )
                
                
                if ( x == 1 ):
                
                    for i in range(self.m_AlphaNpts):
                        Cl_alpha_res = 0

                        if ( i == 0 ):
                            Cl_alpha_res = ((Cl_res[i+1] - Cl_res[i])/(alpha_res[i+1] - alpha_res[i]))
                        
                        elif ( i == self.m_AlphaNpts ):
                            Cl_alpha_res = ((Cl_res[i] - Cl_res[i-1])/(alpha_res[i] - alpha_res[i-1]))
                        
                        else: # Central differencing
                            Cl_alpha_res = ((Cl_res[i+1] - Cl_res[i-1])/(alpha_res[i+1] - alpha_res[i-1]))
                        
                        
                        Error_Cl_alpha_vlm[i] = (abs((Cl_alpha_res - Cl_alpha_theo[x])/Cl_alpha_theo[x]))*100
                    
                
                
                # Evaluate Cl_alpha near alpha = 0 to avoid errors due to unmodeled stall characteristics
                Cl_alpha_vlm[x] = ((Cl_res[alpha_mid_index + 1] - Cl_res[alpha_mid_index])/alpha_step) 
                
                alpha_vlm[x] = alpha_res
                Cl_vlm[x] = Cl_res
                Cl_approx[x] = Cl_approx_vec
                
                Lift_angle_vlm[x] = 1/(Cl_alpha_vlm[x]) # deg
            
            
            #==== Analysis: VSPAero Panel Single ====#
            print( const.m_VSPSingleAnalysis )
            
            #==== Analysis: VSPAero Compute Geometry to Create Vortex Lattice DegenGeom File ====#
            print( const.m_CompGeomAnalysis )

            # Set defaults
            vsp.SetAnalysisInputDefaults( const.m_CompGeomAnalysis )
            
            panel_analysis=[vsp.PANEL]
            vsp.SetIntAnalysisInput( const.m_CompGeomAnalysis, "AnalysisMethod", panel_analysis )
            
            vsp.SetIntAnalysisInput(const.m_CompGeomAnalysis, "Symmetry", const.m_SymFlagVec, 0)

            # list inputs, type, and current values
            vsp.PrintAnalysisInputs( const.m_CompGeomAnalysis )

            # Execute
            print( "\tExecuting..." )
            compgeom_resid = vsp.ExecAnalysis( const.m_CompGeomAnalysis )
            print( "COMPLETE" )

            # Get & Display Results
            vsp.PrintResults( compgeom_resid )
            
            #==== Analysis: VSPAero Panel Single ====#
            # Set defaults
            vsp.SetAnalysisInputDefaults(const.m_VSPSingleAnalysis)
            print(const.m_VSPSingleAnalysis)

            # Reference geometry set
            vsp.SetIntAnalysisInput(const.m_VSPSingleAnalysis, "GeomSet", const.m_GeomVec, 0)
            vsp.SetIntAnalysisInput(const.m_VSPSingleAnalysis, "RefFlag", const.m_RefFlagVec, 0)
            vsp.SetStringAnalysisInput(const.m_VSPSingleAnalysis, "WingID", wid, 0)
            vsp.SetIntAnalysisInput(const.m_VSPSingleAnalysis, "WakeNumIter", const.m_WakeIterVec, 0)
            vsp.SetIntAnalysisInput( const.m_VSPSingleAnalysis, "AnalysisMethod", panel_analysis )
            vsp.SetIntAnalysisInput(const.m_VSPSingleAnalysis, "Symmetry", const.m_SymFlagVec, 0)
            
            # Freestream Parameters
            Alpha = [1.0]
            vsp.SetDoubleAnalysisInput(const.m_VSPSingleAnalysis, "Alpha", Alpha, 0)
            vsp.SetDoubleAnalysisInput(const.m_VSPSingleAnalysis, "Mach", const.m_MachVec, 0)
            
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
            vsp.WriteResultsCSVFile( rid, fname_res_pm )
            
            # Get Result ID Vec (History and Load ResultIDs)
            rid_vec = vsp.GetStringResults( rid, "ResultsVec" )
            
            if ( len(rid_vec) > 0 ):
            
                # Get Result from Final Wake Iteration
                cl_vec = vsp.GetDoubleResults( rid_vec[0], "CL" )
                
                Cl_pm = cl_vec[int(len(cl_vec)) - 1]
                Cl_alpha_pm[x] = Cl_pm # deg (alpha = 1.0°)
                Lift_angle_pm[x] = 1/(Cl_alpha_pm[x]) # deg
            

            vsp.ClearVSPModel()

    def testHersheyBarUWTessWings(self):
        print( "-> Begin Hershey Bar U and W Tesselation Study:\n")
        
        x = 1 # AR
        
        numUTess = len(self.m_Tess_U)
        numWTess = len(self.m_Tess_W)
        
        Error_Cla = [[]]*numUTess #array<array<double>>
        Error_Cl = [[]]*(numUTess) #array<array<double>>
        Exe_Time= [[]]*(numUTess) # index 0: UTess, index 0: WTess #array<array<double>>
        
        # Calculate Experimental and Theoretical Values
        # Fluid -Dynamic Lift pg 3-2
        # Method 1 of USAF DATCOM Section 1, page 1-7, and also NACA TN-3911)
        C_bot_two = 1 + (pow(math.tan(0.0),2)/pow(const.b,2))
        AR = 2*self.m_halfAR[x]
        C_top = 2*math.pi*AR
        C_bot_one_theo = (pow(AR,2)*pow(const.b,2))/pow(const.k_theo,2)
        C_bot_theo = (2 + (math.sqrt((C_bot_one_theo*C_bot_two)+4)))
        Cl_alpha_theo = (C_top/C_bot_theo)*(math.pi/180) # deg
        Lift_angle_theo = 1/(Cl_alpha_theo) # Cl to lift angle (deg)
        
        for u in range(numUTess):
            Error_Cla[u].resize(numWTess)
            Error_Cl[u].resize(numWTess)
            Exe_Time[u].resize(numWTess)
            
            for w in range(numWTess):
            
                fname = "Hershey_U" + int(self.m_Tess_U[u]) + "_W" + int(self.m_Tess_W[w]) + ".vsp3"
                fname_res = "Hershey_U" + int(self.m_Tess_U[u]) + "_W" + int(self.m_Tess_W[w])+ "_res.csv"
                
                #==== Open and test generated wings ====#
                print("Reading in file: ", False )
                print( fname )
                vsp.ReadVSPFile( fname ) # Sets VSP3 file name

                #==== Analysis: VSPAEROSinglePoint ====#
                print( const.m_VSPSingleAnalysis )

                #==== Analysis: VSPAero Compute Geometry to Create Vortex Lattice DegenGeom File ====#
                print( const.m_CompGeomAnalysis )

                # Set defaults
                vsp.SetAnalysisInputDefaults( const.m_CompGeomAnalysis )
                
                vsp.SetIntAnalysisInput(const.m_CompGeomAnalysis, "Symmetry", const.m_SymFlagVec, 0)

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
                vsp.SetIntAnalysisInput(const.m_VSPSingleAnalysis, "GeomSet", const.m_GeomVec, 0)
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
                if ( len(rid_vec) > 0 ):
                
                    # Get History Results (rid_vec[0]) from Final Wake Iteration in History Result
                    cl_vec = vsp.GetDoubleResults( rid_vec[0], "CL" )
                    Cl_alpha_vsp = cl_vec[int(len(cl_vec)) - 1] # alpha = 1.0 deg
                    
                    Error_Cla[u][w] = (abs((Cl_alpha_vsp - Cl_alpha_theo)/Cl_alpha_theo))*100
                
                
                time_vec = vsp.GetDoubleResults( rid, "Analysis_Duration_Sec" )
                
                if ( len(time_vec) > 0 ):
                
                    Exe_Time[u][w] = time_vec[0]
                
                
                vsp.ClearVSPModel()

    def testHersheyBarTCWings(self):
        print( "-> Begin Hershey Bar Tip Clustering Study:\n" )
        
        num_TC = len(self.m_Tip_Clus)
        x = 1 # AR
        
        span_loc_data=[[]]*(num_TC) #array<array<double>>
        cl_dist_data=[[]]*(num_TC) #array<array<double>>
        cd_dist_data=[[]]*(num_TC) #array<array<double>>

        for  t in range(num_TC):
        
            fname = "Hershey_TC" + float(self.m_Tip_Clus[t]) + ".vsp3"
            fname_res = "Hershey_TC" + float(self.m_Tip_Clus[t]) + "_res.csv"
            
            
            #==== Open and test generated wings ====#
            print("Reading in file: ", False )
            print( fname )
            vsp.ReadVSPFile( fname ) # Sets VSP3 file name

            #==== Analysis: VSPAEROSinglePoint ====#
            print( const.m_VSPSingleAnalysis )

            #==== Analysis: VSPAero Compute Geometry to Create Vortex Lattice DegenGeom File ====#
            print( const.m_CompGeomAnalysis )

            # Set defaults
            vsp.SetAnalysisInputDefaults( const.m_CompGeomAnalysis )
            
            vsp.SetIntAnalysisInput(const.m_CompGeomAnalysis, "Symmetry", const.m_SymFlagVec, 0)

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
            vsp.SetIntAnalysisInput(const.m_VSPSingleAnalysis, "GeomSet", const.m_GeomVec, 0)
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

            # Get Load Result ID
            load_rid = vsp.FindLatestResultsID( "VSPAERO_Load" )
            if ( load_rid != "" ):
            
                # Lift Distribution:
                span_loc_data[t] = vsp.GetDoubleResults( load_rid, "Yavg" )
                cl_dist_data[t] = vsp.GetDoubleResults( load_rid, "cl" )
                cd_dist_data[t] = vsp.GetDoubleResults( load_rid, "cd" )
            
            
            vsp.ClearVSPModel()

    def testHersheyBarUTessWings(self):
        print("-> Begin Hershey Bar U Tesselation Study:\n")
        
        x = 1 # AR
        numUTess = len(self.m_Tess_U)
        
        span_loc_data=[[]]*(numUTess) #array<array<double>>
        cl_dist_data=[[]]*(numUTess) #array<array<double>>
        cd_dist_data=[[]]*(numUTess) #array<array<double>>
        
        for u in range(numUTess):
        
            fname = "Hershey_U" + int(self.m_Tess_U[u]) + ".vsp3"
            fname_res = "Hershey_U" + int(self.m_Tess_U[u]) + "_res.csv"
            
            #==== Open and test generated wings ====#
            print("Reading in file: ", False)
            print( fname )
            vsp.ReadVSPFile( fname ) # Sets VSP3 file name

            #==== Analysis: VSPAEROSinglePoint ====#
            print( const.m_VSPSingleAnalysis )

            #==== Analysis: VSPAero Compute Geometry to Create Vortex Lattice DegenGeom File ====#
            print( const.m_CompGeomAnalysis )

            # Set defaults
            vsp.SetAnalysisInputDefaults( const.m_CompGeomAnalysis )
            
            vsp.SetIntAnalysisInput(const.m_CompGeomAnalysis, "Symmetry", const.m_SymFlagVec, 0)

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
            vsp.SetIntAnalysisInput(const.m_VSPSingleAnalysis, "GeomSet", const.m_GeomVec, 0)
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

            # Get Load Result ID
            load_rid = vsp.FindLatestResultsID( "VSPAERO_Load" )
            if ( load_rid != "" ):
            
                # Lift Distribution:
                span_loc_data[u] = vsp.GetDoubleResults( load_rid, "Yavg" )
                cl_dist_data[u] = vsp.GetDoubleResults( load_rid, "cl" )
                cd_dist_data[u] = vsp.GetDoubleResults( load_rid, "cd" )
            
            
            vsp.ClearVSPModel()

    def testHersheyBarWTessWings(self):
        print("-> Begin Hershey Bar W Tesselation Study:\n")
        
        x = 1 # AR
        numWTess = len(self.m_Tess_W)
        
        span_loc_data=[[]]*(numWTess) #array<array<double>> 
        cl_dist_data=[[]]*(numWTess) #array<array<double>>
        cd_dist_data=[[]]*(numWTess) #array<array<double>> 
        
        for w in range(numWTess):
        
            fname = "Hershey_W" + int(self.m_Tess_W[w]) + ".vsp3"
            fname_res = "Hershey_W" + int(self.m_Tess_W[w]) + "_res.csv"
            
            #==== Open and test generated wings ====#
            print("Reading in file: ", False )
            print( fname )
            vsp.ReadVSPFile( fname ) # Sets VSP3 file name

            #==== Analysis: VSPAEROSinglePoint ====#
            print( const.m_VSPSingleAnalysis )

            #==== Analysis: VSPAero Compute Geometry to Create Vortex Lattice DegenGeom File ====#
            print( const.m_CompGeomAnalysis )

            # Set defaults
            vsp.SetAnalysisInputDefaults( const.m_CompGeomAnalysis )
            
            vsp.SetIntAnalysisInput(const.m_CompGeomAnalysis, "Symmetry", const.m_SymFlagVec, 0)

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
            vsp.SetIntAnalysisInput(const.m_VSPSingleAnalysis, "GeomSet", const.m_GeomVec, 0)
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

            # Get Load Result ID
            load_rid = vsp.FindLatestResultsID( "VSPAERO_Load" )
            if ( load_rid != "" ):
            
                # Lift Distribution:
                span_loc_data[w] = vsp.GetDoubleResults( load_rid, "Yavg" )
                cl_dist_data[w] = vsp.GetDoubleResults( load_rid, "cl" )
                cd_dist_data[w] = vsp.GetDoubleResults( load_rid, "cd" )
            
            
            vsp.ClearVSPModel()

    def testHersheyBarWakeWings(self):
        print("-> Begin Hershey Bar Wake Study:\n")
        
        num_Wake = len(self.m_WakeIter)
        x = 1 # AR
        
        wake_span_loc_data = [[]]*(num_Wake) #array<array<double>>
        wake_cl_dist_data=[[]]*(num_Wake) #array<array<double>>
        computation_time=[0.0]*(num_Wake) #array<double>
        
        # Wake Iteration Study
        for i in range(num_Wake):
        
            fname = "Hershey_Wake" + int(self.m_WakeIter[i]) + ".vsp3"
            fname_res = "Hershey_Wake" + int(self.m_WakeIter[i]) + "_res.csv"
    
            #==== Open and test generated wings ====#
            print( "Reading in file: " , False)
            print( fname )
            self.ReadVSPFile( fname ) # Sets VSP3 file name
            
            #==== Analysis: VSPAEROSinglePoint ====#
            print( const.m_VSPSingleAnalysis )

            #==== Analysis: VSPAero Compute Geometry to Create Vortex Lattice DegenGeom File ====#
            print( const.m_CompGeomAnalysis )

            # Set defaults
            self.SetAnalysisInputDefaults( const.m_CompGeomAnalysis )

            self.SetIntAnalysisInput(const.m_CompGeomAnalysis, "Symmetry", const.m_SymFlagVec, 0)
            
            # list inputs, type, and current values
            self.PrintAnalysisInputs( const.m_CompGeomAnalysis )

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
            vsp.SetIntAnalysisInput(const.m_VSPSingleAnalysis, "GeomSet", const.m_GeomVec, 0)
            vsp.SetIntAnalysisInput(const.m_VSPSingleAnalysis, "RefFlag", const.m_RefFlagVec, 0)

            wid = vsp.FindGeomsWithName( "WingGeom" )
            vsp.SetStringAnalysisInput(const.m_VSPSingleAnalysis, "WingID", wid, 0)

            # Freestream Parameters
            Alpha = [1.0]
            vsp.SetDoubleAnalysisInput(const.m_VSPSingleAnalysis, "Alpha", Alpha, 0)
            vsp.SetDoubleAnalysisInput(const.m_VSPSingleAnalysis, "Mach", const.m_MachVec, 0)
            wake_vec = [self.m_WakeIter[i]]
            vsp.SetIntAnalysisInput(const.m_VSPSingleAnalysis, "WakeNumIter", wake_vec, 0)
            vsp.SetIntAnalysisInput(const.m_VSPSingleAnalysis, "Symmetry", const.m_SymFlagVec, 0)

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

            # Get Load Result ID
            load_rid = vsp.FindLatestResultsID( "VSPAERO_Load" )
            if ( load_rid != "" ):
            
                # Lift Distribution:
                wake_span_loc_data[i] = vsp.GetDoubleResults( load_rid, "Yavg" )
                wake_cl_dist_data[i] = vsp.GetDoubleResults( load_rid, "cl" )
            
            
            time_vec = vsp.GetDoubleResults( rid, "Analysis_Duration_Sec" )
            
            if ( len(time_vec) > 0 ):
            
                computation_time[i] = time_vec[0]
            
            
            vsp.ClearVSPModel()

    def testHersheyBarAdvancedWings(self):
        print("-> Begin Hershey Bar Advanced Settings Study:\n")
        
        x = 2 # AR
        t = 2 # Tip Clustering
    
        num_case = 4 # Number of advanced VSPAERO settings to test
        num_wake = len(self.m_AdvancedWakeVec)

        m_AdvancedTimeVec.resize(num_wake)
        m_HersheyLDAdvancedIDVec.resize(num_wake)
        
        for w in range (num_wake):
        
            span_loc_data=[[]]*(num_case) # array<array<double>>
            cl_dist_data=[[]](num_case) #array<array<double>>
            m_AdvancedTimeVec[w].resize(num_case)
            
            for i in range( num_case ):
            
                fname = "Hershey_Advanced_" + int(i) + ".vsp3"
                fname_res = "Hershey_Advanced_" + int(i) + "_res.csv"
        
                #==== Open and test generated wings ====#
                print("Reading in file: " , False )
                print( fname )
                vsp.ReadVSPFile( fname ) # Sets VSP3 file name
                
                #==== Analysis: VSPAEROSinglePoint ====#
                print( const.m_VSPSingleAnalysis )

                #==== Analysis: VSPAero Compute Geometry to Create Vortex Lattice DegenGeom File ====#
                print( const.m_CompGeomAnalysis )

                # Set defaults
                vsp.SetAnalysisInputDefaults( const.m_CompGeomAnalysis )
                
                vsp.SetIntAnalysisInput(const.m_CompGeomAnalysis, "Symmetry", const.m_SymFlagVec, 0)

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
                vsp.SetIntAnalysisInput(const.m_VSPSingleAnalysis, "GeomSet", const.m_GeomVec, 0)
                vsp.SetIntAnalysisInput(const.m_VSPSingleAnalysis, "RefFlag", const.m_RefFlagVec, 0)

                wid = vsp.FindGeomsWithName( "WingGeom" )
                vsp.SetStringAnalysisInput(const.m_VSPSingleAnalysis, "WingID", wid, 0)

                # Freestream Parameters
                Alpha=[ 1.0]
                vsp.SetDoubleAnalysisInput(const.m_VSPSingleAnalysis, "Alpha", Alpha, 0)
                vsp.SetDoubleAnalysisInput(const.m_VSPSingleAnalysis, "Mach", const.m_MachVec, 0)
                wake_vec=[self.m_WakeIter[w]]
                vsp.SetIntAnalysisInput(const.m_VSPSingleAnalysis, "WakeNumIter", wake_vec, 0)
                vsp.SetIntAnalysisInput(const.m_VSPSingleAnalysis, "Symmetry", const.m_SymFlagVec, 0)

                # if i == 0 -> use default advanced settings
                
                if ( i == 1 ):
                    precon_vec=[vsp.PRECON_JACOBI] # Jacobi Preconditioner
                    vsp.SetIntAnalysisInput(const.m_VSPSingleAnalysis, "Precondition", precon_vec, 0)
                
                elif ( i == 2 ):
                    precon_vec=[vsp.PRECON_SSOR] # SSOR Preconditioner
                    vsp.SetIntAnalysisInput(const.m_VSPSingleAnalysis, "Precondition", precon_vec, 0)
                
                elif ( i == 3 ):
                    KTCorrect_vec=[0] # 2nd Orrder Mach Correction On
                    vsp.SetIntAnalysisInput(const.m_VSPSingleAnalysis, "KTCorrection", KTCorrect_vec, 0)
                
                
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

                # Get Load Result ID
                load_rid = vsp.FindLatestResultsID( "VSPAERO_Load" )
                if ( load_rid != "" ):
                
                    # Lift Distribution:
                    span_loc_data[i] = vsp.GetDoubleResults( load_rid, "Yavg" )
                    cl_dist_data[i] = vsp.GetDoubleResults( load_rid, "cl" )
                
                
                time_vec = vsp.GetDoubleResults( rid, "Analysis_Duration_Sec" )
            
                if ( len(time_vec) > 0 ):
                
                    m_AdvancedTimeVec[w][i] = time_vec[0]
                
                
                vsp.ClearVSPModel()
