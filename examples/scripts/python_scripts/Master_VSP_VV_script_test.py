import subprocess
import sys
from pathlib import Path

def main():
    scriptpath = str(Path(__file__).parent.resolve())
    print(scriptpath)
    subprocess.Popen([sys.executable, scriptpath+'/SuperDeltaTest.py'],start_new_session=True)
    subprocess.Popen([sys.executable, scriptpath+'/EllipseTest.py'],start_new_session=True)
    subprocess.Popen([sys.executable, scriptpath+'/VKTTest.py'],start_new_session=True)
    subprocess.Popen([sys.executable, scriptpath+'/WarrenTest.py'],start_new_session=True)
    subprocess.Popen([sys.executable, scriptpath+'/BertinSmithTest.py'],start_new_session=True)
    subprocess.Popen([sys.executable, scriptpath+'/SweptTest.py'],start_new_session=True)
    subprocess.Popen([sys.executable, scriptpath+'/HersheyTest.py'],start_new_session=True)
    return


if __name__ == '__main__':
    main()