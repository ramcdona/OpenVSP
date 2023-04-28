import subprocess
import sys

def main():
    subprocess.Popen([sys.executable, './SuperDeltaTest.py'],start_new_session=True)
    subprocess.Popen([sys.executable, './EllipseTest.py'],start_new_session=True)
    subprocess.Popen([sys.executable, './VKTTest.py'],start_new_session=True)
    subprocess.Popen([sys.executable, './WarrenTest.py'],start_new_session=True)
    subprocess.Popen([sys.executable, './BertinSmithTest.py'],start_new_session=True)
    subprocess.Popen([sys.executable, './SweptTest.py'],start_new_session=True)
    subprocess.Popen([sys.executable, './HersheyTest.py'],start_new_session=True)
    return


if __name__ == '__main__':
    main()