import openvsp as vsp


class VandVTest:

    '''! Master VandVTest class.
    
    Contains flags for all tests along with the objects
    generated from each test. 
    '''
    def __init__(self):

        #Set the flags for which studies to run
        self.hersheyFlag = True
        self.sweptFlag = False
        self.bertinFlag = False
        self.warrenFlag =  False
        self.VKTFlag = False
        self.ellipseFlag = False
        self.superDeltaFlag = False

        #Internal Classes for each study
        self.hersheyObj = None
        self.sweptObj = None
        self.bertinObj = None
        self.warrenObj = None
        self.VKTObj = None
        self.ellipseObj = None
        self.superDeltaObj = None

        #Internal Members
        self.htmlFile = None
        self.file = None

    def OpenHTMLFile(self):
        self.version = vsp.GetVSPVersion() #GetVSPVersion
        self.file = open(self.version+"_VV.html")

    def TestAll(self):
        if (self.hersheyFlag):
            self.hersheyObj = None #Replace with HersheyTest object contructor
            self.hersheyObj #Call HersheyBarStudy

        if (self.sweptFlag):
            self.sweptObj = None #Replace with HersheyTest object contructor
            self.sweptObj #Call HersheyBarStudy

        if (self.bertinFlag):
            self.bertinObj = None #Replace with HersheyTest object contructor
            self.bertinObj #Call HersheyBarStudy
            
        if (self.warrenFlag):
            self.warrenObj = None #Replace with HersheyTest object contructor
            self.hersheyObj #Call HersheyBarStudy
            
        if (self.VKTFlag):
            self.VKTObj = None #Replace with HersheyTest object contructor
            self.VKTObj #Call HersheyBarStudy
            
        if (self.ellipseFlag):
            self.ellipseObj = None #Replace with HersheyTest object contructor
            self.ellipseObj #Call HersheyBarStudy
            
        if (self.superDeltaFlag):
            self.superDeltaObj = None #Replace with HersheyTest object contructor
            self.superDeltaObj #Call HersheyBarStudy
    
    def generateHTML(self):
        pass
        


def main():
    print("Begining Maseter VSP V&V Script")
    masterVandVTest = VandVTest()


    masterVandVTest.TestAll()

    #masterVandVTest.OpenHTMLFile()

    #masterVandVTest.generateHTML()

    errorMgr :vsp.ErrorMgrSingleton = vsp.ErrorMgrSingleton_getInstance()
    #Check for API Errors 
    while(errorMgr.GetNumTotalErrors() >0):
        err:vsp.ErrorObj = errorMgr.PopLastError()
        print(err.GetErrorString())

    print("End Master VSP V&V Script")

if __name__ == "__main__":
    main()
    