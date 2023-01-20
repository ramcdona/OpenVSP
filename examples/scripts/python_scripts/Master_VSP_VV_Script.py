import ~/PrivateVSP_ESAero/build/python_apivsp.py

def main():
    print("Beging MAseter VSP V&V Script")
    version = v.getVSPVersion() #GetVSPVersion
    open(version+"_VV.html")

    
    print("End Master VSP V&V Script")

if __name__ == "__main__":
    main()