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


## <div align="center"> Equations</div>
Lifting Line Theory (LLT) provides the following equations outlined in the NACA TN 3911 *A Method for Predicting Lift Increments Due to Flap Deflection
at Low Angles of Attack in Incompressible Flow* by Lowry and Polhamus:

$$C_{L\alpha} = \frac{2\pi AR }{2 + \sqrt{(\frac{AR^2 B^2}{K^2})(1 + \frac{tan^2 \Lambda_{c/2}}{B^2}) + 4}}$$
$$B = \sqrt{1-M^2}$$
$$K = C_{L\alpha2D}(\frac{180}{\pi})/(2\pi)$$

## <div align="center"> Case 1: Hershey Bar</div>
Various Hershey Bar wings with AR ranging from 10 to 60 are modeled through the OpenVSP API for this V&V case.
The Hershey Bar wing has been studied and used for aerodynamic verification and validation studies extensively.
The Hershey Bar wing has a unit length chord and no sweep. For the studies presented below, a symmetric NACA0012 airfoil is
modeled for each Heshey Bar wing. A series of studies are conducted that investigate qualities such as tesselation, aspect ratio,
and advanced settings, and their effect on VSPAERO accuracy. 
### Aspect Ratio Study
In the first Hershey Bar study, the resulting coefficient of lift (\(C_L\)) from VSPAERO's vortex lattice solver is compared to the
approximate \(C_L\) determined for each angle of attack from Lifting Line Theory. The first plot listed below displays these results.

Next, the lift curve slope (\(C_{L\alpha}\)) is calculated for each vortex lattice and panel method VSPAERO result, defined by a particular AR,
by subtracting \(C_L\) at two angle of attack flow condition cases and dividing by the total change in \(\alpha\).
This method, known as central differencing, cannot be used at the minimum or maximum angle of attack values.
For those cases, \(C_{L\alpha}\) is identified as the slope of the \(C_L\) vs \(\alpha\) curve immediately before or after the angle of attack endpoint.
These methods are known as forward and backward differencing. The calculated \(C_{L\alpha}\) values are compared to \(C_{L\alpha}\)
determined by Lifting Line Theory at the associated aspect ratios. A 2D lift curve slope of 0.1096622 per degree is assumed, given a theoretical K ratio
of 1.0. This plot can be seen in the second graph below.
#### <div align="center"> Aspect Ratio Study Geometry Setup </div>
| Airfoil | AR  | Root Chord | Tip Chord | \(\Lambda\) (°) | Span Tess (U) | Chord Tess (W) | LE Clustering | TE Clustering | Tip Clustering |
| :-----: | :-: | :--------: | :-------: | :-------------: | :-----------: | :------------: | :-----------: | :-----------: | :------------: |
| NACA0012 | 5 to 60 | 1.0 | 1.0 | 0.0 | 41 | 51 | 0.2 | 1.0 | 1.0|

#### <div align="center"> Aspect Ratio Study VSPAERO Setup </div>

![](hershey_files/hershey_img/ClvA.png)