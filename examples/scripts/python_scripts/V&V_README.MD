# <div align="center">OpenVSP Verification & Validation Studies </div>
#### <div align="center">[OpenVSP Home](http://www.openvsp.org) | [OpenVSP Wiki](http://www.openvsp.org/wiki/doku.php)</div>
## Architecture </div>

This python itteration of the V&V studies is implemented via calls to the c++ api via swig generation.
There are requirements for this code, mainly matplotlib used for charting, and pathlib used for consistent file structure resasons.
Each study is broken out into its own python file, which can be run from command line with no arguments. 
(it has been set up for, with a little future work, running with parameters specifying which substudies to run and at what regenration level)
Each study generates its files in the directory of the script itself in a folder named 'testname'+'_files'.
Each study generates its needed vsp files in the vsp_files subdirectory.
Generated images are stored in the 'testname'+'_img' subdirectory, and inside that there may be subdirectories for the subtests being run.
Currently, images are being generated as .svg files, scalable vector graphics, which are infinitely resizable without losing resolution, and can be opened with any web browser or photo app.
If desired the .svg extensions can be replaced with .png and it should all still work as that how it was first implemented.
Each study has 2 main functions, a 'run'+'testname'+'study' function that acts as the pseudo main function, and the setup_filepaths function that is self explanatory.
Each substudy has 3 main functions, one for generating required files, one for testing everything, and a third for creating graphs and tables.
Each study also has the ability to generate a picklefile of the class object, effectively allowing reusing stored computations.
The run modes for each substudy are currently 0 for do nothing, 1 for only regenerate charts which is helpful for debugging charts, 2 for only redoing the testing and charts, and 3 for regenerating everything.
The saved charts are then able to display in the report which is written with Report.MD

## State of the current code