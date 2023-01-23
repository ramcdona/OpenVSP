import openvsp as vsp

def Test_All():
    pass

def main():
    print("Begining MAseter VSP V&V Script")
    version = vsp.GetVSPVersion() #GetVSPVersion
    file = open(version+"_VV.html")
    test = Test_All()
    errorMgr :vsp.ErrorMgrSingleton = vsp.ErrorMgrSingleton_getInstance()

    #Check for API Errors 
    while(errorMgr.GetNumTotalErrors() >0):
        err:vsp.ErrorObj = errorMgr.PopLastError()
        print(err.GetErrorString())


    
    print("End Master VSP V&V Script")

if __name__ == "__main__":
    main()