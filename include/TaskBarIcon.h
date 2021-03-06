/******************************************************************************
 * wxVcashGUI: a GUI for Vcash, a decentralized currency
 *             for the internet (https://vcash.info).
 *
 * Copyright (c) The Vcash Developers
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 ******************************************************************************/

#ifndef TASKBARICON_H
#define TASKBARICON_H

#include <wx/wxprec.h>

#ifndef WX_PRECOMP
#include <wx/taskbar.h>
#include <wx/window.h>
#endif

namespace wxGUI {
    class VcashApp;

    class TaskBarIcon : public wxTaskBarIcon {
    private:
        VcashApp &vcashApp;
    public:
        TaskBarIcon(VcashApp &vcashApp);
        void disable();
        static bool isEnabled();
        wxMenu* CreatePopupMenu();
    };
};

#endif // TASKBARICON_H
