//
// This file is released under the terms of the NASA Open Source Agreement (NOSA)
// version 1.3 as detailed in the LICENSE file which accompanies this software.
//
/// \class AwaveScreen
/// The AWAVE slicer UI
//
//////////////////////////////////////////////////////////////////////

#ifndef AwaveScreen_H_
#define AwaveScreen_H_

#include "VspScreenQt.h"

class AwaveScreenPrivate;
class AwaveScreen : public VspScreenQt
{
    VSP_DECLARE_PRIVATE( AwaveScreen )
public:
    AwaveScreen( ScreenMgr* mgr );
    ~AwaveScreen();
};

#endif /* AwaveScreen_H_ */
