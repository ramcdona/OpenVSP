import openvsp as vsp
import math
import Constants as const
import traceback
from pathlib import Path
from bokeh.plotting import figure,output_file, show
from bokeh.io import export_png
import pickle

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
        self.Error_Cla_W_Tess_Sensitivity = [[0.0]*len(self.m_Tess_U) for i in range(len(self.m_Tess_W))]
        self.Cl_alpha_vlm = [[0.0]*len(self.m_Sweep) for i in range(len(self.m_halfAR))]
        self.Cl_alpha_pm = [[0.0]*len(self.m_Sweep) for i in range(len(self.m_halfAR))]
        self.Cl_alpha_theo_multi = [[0.0]*len(self.m_Sweep) for i in range(len(self.m_halfAR))]
        self.Avg_Cla_Error_VLM = [0.0]*len(self.m_Sweep)
        self.Avg_Cla_Error_PM = [0.0]*len(self.m_Sweep)
        
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
                fname = "swept_files/vsp_files/Swept_U" + str(self.m_Tess_U[u]) + "_W" + str(self.m_Tess_W[w]) + ".vsp3"

                # Save Vehicle to File
                message = "-->Saving vehicle file to: " + fname + "\n"
                print( message )
                vsp.WriteVSPFile( fname, vsp.SET_ALL )
                print( "COMPLETE\n" )
            
        
        vsp.ClearVSPModel()

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
                fname = "swept_files/vsp_files/Swept_U" + str(self.m_Tess_U[u]) + "_W" + str(self.m_Tess_W[w]) + ".vsp3"
                fname_res = "swept_files/vsp_files/Swept_U" + str(self.m_Tess_U[u]) + "_W" + str(self.m_Tess_W[w]) + "_res.csv"

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
                    Cl_res = cl_vec[len(cl_vec) - 1]

                    # Calculate Error
                    C_l_alpha_vsp = Cl_res # alpha = 1.0 (deg)
                    self.Error_Cla[u][w] = (abs((C_l_alpha_vsp - self.Cl_alpha_theo)/self.Cl_alpha_theo))*100
                    self.Error_Cla_W_Tess_Sensitivity[w][u] = self.Error_Cla[u][w]
                
                
                vsp.ClearVSPModel()
        
#======== Use Bokeh to Create tables and Graphs for the _________ Studies =#
    def GenerateSweptUWTessCharts(self):
        p = figure(width=const.bokehwidth,height=const.bokehheight, title="Swept Wing VLM Cl_alpha Span Tesselation (U Tess) Sensitivity",x_axis_label="Chord Tesselation (W Tess)", y_axis_label=r"Cl_alpha % Error")
        for i in range(len(self.Error_Cla)):
            p.line(self.m_Tess_W,self.Error_Cla[i], legend_label="U Tess:"+str(self.m_Tess_U[i]),color=const.bokehcolors[i],line_width=const.bokehlinewidth)
            p.circle(self.m_Tess_W,self.Error_Cla[i], color=const.bokehcolors[i],size=const.bokehsize)
        export_png(p,filename="swept_files/swept_img/span_tesselation/span_tess.png")
        p = figure(width=const.bokehwidth,height=const.bokehheight, title="Swept Wing VLM Cl_alpha Chord Tesselation (W Tess) Sensitivity",x_axis_label="Span Tesselation (U Tess)", y_axis_label=r"Cl_alpha % Error")
        for i in range(len(self.Error_Cla)):
            p.line(self.m_Tess_U,self.Error_Cla_W_Tess_Sensitivity[i], legend_label="W Tess:"+str(self.m_Tess_W[i]),color=const.bokehcolors[i],line_width=const.bokehlinewidth)
            p.circle(self.m_Tess_U,self.Error_Cla_W_Tess_Sensitivity[i], color=const.bokehcolors[i],size=const.bokehsize)
        export_png(p,filename="swept_files/swept_img/chord_tesselation/chord_tess.png")
        

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
         #==== Add Wing Geometry ====#
        wing_id = vsp.AddGeom( "WING", "" )
        
        #==== Set Wing Section ====#
        vsp.SetDriverGroup( wing_id, 1, vsp.AR_WSECT_DRIVER, vsp.ROOTC_WSECT_DRIVER, vsp.TIPC_WSECT_DRIVER )
        
        vsp.Update()
        
        #==== Set NACA 0012 Airfoil and Common Parms 
        vsp.SetParmVal( wing_id, "ThickChord", "XSecCurve_0", 0.12 )
        vsp.SetParmVal( wing_id, "ThickChord", "XSecCurve_1", 0.12 )
        vsp.SetParmVal( wing_id, "Root_Chord", "XSec_1", 1.0 )
        vsp.SetParmVal( wing_id, "Tip_Chord", "XSec_1", 1.0 )
        
        vsp.SetParmVal( wing_id, "TECluster", "WingGeom", 1.0 )
        vsp.SetParmVal( wing_id, "LECluster", "WingGeom", 0.2 )
        
        u = 3 # UTess
        w = 3 # WTess
        
        vsp.SetParmVal( wing_id, "SectTess_U", "XSec_1", self.m_Tess_U[u] ) # Constant U Tess
        vsp.SetParmVal( wing_id, "Tess_W", "Shape", self.m_Tess_W[w] ) # Constant W Tess
        vsp.SetParmVal( wing_id, "OutCluster", "XSec_1", 1.0 ) # Constant Tip Clustering
        
        vsp.Update()
        
        for x in range(len(self.m_halfAR)):
            for s in range(len(self.m_Sweep)):
                vsp.SetParmVal( wing_id, "Aspect", "XSec_1", self.m_halfAR[x] )
                vsp.SetParmVal( wing_id, "Sweep", "XSec_1", self.m_Sweep[s] )

                vsp.Update()

                #==== Setup export filenames for AR Study ====#
                fname = "swept_files/vsp_files/Swept_AR" + str(2*self.m_halfAR[x]) + "_Sweep" + str(self.m_Sweep[s]) + ".vsp3"

                #==== Save Vehicle to File ====#
                message = "-->Saving vehicle file to: " + fname + "\n"
                print( message )
                vsp.WriteVSPFile( fname, vsp.SET_ALL )
                print( "COMPLETE\n" )

        
        vsp.ClearVSPModel()
        
