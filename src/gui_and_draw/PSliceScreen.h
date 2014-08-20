//
// This file is released under the terms of the NASA Open Source Agreement (NOSA)
// version 1.3 as detailed in the LICENSE file which accompanies this software.
//
//////////////////////////////////////////////////////////////////////

#ifndef PSLICESCREEN_H_
#define PSLICESCREEN_H_

#include "VspScreenQt.h"

class PSliceScreen : public VspScreenQt
{
public:
    PSliceScreen( ScreenMgr* mgr );
    ~PSliceScreen();
};

#endif /* PSLICESCREEN_H_ */
