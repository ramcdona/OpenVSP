//
// This file is released under the terms of the NASA Open Source Agreement (NOSA)
// version 1.3 as detailed in the LICENSE file which accompanies this software.
//
/// \class VspScreenQt
/// The base of Qt-based screens.
//
//////////////////////////////////////////////////////////////////////

#if !defined(VSPSCREENQT__INCLUDED_)
#define VSPSCREENQT__INCLUDED_

#include "VSPScreen.h"
#include <QScopedPointer>

class VspScreenQtPrivate;
class VspScreenQt : public VspScreen
{
    Q_DISABLE_COPY( VspScreenQt )
    Q_DECLARE_PRIVATE( VspScreenQt )
    bool Update() Q_DECL_OVERRIDE;
public:
    ~VspScreenQt();

    void Show() Q_DECL_OVERRIDE;
    void Hide() Q_DECL_OVERRIDE;
    bool IsShown() Q_DECL_OVERRIDE;
    void SetNonModal() Q_DECL_OVERRIDE;
    void GuiDeviceCallBack( GuiDevice* device ) Q_DECL_OVERRIDE;

    virtual int x() Q_DECL_OVERRIDE;
    virtual int y() Q_DECL_OVERRIDE;
    virtual int w() Q_DECL_OVERRIDE;
    virtual int h() Q_DECL_OVERRIDE;
    virtual void position( int X, int Y ) Q_DECL_OVERRIDE;

protected:
    QScopedPointer<VspScreenQtPrivate> const d_ptr;
    VspScreenQt( VspScreenQtPrivate & dd, ScreenMgr * mgr );
};

/// Use in place of Q_DECLARE_PRIVATE for a multiply-inheriting private class.
#define VSP_DECLARE_PRIVATE(Class) \
    friend class Class##Private; \
    Class##Private* d_func(); \
    const Class##Private* d_func() const;

#define VSP_DEFINE_PRIVATE(Class) \
    inline Class##Private* Class::d_func() { return static_cast<Class##Private *>(qGetPtrHelper(d_ptr)); } \
    inline const Class##Private* Class::d_func() const { return static_cast<const Class##Private *>(qGetPtrHelper(d_ptr)); }

#endif // VSPSCREENQT__INCLUDED_