#========== Run the actual ____________ Studies ==============================#
    def TestSweptARSweepWings(self):
        #Insert lines 4663-4848 from v&v script
        print( "-> Begin Swept Wing AR Sweep Test:\n" )
        
        num_AR = len(self.m_halfAR)
        num_Sweep = len(self.m_Sweep)
        
        Lift_angle_vlm = [[0]*num_Sweep for i in range(num_AR)]
        Lift_angle_theo = [[0]*num_Sweep for i in range(num_AR)]
        #Cl_alpha_vlm = [[0]*num_Sweep for i in range(num_AR)]
        #Cl_alpha_theo = [[0]*num_Sweep for i in range(num_AR)]
        Lift_angle_pm = [[0]*num_Sweep for i in range(num_AR)]
        #Cl_alpha_pm = [[0]*num_Sweep for i in range(num_AR)]
        
        #Avg_Cla_Error_VLM = [0.0] * num_Sweep
        Cla_Error_VLM = [0.0] * num_Sweep
        C_ratio = [0.0]*num_AR
        #Avg_Cla_Error_PM = [0.0] * num_Sweep
        
              
        for x in range(num_AR):
            
            C_ratio[x] = 1/(2*self.m_halfAR[x]) # AR to chord ratio
            
            for s in range(num_Sweep):
            
                # Open the file
                fname = "swept_files/vsp_files/Swept_AR" + str(2*self.m_halfAR[x]) + "_Sweep" + str(self.m_Sweep[s]) + ".vsp3"
                fname_res_vlm = "swept_files/vsp_files/Swept_AR" + str(2*self.m_halfAR[x]) + "_Sweep" + str(self.m_Sweep[s]) + "_vlm_res.csv"
                fname_res_pm = "swept_files/vsp_files/Swept_AR" + str(2*self.m_halfAR[x]) + "_Sweep" + str(self.m_Sweep[s]) + "_pm_res.csv"

                print( "Reading in file: " )
                print( fname )
                vsp.ReadVSPFile( fname ) # Sets VSP3 file name
                
                #==== Analysis: VSPAeroSinglePoint ====#
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
                vsp.SetIntAnalysisInput( const.m_VSPSingleAnalysis, "GeomSet", const.m_GeomVec, 0 )
                vsp.SetIntAnalysisInput(const.m_VSPSingleAnalysis, "RefFlag", const.m_RefFlagVec, 0)
                vsp.SetIntAnalysisInput(const.m_VSPSingleAnalysis, "Symmetry", const.m_SymFlagVec, 0)

                wid = vsp.FindGeomsWithName( "WingGeom" )
                vsp.SetStringAnalysisInput(const.m_VSPSingleAnalysis, "WingID", wid, 0)

                # Freestream Parameters
                Alpha = [1.0]
                vsp.SetDoubleAnalysisInput(const.m_VSPSingleAnalysis, "Alpha", Alpha, 0)
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
                vsp.WriteResultsCSVFile( rid, fname_res_vlm )

                # Calculate Experimental and Theoretical Values
                C_bot_two = 1 + (pow((math.tan(math.radians(self.m_Sweep[s]))),2)/pow(const.b,2))
                C_top = 2*math.pi*2*self.m_halfAR[x]
                C_bot_one_theo = (pow((2*self.m_halfAR[x]),2)*pow(const.b,2))/pow(const.k_theo,2)
                C_bot_theo = (2 + (math.sqrt((C_bot_one_theo*C_bot_two)+4)))
                C_l_alpha_exper_theo = C_top/C_bot_theo
                self.Cl_alpha_theo_multi[x][s] = math.degrees(C_l_alpha_exper_theo) # rad --> deg
                Lift_angle_theo[x][s] = 1/self.Cl_alpha_theo_multi[x][s] # Cl to lift angle
                
                # Get Result ID Vec (History and Load ResultIDs)
                rid_vec = vsp.GetStringResults( rid, "ResultsVec" )
                if len(rid_vec) > 0 :
                
                    # Get History Results (rid_vec[0]) from Final Wake Iteration in History Result
                    cl_vec = vsp.GetDoubleResults( rid_vec[0], "CL" )
                    Cl_res = cl_vec[len(cl_vec) - 1]
                    self.Cl_alpha_vlm[x][s] = Cl_res # alpha = 1.0 (deg)
                    Lift_angle_vlm[x][s] = 1/(self.Cl_alpha_vlm[x][s]) # deg
                    
                    # Add error
                    self.Avg_Cla_Error_VLM[s] += abs((self.Cl_alpha_vlm[x][s] - self.Cl_alpha_theo_multi[x][s])/self.Cl_alpha_theo_multi[x][s])/len(self.m_Sweep)
                
                
                #==== Analysis: VSPAero Panel Single ====#
                print( const.m_VSPSingleAnalysis )
                
                #==== Analysis: VSPAero Compute Geometry to Create Vortex Lattice DegenGeom File ====#
                print( const.m_CompGeomAnalysis )

                # Set defaults
                vsp.SetAnalysisInputDefaults( const.m_CompGeomAnalysis )
                
                panel_analysis = [vsp.PANEL]
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
                vsp.SetIntAnalysisInput(const.m_VSPSingleAnalysis, "AnalysisMethod", panel_analysis)
                vsp.SetIntAnalysisInput(const.m_VSPSingleAnalysis, "Symmetry", const.m_SymFlagVec, 0)
                
                # Freestream Parameters
                alpha= [1]
                vsp.SetDoubleAnalysisInput(const.m_VSPSingleAnalysis, "Alpha", alpha, 0)
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
                if len(rid_vec) > 0:
                    # Get Result from Final Wake Iteration
                    cl_vec = vsp.GetDoubleResults( rid_vec[0], "CL" )
                    
                    Cl_pm = cl_vec[len(cl_vec) - 1]
                    self.Cl_alpha_pm[x][s] = Cl_pm # deg (alpha = 1.0°)
                    Lift_angle_pm[x][s] = 1/(self.Cl_alpha_pm[x][s]) # deg
                    
                    # Add error
                    self.Avg_Cla_Error_PM[s] += abs((self.Cl_alpha_pm[x][s] - self.Cl_alpha_theo_multi[x][s])/self.Cl_alpha_theo_multi[x][s])/len(self.m_Sweep)
                
                vsp.ClearVSPModel()
