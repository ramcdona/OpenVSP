import openvsp as vsp
from bokeh.plotting import figure,
from bokeh.io import export_png

class SweptTest:
    '''!Class for running and collecting data from the
        swept wing studies
    '''
    def __init__self():
        #Which studies to run
        uw = True
        ar = True
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
        pass

#========== Run the actual ____________ Studies ==============================#
    def TestSweptUWTessWings(self):
        #Insert lines 4955-5056 from v&v script
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