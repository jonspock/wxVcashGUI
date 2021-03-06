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
 
#include "wx/wxprec.h"

#ifndef WX_PRECOMP
#include <wx/clipbrd.h>
#include <wx/menu.h>
#include "wx/sizer.h"
#endif

#include "AddressesPage.h"
#include "BlockExplorer.h"
#include "QRDialog.h"
#include "ToolsFrame.h"
#include "VcashApp.h"

namespace wxGUI {
    int cmpAddresses(wxIntPtr item1, wxIntPtr item2, wxIntPtr sortDt) {
        AddressesPage::SortData *sortData = (AddressesPage::SortData *) sortDt;
        AddressesPage *addressesPage = sortData->addressesPage;
        wxListCtrl *listCtrl = addressesPage->listCtrl;
        auto order = sortData->order;

        for(int i=0; i<order.size(); i++) {
            auto col = order[i].first;
            int result;

            switch(col) {
                case AddressesPage::Account: {
                    long index1 = listCtrl->FindItem(-1, item1);
                    auto str1 = listCtrl->GetItemText(index1, col);

                    long index2 = listCtrl->FindItem(-1, item2);
                    auto str2 = listCtrl->GetItemText(index2, col);

                    result = str1.Cmp(str2);
                    break;
                }

                case AddressesPage::Address: {
                    long index1 = listCtrl->FindItem(-1, item1);
                    auto str1 = listCtrl->GetItemText(index1, col);

                    long index2 = listCtrl->FindItem(-1, item2);
                    auto str2 = listCtrl->GetItemText(index2, col);

                    result = str1.Cmp(str2);
                    break;
                }
            }
            if((result != 0) || (i==order.size()-1))
                return order[i].second ? result : -result;
        }
    }
}

using namespace wxGUI;

AddressesPage::AddressesPage(VcashApp &vcashApp, wxWindow &parent)
        : addresses(), wxPanel(&parent) {

    vcashApp.view.addressesPage = this;

    listCtrl = new wxListCtrl(this, wxID_ANY, wxDefaultPosition, wxSize(450, wxDefaultSize.GetHeight()),
                               wxLC_REPORT | wxLC_SINGLE_SEL | wxBORDER_NONE);
    listCtrl->InsertColumn(Column::Account, "Account", wxLIST_FORMAT_LEFT);
    listCtrl->InsertColumn(Column::Address, "Address", wxLIST_FORMAT_LEFT);

    wxSizer *pageSizer = new wxBoxSizer(wxVERTICAL);
    pageSizer->Add(listCtrl, 1, wxALL | wxEXPAND, 5);

    SetSizerAndFit(pageSizer);

    auto openMenu = [this, &vcashApp](long index, wxPoint pos) {
        bool onAddress = index >= 0;
        std::string address = onAddress ? listCtrl->GetItemText(index, Address).ToStdString() : "";

        enum PopupMenu {
            Copy, VcashExplorer, VcashProjectExplorer, New, QR
        };

        wxMenu *explorers = new wxMenu();;
        explorers->Append(VcashExplorer, wxT("Vcash Explorer"));
        explorers->Enable(VcashExplorer, onAddress);
        explorers->Append(VcashProjectExplorer, wxT("Vcash Project Explorer"));
        explorers->Enable(VcashProjectExplorer, onAddress);

        wxMenu popupMenu;
        popupMenu.AppendSubMenu(explorers, wxT("&Block explorer"));

        popupMenu.Append(Copy, wxT("&Copy"));
        popupMenu.Enable(Copy, onAddress);
        popupMenu.Append(New, wxT("&New"));

        popupMenu.Append(QR, wxT("&QR Code"));
        popupMenu.Enable(QR, onAddress);

        auto select = GetPopupMenuSelectionFromUser(popupMenu, pos);

        switch (select) {
            case Copy: {
                if (wxTheClipboard->Open()) {
                    // wxTheClipboard->Clear(); doesn't work on Windows
                    wxTheClipboard->SetData(new wxTextDataObject(address));
                    // wxTheClipboard->Flush();
                    wxTheClipboard->Close();
                }
                break;
            }
            case VcashExplorer: {
                wxLaunchDefaultBrowser(VcashExplorer::addressURL(address));
                break;
            }
            case VcashProjectExplorer: {
                wxLaunchDefaultBrowser(VcashProjectExplorer::addressURL(address));
                break;
            }
            case New: {
                wxString account = (index >= 0) ? listCtrl->GetItemText(index, Account) : wxT("");
                vcashApp.controller.onConsoleCommandEntered("getnewaddress " + account.ToStdString());
                break;
            }
            case QR:
                new QRDialog(*this, wxT("QR Address"), address);
                break;
            default:
                break;
        }
    };

    listCtrl->Bind(wxEVT_LIST_ITEM_RIGHT_CLICK, [openMenu](wxListEvent &ev) {
        long index = ev.GetIndex();
        openMenu(index, wxDefaultPosition);
        ev.Skip();
    });

    listCtrl->Bind(wxEVT_CHAR, [this, openMenu](wxKeyEvent &ev) {
        if(ev.GetKeyCode() == WXK_RETURN && listCtrl->GetSelectedItemCount() > 0) {

            long index = listCtrl->GetNextItem(-1,
                                               wxLIST_NEXT_ALL,
                                               wxLIST_STATE_SELECTED);
            if(index >= 0) {
                wxPoint pos;
                listCtrl->GetItemPosition(index, pos);
                pos.x += 50;
                pos.y += 50;
                openMenu(index, pos);
            }
        }
        ev.Skip();
    });

    // Sort is according to first element in order vector. true means ascending order.
    // In case of tie, we use next next element in vector
    sortData = { this, { { Account, true }, { Address, true } }};

    listCtrl->Bind(wxEVT_LIST_COL_CLICK, [this](wxListEvent &ev) {
        Column column = static_cast<Column >(ev.GetColumn());

        int i;
        for(i=0; (i<sortData.order.size()) && (sortData.order[i].first != column); i++)
            ;

        if(i<sortData.order.size()) {
            std::pair<Column, bool> p = sortData.order[i];
            p.second = !p.second; // invert order

            // move clicked column to first position
            for(int j=i; j>0; j--)
                sortData.order[j] = sortData.order[j-1];
            sortData.order[0] = p;
            listCtrl->SortItems(cmpAddresses, (wxIntPtr) &sortData);
        }
        ev.Skip();
    });
}