#======== Use Bokeh to Create tables and Graphs for the _________ Studies =#
    def GenerateSweptARSweepCharts(self):
        for i in range(len(self.m_Sweep)):
            p = figure(width=const.bokehwidth,height=const.bokehheight, title=str(self.m_Sweep[i])+"° Sweep: Cl_alpha vs. Aspect Ratio",x_axis_label="AR", y_axis_label="Cl_alpha (°)")
            p.line(self.m_halfAR*2, self.Cl_alpha_vlm[i],color=const.bokehcolors[0],legend_label=r"VSPAERO VLM",line_width=const.bokehlinewidth)
            p.circle(self.m_halfAR*2,self.Cl_alpha_vlm[i],color=const.bokehcolors[0],size=const.bokehsize)
            
            p.line(self.m_halfAR*2, self.Cl_alpha_pm[i],color=const.bokehcolors[1],legend_label=r"VSPAERO Panel",line_width=const.bokehlinewidth)
            p.circle(self.m_halfAR*2,self.Cl_alpha_pm[i],color=const.bokehcolors[1],size=const.bokehsize)
            
            p.line(self.m_halfAR*2, self.Cl_alpha_theo_multi[i],color=const.bokehcolors[-1],legend_label=r"LLT",line_width=const.bokehlinewidth)
            p.circle(self.m_halfAR*2,self.Cl_alpha_theo_multi[i],color=const.bokehcolors[-1],size=const.bokehsize)
            
            p.add_layout(p.legend[0],"right")
            export_png(p,filename="swept_files/swept_img/ar_sweep/ar_sweep_deg_"+str(self.m_Sweep[i])+".png")
            
        p = figure(width=const.bokehwidth,height=const.bokehheight, title=str(self.m_Sweep[i])+r"Average % Error in Cl_alpha Across All Aspect Ratios Sweep Sensitivity",x_axis_label="Sweep (°)", y_axis_label=r"Cl_alpha % Error")
        p.line(self.m_Sweep, self.Avg_Cla_Error_VLM*100,color=const.bokehcolors[0],legend_label=r"VLM",line_width=const.bokehlinewidth)
        p.circle(self.m_Sweep, self.Avg_Cla_Error_VLM*100,color=const.bokehcolors[0],size=const.bokehsize)
        
        p.line(self.m_Sweep, self.Avg_Cla_Error_VLM*100,color=const.bokehcolors[1],legend_label=r"Panel Method",line_width=const.bokehlinewidth)
        p.circle(self.m_Sweep, self.Avg_Cla_Error_VLM*100,color=const.bokehcolors[1],size=const.bokehsize)
        
        p.add_layout(p.legend[0],"right")
        export_png(p,filename="swept_files/swept_img/ar_sweep/ar_sweep_avgs.png")

