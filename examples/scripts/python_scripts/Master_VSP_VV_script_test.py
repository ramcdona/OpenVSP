import subprocess
import sys
from pathlib import Path

def main():
    currentpath = str(Path(__file__).parent.resolve())
    subprocess.Popen([sys.executable, currentpath+'/SuperDeltaTest.py'],start_new_session=True)
    subprocess.Popen([sys.executable, currentpath+'/EllipseTest.py'],start_new_session=True)
    subprocess.Popen([sys.executable, currentpath+'/VKTTest.py'],start_new_session=True)
    subprocess.Popen([sys.executable, currentpath+'/WarrenTest.py'],start_new_session=True)
    subprocess.Popen([sys.executable, currentpath+'/BertinSmithTest.py'],start_new_session=True)
    subprocess.Popen([sys.executable, currentpath+'/SweptTest.py'],start_new_session=True)
    subprocess.Popen([sys.executable, currentpath+'/HersheyTest.py'],start_new_session=True)
    return


if __name__ == '__main__':
    main()