void AddressesPage::addAddress(const std::string &account, const std::string &address) {
    int newItemIndex = listCtrl->GetItemCount();
    long index;

    auto pair = addresses.insert(std::make_pair(address, newItemIndex));

    if (pair.second) {
        // Not in list. This is a new address
        wxListItem item{};
        item.SetId(newItemIndex);
        index = listCtrl->InsertItem(item);

        if (index >= 0) {
            listCtrl->SetItemPtrData(index, (wxUIntPtr) newItemIndex);
            listCtrl->SetItem(index, Account, wxString(account));
            listCtrl->SetColumnWidth(Account, wxLIST_AUTOSIZE_USEHEADER);
            listCtrl->SetItem(index, Address, wxString(address));
            listCtrl->SetColumnWidth(Address, wxLIST_AUTOSIZE_USEHEADER);
        } else
            addresses.erase(pair.first); // was not inserted in listCtrl. Remove it from addresses map
    } else {
        // This is an update
        index = listCtrl->FindItem(-1, pair.first->second); // search for item with this address
        if (index>=0) {
            listCtrl->SetItem(index, Account, wxString(account));
        }
    }

    listCtrl->SortItems(cmpAddresses, (wxIntPtr) &sortData);
}

void AddressesPage::emboldenAddress(const std::string &address, bool bold) {
    int newItemIndex = listCtrl->GetItemCount();
    long index;

    auto pair = addresses.insert(std::make_pair(address, newItemIndex));

    if (!pair.second) {
        // This is an update
        index = listCtrl->FindItem(-1, pair.first->second); // search for item with this address
        if (index>=0) {
            wxListItem info;
            info.m_itemId = pair.first->second;
            listCtrl->GetItem(info);
            wxFont font = info.GetFont();
            font.SetWeight(bold ? wxFONTWEIGHT_BOLD : wxFONTWEIGHT_NORMAL);
#if defined(__WXOSX__)
            info.SetFont(font.Scaled(1.4));
#else
            info.SetFont(font);	    
#endif
            // It seems we need to temporarily change colour in order to effectively change the font
            wxColour colour = info.GetBackgroundColour();
            info.SetBackgroundColour(wxColour(0,1,2,3));
            info.SetBackgroundColour(colour);
            listCtrl->SetItem(info);
            listCtrl->SetColumnWidth(Address, wxLIST_AUTOSIZE_USEHEADER);
        }
    }
}