def test_init():
    print("Testing SweptTest __init__()")
    swept = SweptTest()
    print(f"\tm_halfAR {swept.m_halfAR}")
    #print(f"\tm_AlphaNpts {swept.m_AlphaNpts}")
    #print(f"\tm_Tip_Clus {swept.m_Tip_Clus}")
    print(f"\tm_Tess_U {swept.m_Tess_U}")
    #print(f"\tm_WakeIter {swept.m_WakeIter}")
    #print(f"\tm_AdvancedWakeVec {swept.m_AdvancedWakeVec}")
    #print(f"\tm_AR10_Y_Cl_Cd_vec {swept.m_AR10_Y_Cl_Cd_vec}")
    print("\n")
    return swept

def generateCharts(swept: SweptTest):
    swept.GenerateSweptUWTessCharts()
    swept.GenerateSweptARSweepCharts()

def unit_test_swept():
    setup_filepaths()
    swept = test_init()

    test_swept_generate(swept)

    test_swept_test(swept)
    print("New Generate")
    with open(str(Path(__file__).parent.resolve())+'/swept_files/swepttest.pckl',"wb") as picklefile:
        pickle.dump(swept,picklefile)
    with open(str(Path(__file__).parent.resolve())+'/swept_files/swepttest.pckl',"rb") as picklefile:    
        swept = pickle.load(picklefile)
    generateCharts(swept)

def setup_filepaths():
    scriptpath = Path(__file__).parent.resolve()
    testnames = ["swept_files/"]
    subnames = [["swept_img/","vsp_files/"]]
    subsubnames = [[["chord_tesselation","span_tesselation"],[""]]]
    for i in range(len(testnames)):
        for j in range(len(subnames[i])):
            for k in range(len(subsubnames[i][j])):
                dirname = Path.joinpath(scriptpath, testnames[i]+subnames[i][j]+subsubnames[i][j][k])
                dirname.mkdir(parents=True, exist_ok=True)

def test_swept_generate(swept: SweptTest):
    print("Testing Wing Generation")
    try:
        swept.GenerateSweptARSweepWings()
        print("Completed GenerateSweptARSweepWings()")
        print("-------------------------------------")
    except:
        print("\tERROR: Failed GenerateSweptARSweepWings()")
        traceback.print_exc()
        return    
    
def test_swept_test(swept: SweptTest):
    print("Testing Test Functions")
    print("Testing SweptARSweepWings")
    try:
        swept.TestSweptARSweepWings()   
        print("\tCompleted TestSweptARSweepWings()") 
        print("-------------------------------------")
    except:
        print("\tERROR: Failed TestSweptARSweepWings()")
        traceback.print_exc()
        return    

if __name__ == "__main__":
    unit_test_swept()