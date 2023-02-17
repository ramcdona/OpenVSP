# <div align="center">OpenVSP Verification & Validation Studies</div>
#### <div align="center">[OpenVSP Home](http://www.openvsp.org) | [OpenVSP Wiki](http://www.openvsp.org/wiki/doku.php)</div>
## <div align="center"> Introduction </div>


This document outlines a series of verification and validation (V&V) studies that have been conducted for the VSPAERO aerodynamic solver.
This HTML wasgenerated from a single *.vspscript file named "Master_VSP_VV_Script.vspscript" located in the scripts directory distributed with the
OpenVSP software. In each of the V&V test cases listed below, there exists published theoretical or experimental results that can be verified for a
particular geometric model. The results from VSPAERO are then able to be validated against the published results to determine the accuracy of the
solver. Each test begins by modeling a wing or series of wing geometries through the OpenVSP API. The geometric parameters are listed in a table for
each study. If a parameter is not listed in the table the default value in OpenVSP should be assumed. Each generated model is saved, and therefore can
be loaded in OpenVSP to verify the model accurately represents the geometry of the V&V study. Depending on the particular test case, the results needed
for validation must then be identified or calculated. Once results for validation are established, VSPAERO is setup and executed through the Analysis
Manager. The VSPAERO analysis inputs are listed in a table for each study, with default values used for items not identified in the tables.
Following VSPAERO execution, results are obtained through the Results Manager, compared to the published results, and displayed.

## <div align="center"> Notation </div>
Need to sort out equation section
AR 	$\equiv$	Aspect Ratio


<embed type="text/html" src="./test.html" width="1000" height="1000"></embed>
## <div align="center"> Equations</div>
Lifting Line Theory (LLT) provides the following equations outlined in the NACA TN 3911 *A Method for Predicting Lift Increments Due to Flap Deflection
at Low Angles of Attack in Incompressible Flow* by Lowry and Polhamus:
__INSERT EQATION HERE__
$$C_{L\alpha} = \frac{2\pi AR }{2 + \sqrt{(\frac{AR^2 B^2}{K^2})(1 + \frac{tan^2 \Lambda_{c/2}}{B^2}) + 4}}$$